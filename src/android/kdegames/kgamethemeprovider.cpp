/*
    SPDX-FileCopyrightText: 2012 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kgamethemeprovider.h"

// own
#include <kdegames_logging.h>
// KF
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
// Qt
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QStandardPaths>
// Std
#include <utility>

class KGameThemeProviderPrivate
{
public:
    KGameThemeProvider *const q;

    QString m_name;
    QList<const KGameTheme *> m_themes;
    const QByteArray m_configKey;
    mutable const KGameTheme *m_currentTheme = nullptr;
    const KGameTheme *m_defaultTheme = nullptr;
    // this stores the arguments which were passed to discoverThemes()
    QString m_dtDirectory;
    QString m_dtDefaultThemeName;
    const QMetaObject *m_dtThemeClass = nullptr;
    // this disables the addTheme() lock during rediscoverThemes()
    bool m_inRediscover = false;

public:
    KGameThemeProviderPrivate(KGameThemeProvider *parent, const QByteArray &key)
        : q(parent)
        , m_configKey(key)
    {
    }

    void updateThemeName()
    {
        Q_EMIT q->currentThemeNameChanged(q->currentThemeName());
    }
};

KGameThemeProvider::KGameThemeProvider(const QByteArray &configKey, QObject *parent)
    : QObject(parent)
    , d_ptr(new KGameThemeProviderPrivate(this, configKey))
{
    qRegisterMetaType<const KGameTheme *>();
    qRegisterMetaType<KGameThemeProvider *>();
    connect(this, &KGameThemeProvider::currentThemeChanged, this, [this]() {
        Q_D(KGameThemeProvider);
        d->updateThemeName();
    });
}

KGameThemeProvider::~KGameThemeProvider()
{
    Q_D(KGameThemeProvider);

    if (!d->m_themes.isEmpty()) {
        // save current theme in config file (no sync() call here; this will most
        // likely be called at application shutdown when others are also writing to
        // KGlobal::config(); also KConfig's dtor will sync automatically)
        // but do not save if there is no choice; this is esp. helpful for the
        // KGameRenderer constructor overload that uses a single KGameTheme instance
        if (d->m_themes.size() > 1 && !d->m_configKey.isEmpty()) {
            KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("KgTheme"));
            cg.writeEntry(d->m_configKey.data(), currentTheme()->identifier());
        }
        // cleanup
        while (!d->m_themes.isEmpty()) {
            delete const_cast<KGameTheme *>(d->m_themes.takeFirst());
        }
    }
}

QString KGameThemeProvider::name() const
{
    Q_D(const KGameThemeProvider);

    return d->m_name;
}

QList<const KGameTheme *> KGameThemeProvider::themes() const
{
    Q_D(const KGameThemeProvider);

    return d->m_themes;
}

void KGameThemeProvider::addTheme(KGameTheme *theme)
{
    Q_D(KGameThemeProvider);

    // The intended use is to create the KGameThemeProvider object, add themes,
    //*then* start to work with the currentLevel(). The first call to
    // currentTheme() will load the previous selection from the config, and the
    // level list will be considered immutable from this point.
    Q_ASSERT_X(d->m_currentTheme == nullptr || d->m_inRediscover, "KGameThemeProvider::addTheme", "Only allowed before currentTheme() is called.");
    // add theme
    d->m_themes.append(theme);
    theme->setParent(this);
}

const KGameTheme *KGameThemeProvider::defaultTheme() const
{
    Q_D(const KGameThemeProvider);

    return d->m_defaultTheme;
}

void KGameThemeProvider::setDefaultTheme(const KGameTheme *theme)
{
    Q_D(KGameThemeProvider);

    if (d->m_currentTheme) {
        qCDebug(KDEGAMES_LOG) << "You're calling setDefaultTheme after the current "
                                 "theme has already been determined. That's not gonna work.";
        return;
    }
    Q_ASSERT(d->m_themes.contains(theme));
    d->m_defaultTheme = theme;
}

const KGameTheme *KGameThemeProvider::currentTheme() const
{
    Q_D(const KGameThemeProvider);

    if (d->m_currentTheme) {
        return d->m_currentTheme;
    }
    Q_ASSERT(!d->m_themes.isEmpty());
    // check configuration file for saved theme
    if (!d->m_configKey.isEmpty()) {
        KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("KgTheme"));
        const QByteArray id = cg.readEntry(d->m_configKey.data(), QByteArray());
        // look for a theme with this id
        for (const KGameTheme *theme : std::as_const(d->m_themes)) {
            if (theme->identifier() == id) {
                return d->m_currentTheme = theme;
            }
        }
    }
    // fall back to default theme (or first theme if no default specified)
    return d->m_currentTheme = (d->m_defaultTheme ? d->m_defaultTheme : d->m_themes.first());
}

void KGameThemeProvider::setCurrentTheme(const KGameTheme *theme)
{
    Q_D(KGameThemeProvider);

    Q_ASSERT(d->m_themes.contains(theme));
    if (d->m_currentTheme != theme) {
        d->m_currentTheme = theme;
        Q_EMIT currentThemeChanged(theme);
    }
}

QString KGameThemeProvider::currentThemeName() const
{
    Q_D(const KGameThemeProvider);

    return currentTheme()->name();
}

void KGameThemeProvider::discoverThemes(const QString &directory, const QString &defaultThemeName, const QMetaObject *themeClass)
{
    Q_D(KGameThemeProvider);

    d->m_dtDirectory = directory;
    d->m_dtDefaultThemeName = defaultThemeName;
    d->m_dtThemeClass = themeClass;
    rediscoverThemes();
}

static QStringList findSubdirectories(const QStringList &dirs)
{
    QStringList result;

    for (const QString &dir : dirs) {
        const QStringList subdirNames = QDir(dir).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        result.reserve(result.size() + subdirNames.size());
        for (const QString &subdirName : subdirNames) {
            const QString subdir = dir + QLatin1Char('/') + subdirName;
            result << subdir;
        }
    }
    if (!result.isEmpty()) {
        result += findSubdirectories(result);
    }

    return result;
}

void KGameThemeProvider::rediscoverThemes()
{
    Q_D(KGameThemeProvider);

    if (d->m_dtDirectory.isEmpty()) {
        return; // discoverThemes() was never called
    }

    KGameTheme *defaultTheme = nullptr;

    d->m_inRediscover = true;
    const QString defaultFileName = d->m_dtDefaultThemeName + QLatin1String(".desktop");

    QStringList themePaths;
    QStringList dirs = QStandardPaths::locateAll(QStandardPaths::AppDataLocation, d->m_dtDirectory, QStandardPaths::LocateDirectory);
    // KPat on Android: the bundled themes are compiled into the Qt resources
    const QString bundledDir = QLatin1String(":/share/kpat/") + d->m_dtDirectory;
    if (QDir(bundledDir).exists()) {
        dirs.append(bundledDir);
    }
    const QStringList allDirs = dirs + findSubdirectories(dirs);
    for (const QString &dir : allDirs) {
        const QStringList fileNames = QDir(dir).entryList({QStringLiteral("*.desktop")});
        for (const QString &file : fileNames) {
            if (!themePaths.contains(file)) {
                themePaths.append(dir + QLatin1Char('/') + file);
            }
        }
    }

    // create themes from result, order default theme at the front (that's not
    // needed by KGameThemeProvider, but nice for the theme selector)
    QList<KGameTheme *> themes;
    themes.reserve(themePaths.size());
    QList<QByteArray> newThemeIds;
    newThemeIds.reserve(themePaths.size());
    QList<QByteArray> oldThemeIds;
    oldThemeIds.reserve(d->m_themes.size());
    for (const KGameTheme *theme : std::as_const(d->m_themes)) {
        oldThemeIds.append(theme->identifier());
    }

    for (const QString &themePath : std::as_const(themePaths)) {
        const QString themeDesktopFileName = QFileInfo(themePath).fileName();
        // the identifier is constructed such that it is compatible with
        // KGameTheme (e.g. "themes/default.desktop")
        const QByteArray id = QString(d->m_dtDirectory + QLatin1Char('/') + themeDesktopFileName).toUtf8();

        // avoid duplicates
        if (newThemeIds.contains(id)) {
            continue;
        }

        newThemeIds.append(id);
        if (oldThemeIds.contains(id)) {
            continue;
        }

        // create theme
        KGameTheme *theme;
        if (d->m_dtThemeClass) {
            theme = qobject_cast<KGameTheme *>(d->m_dtThemeClass->newInstance(Q_ARG(QByteArray, id), Q_ARG(QObject *, this)));
            Q_ASSERT_X(theme, "KGameThemeProvider::discoverThemes", "Could not create theme instance. Is your constructor Q_INVOKABLE?");
        } else {
            theme = new KGameTheme(id, this);
        }
        // silently discard invalid theme files
        if (!theme->readFromDesktopFile(themePath)) {
            delete theme;
            continue;
        }
        // order default theme at the front (that's not necessarily needed by
        // KGameThemeProvider, but nice for the theme selector)
        if (themeDesktopFileName == defaultFileName) {
            themes.prepend(theme);
            defaultTheme = theme;
        } else {
            themes.append(theme);
        }
    }

    // remove themes no longer installed
    bool isCurrentThemeRemoved = false;
    for (const QByteArray &id : std::as_const(oldThemeIds)) {
        if (newThemeIds.contains(id)) {
            continue;
        }
        auto it = std::find_if(d->m_themes.cbegin(), d->m_themes.cend(), [id](const KGameTheme *theme) {
            return (theme->identifier() == id);
        });
        // should not happen, but safety check
        if (it == d->m_themes.cend()) {
            Q_ASSERT_X(it != d->m_themes.cend(), Q_FUNC_INFO, id.data());
            continue;
        }

        const KGameTheme *removedTheme = *it;
        if (d->m_defaultTheme == removedTheme) {
            qCWarning(KDEGAMES_LOG) << "The default theme was uninstalled, should not happen.";
            continue;
        }

        if (d->m_currentTheme == removedTheme) {
            d->m_currentTheme = nullptr;
            isCurrentThemeRemoved = true;
        }
        delete const_cast<KGameTheme *>(removedTheme);
        d->m_themes.erase(it);
    }

    // add themes in the determined order
    for (KGameTheme *theme : std::as_const(themes)) {
        addTheme(theme);
    }

    if (defaultTheme != nullptr) {
        setDefaultTheme(defaultTheme);
    } else if (d->m_defaultTheme == nullptr && themes.count() != 0) {
        setDefaultTheme(themes.value(0));
    }

    d->m_inRediscover = false;

    if (isCurrentThemeRemoved) {
        // auto-estimate new one and tell the world
        Q_EMIT currentThemeChanged(currentTheme());
    }
}

QPixmap KGameThemeProvider::generatePreview(const KGameTheme *theme, QSize size)
{
    const qreal dpr = qApp->devicePixelRatio();
    QPixmap pixmap = QPixmap(theme->previewPath()).scaled(size * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    pixmap.setDevicePixelRatio(dpr);
    return pixmap;
}

#include "moc_kgamethemeprovider.cpp"
