/*
    SPDX-FileCopyrightText: 2010 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kgamerendererclient.h"

// own
#include "kgamerenderer.h"
#include "kgamerenderer_p.h"
// Qt
#include <QTimer>

KGameRendererClientPrivate::KGameRendererClientPrivate(KGameRenderer *renderer, const QString &spriteKey, KGameRendererClient *parent)
    : m_parent(parent)
    , m_renderer(renderer)
    , m_spec(spriteKey, -1, QSize())
{
}

KGameRendererClient::KGameRendererClient(KGameRenderer *renderer, const QString &spriteKey)
    : d_ptr(new KGameRendererClientPrivate(renderer, spriteKey, this))
{
    Q_D(KGameRendererClient);

    renderer->d_ptr->m_clients.insert(this, QString());
    // The following may not be triggered directly because it may call receivePixmap() which is a pure virtual method at this point.
    QTimer::singleShot(0, d, &KGameRendererClientPrivate::fetchPixmap);
}

KGameRendererClient::~KGameRendererClient()
{
    Q_D(KGameRendererClient);

    d->m_renderer->d_ptr->m_clients.remove(this);
}

KGameRenderer *KGameRendererClient::renderer() const
{
    Q_D(const KGameRendererClient);

    return d->m_renderer;
}

QString KGameRendererClient::spriteKey() const
{
    Q_D(const KGameRendererClient);

    return d->m_spec.spriteKey;
}

void KGameRendererClient::setSpriteKey(const QString &spriteKey)
{
    Q_D(KGameRendererClient);

    if (d->m_spec.spriteKey != spriteKey) {
        d->m_spec.spriteKey = spriteKey;
        d->fetchPixmap();
    }
}

int KGameRendererClient::frameCount() const
{
    Q_D(const KGameRendererClient);

    return d->m_renderer->frameCount(d->m_spec.spriteKey);
}

int KGameRendererClient::frame() const
{
    Q_D(const KGameRendererClient);

    return d->m_spec.frame;
}

void KGameRendererClient::setFrame(int frame)
{
    Q_D(KGameRendererClient);

    if (d->m_spec.frame != frame) {
        // do some normalization ourselves
        const int frameCount = this->frameCount();
        if (frameCount <= 0 || frame < 0) {
            frame = -1;
        } else {
            const int frameBaseIndex = d->m_renderer->frameBaseIndex();
            frame = (frame - frameBaseIndex) % frameCount + frameBaseIndex;
        }
        if (d->m_spec.frame != frame) {
            d->m_spec.frame = frame;
            d->fetchPixmap();
        }
    }
}

QSize KGameRendererClient::renderSize() const
{
    Q_D(const KGameRendererClient);

    return d->m_spec.size;
}

void KGameRendererClient::setRenderSize(QSize renderSize)
{
    Q_D(KGameRendererClient);

    if (d->m_spec.size != renderSize) {
        d->m_spec.size = renderSize;
        d->fetchPixmap();
    }
}

QHash<QColor, QColor> KGameRendererClient::customColors() const
{
    Q_D(const KGameRendererClient);

    return d->m_spec.customColors;
}

void KGameRendererClient::setCustomColors(const QHash<QColor, QColor> &customColors)
{
    Q_D(KGameRendererClient);

    if (d->m_spec.customColors != customColors) {
        d->m_spec.customColors = customColors;
        d->fetchPixmap();
    }
}

void KGameRendererClientPrivate::fetchPixmap()
{
    m_renderer->d_ptr->requestPixmap(m_spec, m_parent);
}
