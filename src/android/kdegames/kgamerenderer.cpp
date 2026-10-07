/*
    SPDX-FileCopyrightText: 2010-2012 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kgamerenderer.h"
#include "kgamerenderer_p.h"

// own
#include "colorproxy_p.h"
#include "kgamerendererclient.h"
#include "kgametheme.h"
#include "kgamethemeprovider.h"
#include <kdegames_logging.h>
// Qt
#include <QDateTime>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPainter>
#include <QVariant>

// TODO: automatically schedule pre-rendering of animation frames
// TODO: multithreaded SVG loading?

static const QString cacheName(QByteArray theme)
{
    const QString appName = QCoreApplication::instance()->applicationName();
    // e.g. "themes/foobar.desktop" -> "themes/foobar"
    if (theme.endsWith(QByteArrayLiteral(".desktop")))
        theme.chop(8); // 8 = strlen(".desktop")
    return QStringLiteral("kgamerenderer-%1-%2").arg(appName, QString::fromUtf8(theme));
}

KGameRendererPrivate::KGameRendererPrivate(KGameThemeProvider *provider, unsigned cacheSize, KGameRenderer *parent)
    : m_parent(parent)
    , m_provider(provider)
    , m_frameSuffix(QStringLiteral("_%1"))
    , m_sizePrefix(QStringLiteral("%1-%2-"))
    , m_frameCountPrefix(QStringLiteral("fc-"))
    , m_boundsPrefix(QStringLiteral("br-"))
    // default cache size: 3 MiB = 3 << 20 bytes
    , m_cacheSize((cacheSize == 0 ? 3 : cacheSize) << 20)
    , m_rendererPool(&m_workerPool)
{
    qRegisterMetaType<KGRInternal::Job *>();
}

KGameRenderer::KGameRenderer(KGameThemeProvider *provider, unsigned cacheSize)
    : d_ptr(new KGameRendererPrivate(provider, cacheSize, this))
{
    if (!provider->parent()) {
        provider->setParent(this);
    }
    connect(provider, &KGameThemeProvider::currentThemeChanged, this, [this](const KGameTheme *theme) {
        Q_D(KGameRenderer);
        d->_k_setTheme(theme);
    });
}

static KGameThemeProvider *providerForSingleTheme(KGameTheme *theme, QObject *parent)
{
    KGameThemeProvider *prov = new KGameThemeProvider(QByteArray(), parent);
    prov->addTheme(theme);
    return prov;
}

KGameRenderer::KGameRenderer(KGameTheme *theme, unsigned cacheSize)
    : d_ptr(new KGameRendererPrivate(providerForSingleTheme(theme, this), cacheSize, this))
{
}

KGameRenderer::~KGameRenderer()
{
    Q_D(KGameRenderer);

    // cleanup clients
    while (!d->m_clients.isEmpty()) {
        delete d->m_clients.constBegin().key();
    }
    // cleanup own stuff
    d->m_workerPool.waitForDone();
    delete d->m_imageCache;
}

int KGameRenderer::frameBaseIndex() const
{
    Q_D(const KGameRenderer);

    return d->m_frameBaseIndex;
}

void KGameRenderer::setFrameBaseIndex(int frameBaseIndex)
{
    Q_D(KGameRenderer);

    d->m_frameBaseIndex = frameBaseIndex;
}

QString KGameRenderer::frameSuffix() const
{
    Q_D(const KGameRenderer);

    return d->m_frameSuffix;
}

void KGameRenderer::setFrameSuffix(const QString &suffix)
{
    Q_D(KGameRenderer);

    d->m_frameSuffix = suffix.contains(QLatin1String("%1")) ? suffix : QStringLiteral("_%1");
}

KGameRenderer::Strategies KGameRenderer::strategies() const
{
    Q_D(const KGameRenderer);

    return d->m_strategies;
}

void KGameRenderer::setStrategyEnabled(KGameRenderer::Strategy strategy, bool enabled)
{
    Q_D(KGameRenderer);

    const bool oldEnabled = d->m_strategies & strategy;
    if (enabled) {
        d->m_strategies |= strategy;
    } else {
        d->m_strategies &= ~strategy;
    }
    if (strategy == KGameRenderer::UseDiskCache && oldEnabled != enabled) {
        // reload theme
        const KGameTheme *theme = d->m_currentTheme;
        if (theme) {
            d->m_currentTheme = nullptr; // or setTheme() will return immediately
            d->_k_setTheme(theme);
        }
    }
}

void KGameRendererPrivate::_k_setTheme(const KGameTheme *theme)
{
    const KGameTheme *oldTheme = m_currentTheme;
    if (oldTheme == theme) {
        return;
    }
    qCDebug(KDEGAMES_LOG) << "Setting theme:" << theme->name();
    if (!setTheme(theme)) {
        const KGameTheme *defaultTheme = m_provider->defaultTheme();
        if (theme != defaultTheme) {
            qCDebug(KDEGAMES_LOG) << "Falling back to default theme:" << defaultTheme->name();
            setTheme(defaultTheme);
            m_provider->setCurrentTheme(defaultTheme);
        }
    }
    // announce change to KGameRendererClients
    QHash<KGameRendererClient *, QString>::iterator it1 = m_clients.begin(), it2 = m_clients.end();
    for (; it1 != it2; ++it1) {
        it1.value().clear(); // because the pixmap is outdated
        it1.key()->d_ptr->fetchPixmap();
    }
    Q_EMIT m_parent->themeChanged(m_currentTheme);
}

bool KGameRendererPrivate::setTheme(const KGameTheme *theme)
{
    if (!theme) {
        return false;
    }
    // open cache (and SVG file, if necessary)
    if (m_strategies & KGameRenderer::UseDiskCache) {
        std::unique_ptr<KImageCache> oldCache(m_imageCache);
        const QString imageCacheName = cacheName(theme->identifier());
        m_imageCache = new KImageCache(imageCacheName, m_cacheSize);
        m_imageCache->setPixmapCaching(false); // see big comment in KGRPrivate class declaration
        // check timestamp of cache vs. last write access to theme/SVG
        const uint svgTimestamp =
            qMax(QFileInfo(theme->graphicsPath()).lastModified().toSecsSinceEpoch(), theme->property("_k_themeDescTimestamp").value<qint64>());
        QByteArray buffer;
        if (!m_imageCache->find(QStringLiteral("kgr_timestamp"), &buffer))
            buffer = "0";
        const uint cacheTimestamp = buffer.toInt();
        // try to instantiate renderer immediately if the cache does not exist or is outdated
        // FIXME: This logic breaks if the cache evicts the "kgr_timestamp" key. We need additional API in KSharedDataCache to make sure that this key does not
        // get evicted.
        if (cacheTimestamp < svgTimestamp) {
            qCDebug(KDEGAMES_LOG) << "Theme newer than cache, checking SVG";
            std::unique_ptr<QSvgRenderer> renderer(new QSvgRenderer(theme->graphicsPath()));
            if (renderer->isValid()) {
                m_rendererPool.setPath(theme->graphicsPath(), renderer.release());
                m_imageCache->clear();
                m_imageCache->insert(QStringLiteral("kgr_timestamp"), QByteArray::number(svgTimestamp));
            } else {
                // The SVG file is broken, so we deny to change the theme without
                // breaking the previous theme.
                delete m_imageCache;
                KSharedDataCache::deleteCache(imageCacheName);
                m_imageCache = oldCache.release();
                qCDebug(KDEGAMES_LOG) << "Theme change failed: SVG file broken";
                return false;
            }
        }
        // theme is cached - just delete the old renderer after making sure that no worker threads are using it anymore
        else if (m_currentTheme != theme)
            m_rendererPool.setPath(theme->graphicsPath());
    } else // !(m_strategies & KGameRenderer::UseDiskCache) -> no cache is used
    {
        // load SVG file
        std::unique_ptr<QSvgRenderer> renderer(new QSvgRenderer(theme->graphicsPath()));
        if (renderer->isValid()) {
            m_rendererPool.setPath(theme->graphicsPath(), renderer.release());
        } else {
            qCDebug(KDEGAMES_LOG) << "Theme change failed: SVG file broken";
            return false;
        }
        // disconnect from disk cache (only needed if changing strategy)
        delete m_imageCache;
        m_imageCache = nullptr;
    }
    // clear in-process caches
    m_pixmapCache.clear();
    m_frameCountCache.clear();
    m_boundsCache.clear();
    // done
    m_currentTheme = theme;
    return true;
}

const KGameTheme *KGameRenderer::theme() const
{
    Q_D(const KGameRenderer);

    // ensure that some theme is loaded
    if (!d->m_currentTheme) {
        const_cast<KGameRendererPrivate *>(d)->_k_setTheme(d->m_provider->currentTheme());
    }
    return d->m_currentTheme;
}

KGameThemeProvider *KGameRenderer::themeProvider() const
{
    Q_D(const KGameRenderer);

    return d->m_provider;
}

QString KGameRendererPrivate::spriteFrameKey(const QString &key, int frame, bool normalizeFrameNo) const
{
    // fast path for non-animated sprites
    if (frame < 0) {
        return key;
    }
    // normalize frame number
    if (normalizeFrameNo) {
        const int frameCount = m_parent->frameCount(key);
        if (frameCount <= 0) {
            // non-animated sprite
            return key;
        } else {
            frame = (frame - m_frameBaseIndex) % frameCount + m_frameBaseIndex;
        }
    }
    return key + m_frameSuffix.arg(frame);
}

int KGameRenderer::frameCount(const QString &key) const
{
    Q_D(const KGameRenderer);

    // ensure that some theme is loaded
    if (!d->m_currentTheme) {
        const_cast<KGameRendererPrivate *>(d)->_k_setTheme(d->m_provider->currentTheme());
    }
    // look up in in-process cache
    QHash<QString, int>::const_iterator it = d->m_frameCountCache.constFind(key);
    if (it != d->m_frameCountCache.constEnd()) {
        return it.value();
    }
    // look up in shared cache (if SVG is not yet loaded)
    int count = -1;
    bool countFound = false;
    const QString cacheKey = d->m_frameCountPrefix + key;
    if (d->m_rendererPool.hasAvailableRenderers() && (d->m_strategies & KGameRenderer::UseDiskCache)) {
        QByteArray buffer;
        if (d->m_imageCache->find(cacheKey, &buffer)) {
            count = buffer.toInt();
            countFound = true;
        }
    }
    // determine from SVG
    if (!countFound) {
        QSvgRenderer *renderer = d->m_rendererPool.allocRenderer();
        // look for animated sprite first
        count = d->m_frameBaseIndex;
        while (renderer->elementExists(d->spriteFrameKey(key, count, false))) {
            ++count;
        }
        count -= d->m_frameBaseIndex;
        // look for non-animated sprite instead
        if (count == 0) {
            if (!renderer->elementExists(key)) {
                count = -1;
            }
        }
        d->m_rendererPool.freeRenderer(renderer);
        // save in shared cache for following requests
        if (d->m_strategies & KGameRenderer::UseDiskCache) {
            d->m_imageCache->insert(cacheKey, QByteArray::number(count));
        }
    }
    d->m_frameCountCache.insert(key, count);
    return count;
}

QRectF KGameRenderer::boundsOnSprite(const QString &key, int frame) const
{
    Q_D(const KGameRenderer);

    const QString elementKey = d->spriteFrameKey(key, frame);
    // ensure that some theme is loaded
    if (!d->m_currentTheme) {
        const_cast<KGameRendererPrivate *>(d)->_k_setTheme(d->m_provider->currentTheme());
    }
    // look up in in-process cache
    QHash<QString, QRectF>::const_iterator it = d->m_boundsCache.constFind(elementKey);
    if (it != d->m_boundsCache.constEnd()) {
        return it.value();
    }
    // look up in shared cache (if SVG is not yet loaded)
    QRectF bounds;
    bool boundsFound = false;
    const QString cacheKey = d->m_boundsPrefix + elementKey;
    if (!d->m_rendererPool.hasAvailableRenderers() && (d->m_strategies & KGameRenderer::UseDiskCache)) {
        QByteArray buffer;
        if (d->m_imageCache->find(cacheKey, &buffer)) {
            QDataStream stream(buffer);
            stream >> bounds;
            boundsFound = true;
        }
    }
    // determine from SVG
    if (!boundsFound) {
        QSvgRenderer *renderer = d->m_rendererPool.allocRenderer();
        bounds = renderer->boundsOnElement(elementKey);
        d->m_rendererPool.freeRenderer(renderer);
        // save in shared cache for following requests
        if (d->m_strategies & KGameRenderer::UseDiskCache) {
            QByteArray buffer;
            {
                QDataStream stream(&buffer, QIODevice::WriteOnly);
                stream << bounds;
            }
            d->m_imageCache->insert(cacheKey, buffer);
        }
    }
    d->m_boundsCache.insert(elementKey, bounds);
    return bounds;
}

bool KGameRenderer::spriteExists(const QString &key) const
{
    Q_D(const KGameRenderer);

    return this->frameCount(key) >= 0;
}

QPixmap KGameRenderer::spritePixmap(const QString &key, QSize size, int frame, const QHash<QColor, QColor> &customColors) const
{
    Q_D(const KGameRenderer);

    QPixmap result;
    const_cast<KGameRendererPrivate *>(d)->requestPixmap(KGRInternal::ClientSpec(key, frame, size, customColors), nullptr, &result);
    return result;
}

// Helper function for KGameRendererPrivate::requestPixmap.
void KGameRendererPrivate::requestPixmap__propagateResult(const QPixmap &pixmap, KGameRendererClient *client, QPixmap *synchronousResult)
{
    if (client) {
        client->receivePixmap(pixmap);
    }
    if (synchronousResult) {
        *synchronousResult = pixmap;
    }
}

void KGameRendererPrivate::requestPixmap(const KGRInternal::ClientSpec &spec, KGameRendererClient *client, QPixmap *synchronousResult)
{
    // NOTE: If client == 0, the request is synchronous and must be finished when this method returns. This behavior is used by KGR::spritePixmap(). Instead of
    // KGameRendererClient::receivePixmap, the QPixmap* argument is then used to return the result. parse request
    if (spec.size.isEmpty()) {
        requestPixmap__propagateResult(QPixmap(), client, synchronousResult);
        return;
    }
    const QString elementKey = spriteFrameKey(spec.spriteKey, spec.frame);
    const qreal dpr = qApp->devicePixelRatio();
    const QSize size = spec.size * dpr;
    QString cacheKey = m_sizePrefix.arg(size.width()).arg(size.height()) + elementKey;
    QHash<QColor, QColor>::const_iterator it1 = spec.customColors.constBegin(), it2 = spec.customColors.constEnd();
    static const QString colorSuffix(QStringLiteral("-%1-%2"));
    for (; it1 != it2; ++it1) {
        cacheKey += colorSuffix.arg(it1.key().rgba()).arg(it1.value().rgba());
    }
    // check if update is needed
    if (client) {
        if (m_clients.value(client) == cacheKey) {
            return;
        }
        m_clients[client] = cacheKey;
    }
    // ensure that some theme is loaded
    if (!m_currentTheme) {
        _k_setTheme(m_provider->currentTheme());
    }
    // try to serve from high-speed cache
    QHash<QString, QPixmap>::const_iterator it = m_pixmapCache.constFind(cacheKey);
    if (it != m_pixmapCache.constEnd()) {
        QPixmap p = it.value();
        p.setDevicePixelRatio(dpr);
        requestPixmap__propagateResult(p, client, synchronousResult);
        return;
    }
    // try to serve from low-speed cache
    if (m_strategies & KGameRenderer::UseDiskCache) {
        QPixmap pix;
        if (m_imageCache->findPixmap(cacheKey, &pix)) {
            pix.setDevicePixelRatio(dpr);
            m_pixmapCache.insert(cacheKey, pix);
            requestPixmap__propagateResult(pix, client, synchronousResult);
            return;
        }
    }
    // if asynchronous request, is such a rendering job already running?
    if (client && m_pendingRequests.contains(cacheKey)) {
        return;
    }
    // create job
    KGRInternal::Job *job = new KGRInternal::Job;
    job->rendererPool = &m_rendererPool;
    job->cacheKey = cacheKey;
    job->elementKey = elementKey;
    job->spec = spec;
    job->spec.size = size;
    const bool synchronous = !client;
    if (synchronous || !(m_strategies & KGameRenderer::UseRenderingThreads)) {
        KGRInternal::Worker worker(job, true, this);
        worker.run();
        // if everything worked fine, result is in high-speed cache now
        const QPixmap result = m_pixmapCache.value(cacheKey);
        requestPixmap__propagateResult(result, client, synchronousResult);
    } else {
        m_workerPool.start(new KGRInternal::Worker(job, !client, this));
        m_pendingRequests << cacheKey;
    }
}

void KGameRendererPrivate::jobFinished(KGRInternal::Job *job, bool isSynchronous)
{
    // read job
    const qreal dpr = qApp->devicePixelRatio();
    const QString cacheKey = job->cacheKey;
    QImage result = job->result;
    result.setDevicePixelRatio(dpr);
    delete job;
    // check who wanted this pixmap
    m_pendingRequests.removeAll(cacheKey);
    const QList<KGameRendererClient *> requesters = m_clients.keys(cacheKey);
    // put result into image cache
    if (m_strategies & KGameRenderer::UseDiskCache) {
        m_imageCache->insertImage(cacheKey, result);
        // convert result to pixmap (and put into pixmap cache) only if it is needed now
        // This optimization saves the image-pixmap conversion for intermediate sizes which occur during smooth resize events or window initializations.
        if (!isSynchronous && requesters.isEmpty()) {
            return;
        }
    }
    const QPixmap pixmap = QPixmap::fromImage(result);
    m_pixmapCache.insert(cacheKey, pixmap);
    for (KGameRendererClient *requester : requesters) {
        requester->receivePixmap(pixmap);
    }
}

// BEGIN KGRInternal::Job/Worker

KGRInternal::Worker::Worker(KGRInternal::Job *job, bool isSynchronous, KGameRendererPrivate *parent)
    : m_job(job)
    , m_synchronous(isSynchronous)
    , m_parent(parent)
{
}

static const uint transparentRgba = QColor(Qt::transparent).rgba();

void KGRInternal::Worker::run()
{
    QImage image(m_job->spec.size, QImage::Format_ARGB32_Premultiplied);
    // image might not be created if size is too large for needed memory, or invalid
    if (image.isNull()) {
        qCWarning(KDEGAMES_LOG) << "Could not create QImage. Key:" << m_job->spec.spriteKey << "Frame:" << m_job->spec.frame << "Size:" << m_job->spec.size;
    } else {
        image.fill(transparentRgba);
        QPainter *painter = nullptr;
        QPaintDeviceColorProxy *proxy = nullptr;
        // if no custom colors requested, paint directly onto image
        if (m_job->spec.customColors.isEmpty()) {
            painter = new QPainter(&image);
        } else {
            proxy = new QPaintDeviceColorProxy(&image, m_job->spec.customColors);
            painter = new QPainter(proxy);
        }

        // do renderering
        QSvgRenderer *renderer = m_job->rendererPool->allocRenderer();
        renderer->render(painter, m_job->elementKey);
        m_job->rendererPool->freeRenderer(renderer);
        delete painter;
        delete proxy;
    }
    // talk back to the main thread
    m_job->result = image;
    QMetaObject::invokeMethod(m_parent, "jobFinished", Qt::AutoConnection, Q_ARG(KGRInternal::Job *, m_job), Q_ARG(bool, m_synchronous));
    // NOTE: KGR::spritePixmap relies on Qt::DirectConnection when this method is run in the main thread.
}

// END KGRInternal::Job/Worker

// BEGIN KGRInternal::RendererPool

KGRInternal::RendererPool::RendererPool(QThreadPool *threadPool)
    : m_valid(Checked_Invalid) // don't try to allocate renderers until given a valid SVG file
    , m_threadPool(threadPool)
{
}

KGRInternal::RendererPool::~RendererPool()
{
    // This deletes all renderers.
    setPath(QString());
}

void KGRInternal::RendererPool::setPath(const QString &graphicsPath, QSvgRenderer *renderer)
{
    QMutexLocker locker(&m_mutex);
    // delete all renderers
    m_threadPool->waitForDone();
    QHash<QSvgRenderer *, QThread *>::const_iterator it1 = m_hash.constBegin(), it2 = m_hash.constEnd();
    for (; it1 != it2; ++it1) {
        Q_ASSERT(it1.value() == nullptr); // nobody may be using our renderers anymore now
        delete it1.key();
    }
    m_hash.clear();
    // set path
    m_path = graphicsPath;
    // existence of a renderer instance is evidence for the validity of the SVG file
    if (renderer) {
        m_valid = Checked_Valid;
        m_hash.insert(renderer, nullptr);
    } else {
        m_valid = Unchecked;
    }
}

bool KGRInternal::RendererPool::hasAvailableRenderers() const
{
    // look for a renderer which is not associated with a thread
    QMutexLocker locker(&m_mutex);
    return m_hash.key(nullptr) != nullptr;
}

QSvgRenderer *KGRInternal::RendererPool::allocRenderer()
{
    QThread *thread = QThread::currentThread();
    // look for an available renderer
    QMutexLocker locker(&m_mutex);
    QSvgRenderer *renderer = m_hash.key(nullptr);
    if (!renderer) {
        // instantiate a new renderer (only if the SVG file has not been found to be invalid yet)
        if (m_valid == Checked_Invalid) {
            return nullptr;
        }
        renderer = new QSvgRenderer(m_path);
        m_valid = renderer->isValid() ? Checked_Valid : Checked_Invalid;
    }
    // mark renderer as used
    m_hash.insert(renderer, thread);
    return renderer;
}

void KGRInternal::RendererPool::freeRenderer(QSvgRenderer *renderer)
{
    // mark renderer as available
    QMutexLocker locker(&m_mutex);
    m_hash.insert(renderer, nullptr);
}

// END KGRInternal::RendererPool

#include "moc_kgamerenderer.cpp"
#include "moc_kgamerenderer_p.cpp"
