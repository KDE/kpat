/*
    SPDX-FileCopyrightText: 2010 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KGAMERENDERER_P_H
#define KGAMERENDERER_P_H

// KF
#include <KImageCache>
// Qt
#include <QHash>
#include <QMetaType>
#include <QMutex>
#include <QRunnable>
#include <QSvgRenderer>
#include <QThreadPool>

namespace KGRInternal
{
// Describes the state of a KGameRendererClient.
struct ClientSpec {
    // The parentheses around QHash<QColor, QColor>() avoid compile
    // errors on platforms with older gcc versions, e.g. OS X 10.6.
    inline ClientSpec(const QString &spriteKey = QString(),
                      int frame = -1,
                      QSize size = QSize(),
                      const QHash<QColor, QColor> &customColors = (QHash<QColor, QColor>()));
    QString spriteKey;
    int frame;
    QSize size;
    QHash<QColor, QColor> customColors;
};
ClientSpec::ClientSpec(const QString &spriteKey_, int frame_, QSize size_, const QHash<QColor, QColor> &customColors_)
    : spriteKey(spriteKey_)
    , frame(frame_)
    , size(size_)
    , customColors(customColors_)
{
}

// Instantiates QSvgRenderer instances from one SVG file for multiple threads.
class RendererPool
{
public:
    // The renderer pool needs the thread pool instance of
    // KGameRendererPrivate to terminate workers when a new SVG is loaded.
    // WARNING Call this only from the main thread.
    inline RendererPool(QThreadPool *threadPool);
    inline ~RendererPool();

    // The second argument can be used to pass an instance which has been
    // used earlier to check the validity of the SVG file.
    inline void setPath(const QString &graphicsPath, QSvgRenderer *renderer = nullptr);
    // This can be used to determine whether a call to allocRenderer()
    // would need to create a new renderer instance.
    inline bool hasAvailableRenderers() const;

    // Returns a SVG renderer instance that can be used in the calling thread.
    inline QSvgRenderer *allocRenderer();
    // Marks this renderer as available for allocation by other threads.
    inline void freeRenderer(QSvgRenderer *renderer);

private:
    QString m_path; // path to SVG file
    enum Validity {
        Checked_Invalid,
        Checked_Valid,
        Unchecked
    };
    Validity m_valid; // holds whether m_path points to a valid file

    mutable QMutex m_mutex;
    QThreadPool *m_threadPool;
    QHash<QSvgRenderer *, QThread *> m_hash;
};

// Describes a rendering job which is delegated to a worker thread.
struct Job {
    KGRInternal::RendererPool *rendererPool;
    ClientSpec spec;
    QString cacheKey, elementKey;
    QImage result;
};

// Describes a worker thread.
class Worker : public QRunnable
{
public:
    Worker(Job *job, bool isSynchronous, KGameRendererPrivate *parent);

    void run() override;

private:
    Job *m_job;
    bool m_synchronous;
    KGameRendererPrivate *m_parent;
};
}

Q_DECLARE_METATYPE(KGRInternal::Job *)

class KGameRendererPrivate : public QObject
{
    Q_OBJECT

public:
    KGameRendererPrivate(KGameThemeProvider *provider, unsigned cacheSize, KGameRenderer *parent);

    void _k_setTheme(const KGameTheme *theme);
    bool setTheme(const KGameTheme *theme);
    inline QString spriteFrameKey(const QString &key, int frame, bool normalizeFrameNo = false) const;
    void requestPixmap(const KGRInternal::ClientSpec &spec, KGameRendererClient *client, QPixmap *synchronousResult = nullptr);

private:
    inline void requestPixmap__propagateResult(const QPixmap &pixmap, KGameRendererClient *client, QPixmap *synchronousResult);
public Q_SLOTS:
    void jobFinished(KGRInternal::Job *job, bool isSynchronous); // NOTE: This is invoked from KGRInternal::Worker::run.

public:
    KGameRenderer *const m_parent;

    KGameThemeProvider *m_provider;
    const KGameTheme *m_currentTheme = nullptr; // will be loaded on first use

    QString m_frameSuffix, m_sizePrefix, m_frameCountPrefix, m_boundsPrefix;
    unsigned m_cacheSize;
    KGameRenderer::Strategies m_strategies = KGameRenderer::UseDiskCache | KGameRenderer::UseRenderingThreads;
    int m_frameBaseIndex = 0;

    QThreadPool m_workerPool;
    mutable KGRInternal::RendererPool m_rendererPool;

    QHash<KGameRendererClient *, QString> m_clients; // maps client -> cache key of current pixmap
    QStringList m_pendingRequests; // cache keys of pixmaps which are currently being rendered

    KImageCache *m_imageCache = nullptr;

    // In multi-threaded scenarios, there are two possible ways to use KIC's
    // pixmap cache.
    // 1. The worker renders a QImage and stores it in the cache. The main
    //    thread reads the QImage again and converts it into a QPixmap,
    //    storing it in the pixmap cache for later re-use.
    // i.e. QImage -> diskcache -> QImage -> QPixmap -> pixmapcache -> serve
    // 2. The worker renders a QImage and sends it directly to the main
    //    thread, which converts it to a QPixmap. The QPixmap is stored in
    //    KIC's pixmap cache, and converted to QImage to be written to the
    //    shared data cache.
    // i.e. QImage -> QPixmap -> pixmapcache -> serve
    //                       \-> QImage -> diskcache
    // We choose a third way:
    // 3. The worker renders a QImage which is converted to a QPixmap by the
    //    main thread. The main thread caches the QPixmap itself, and stores
    //    the QImage in the cache.
    // i.e. QImage -> QPixmap -> pixmapcache -> serve
    //            \-> diskcache
    // As you see, implementing an own pixmap cache saves us one conversion.
    // We therefore disable KIC's pixmap cache because we do not need it.
    QHash<QString, QPixmap> m_pixmapCache;
    mutable QHash<QString, int> m_frameCountCache;
    mutable QHash<QString, QRectF> m_boundsCache;
};

class KGameRendererClientPrivate : public QObject
{
    Q_OBJECT

public:
    KGameRendererClientPrivate(KGameRenderer *renderer, const QString &spriteKey, KGameRendererClient *parent);

public Q_SLOTS:
    void fetchPixmap();

public:
    KGameRendererClient *const m_parent;

    KGameRenderer *m_renderer;

    KGRInternal::ClientSpec m_spec;
};

#endif // KGAMERENDERER_P_H
