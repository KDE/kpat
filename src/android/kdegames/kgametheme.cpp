/*
    SPDX-FileCopyrightText: 2012 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kgametheme.h"

// own
#include <kdegames_logging.h>
// KF
#include <KConfig>
#include <KConfigGroup>
// Qt
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
// Std
#include <utility>

class KGameThemePrivate
{
public:
    const QByteArray m_identifier;
    QString m_name, m_description, m_author, m_authorEmail, m_graphicsPath, m_previewPath;
    QString m_license;
    QString m_copyrightText;
    QString m_version;
    QString m_website;
    QString m_bugReportUrl;
    QMap<QString, QString> m_customData;

public:
    explicit KGameThemePrivate(const QByteArray &id)
        : m_identifier(id)
    {
    }

    static QStringList s_configGroupNames;
};

/*static*/ QStringList KGameThemePrivate::s_configGroupNames;

KGameTheme::KGameTheme(const QByteArray &identifier, QObject *parent)
    : QObject(parent)
    , d_ptr(new KGameThemePrivate(identifier))
{
}

KGameTheme::~KGameTheme() = default;

QByteArray KGameTheme::identifier() const
{
    Q_D(const KGameTheme);

    return d->m_identifier;
}

#define KGAMETHEME_STRING_PROPERTY(PROP, SETTER)                                                                                                               \
    QString KGameTheme::PROP() const                                                                                                                           \
    {                                                                                                                                                          \
        Q_D(const KGameTheme);                                                                                                                                 \
        return d->m_##PROP;                                                                                                                                    \
    }                                                                                                                                                          \
    void KGameTheme::SETTER(const QString &val)                                                                                                                \
    {                                                                                                                                                          \
        Q_D(KGameTheme);                                                                                                                                       \
        d->m_##PROP = val;                                                                                                                                     \
    }

KGAMETHEME_STRING_PROPERTY(name, setName)
KGAMETHEME_STRING_PROPERTY(description, setDescription)
KGAMETHEME_STRING_PROPERTY(license, setLicense)
KGAMETHEME_STRING_PROPERTY(copyrightText, setCopyrightText)
KGAMETHEME_STRING_PROPERTY(version, setVersion)
KGAMETHEME_STRING_PROPERTY(website, setWebsite)
KGAMETHEME_STRING_PROPERTY(bugReportUrl, setBugReportUrl)
KGAMETHEME_STRING_PROPERTY(author, setAuthor)
KGAMETHEME_STRING_PROPERTY(authorEmail, setAuthorEmail)
KGAMETHEME_STRING_PROPERTY(graphicsPath, setGraphicsPath)
KGAMETHEME_STRING_PROPERTY(previewPath, setPreviewPath)

QMap<QString, QString> KGameTheme::customData() const
{
    Q_D(const KGameTheme);

    return d->m_customData;
}

QString KGameTheme::customData(const QString &key, const QString &defaultValue) const
{
    Q_D(const KGameTheme);

    return d->m_customData.value(key, defaultValue);
}

void KGameTheme::setCustomData(const QMap<QString, QString> &customData)
{
    Q_D(KGameTheme);

    d->m_customData = customData;
}

bool KGameTheme::readFromDesktopFile(const QString &path_)
{
    if (path_.isEmpty()) {
        qCDebug(KDEGAMES_LOG) << "Refusing to load theme with no name";
        return false;
    }
    // legacy support: relative paths are resolved with KStandardDirs/appdata
    QString path(path_);
    if (QFileInfo(path).isRelative()) {
        path = QStandardPaths::locate(QStandardPaths::AppDataLocation, path);
        if (path.isEmpty()) {
            qCDebug(KDEGAMES_LOG) << "Could not find theme description" << path;
            return false;
        }
    }
    // default group name
    if (KGameThemePrivate::s_configGroupNames.isEmpty()) {
        KGameThemePrivate::s_configGroupNames << QStringLiteral("KGameTheme");
    }
    // open file, look for a known config group
    KConfig config(path, KConfig::SimpleConfig);
    KConfigGroup group;
    for (const QString &groupName : std::as_const(KGameThemePrivate::s_configGroupNames)) {
        if (config.hasGroup(groupName)) {
            group = config.group(groupName);
        }
    }
    if (!group.isValid()) {
        qCDebug(KDEGAMES_LOG) << "Could not read theme description at" << path;
        return false;
    }
    // check format version
    if (group.readEntry("VersionFormat", 1) > 1) {
        qCDebug(KDEGAMES_LOG) << "Format of theme description too new at" << path;
        return false;
    }

    // resolve paths
    const QFileInfo fi(path);
    const QDir dir = fi.dir();
    QString graphicsPath = group.readEntry("FileName", QString());
    if (!graphicsPath.isEmpty() && QFileInfo(graphicsPath).isRelative())
        graphicsPath = dir.absoluteFilePath(graphicsPath);
    QString previewPath = group.readEntry("Preview", QString());
    if (!previewPath.isEmpty() && QFileInfo(previewPath).isRelative())
        previewPath = dir.absoluteFilePath(previewPath);
    // create theme
    setName(group.readEntry("Name", QString()));
    setDescription(group.readEntry("Description", QString()));
    setLicense(group.readEntry("License", QString()));
    setCopyrightText(group.readEntry("Copyright", QString()));
    setVersion(group.readEntry("Version", QString()));
    setWebsite(group.readEntry("Website", QString()));
    setBugReportUrl(group.readEntry("BugReportUrl", QString()));
    setAuthor(group.readEntry("Author", QString()));
    setAuthorEmail(group.readEntry("AuthorEmail", QString()));
    setGraphicsPath(graphicsPath);
    setPreviewPath(previewPath);
    setCustomData(group.entryMap());
    // store modification date of this file in private property (KGameRenderer
    // wants to clear its cache also if the theme description changes)
    setProperty("_k_themeDescTimestamp", fi.lastModified().toSecsSinceEpoch());
    return true;
}

#include "moc_kgametheme.cpp"
