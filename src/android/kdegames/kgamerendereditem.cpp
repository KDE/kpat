/*
    SPDX-FileCopyrightText: 2010 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kgamerendereditem.h"

class KGameRenderedItemPrivate
{
    // NOTE: reserved for later use
};

KGameRenderedItem::KGameRenderedItem(KGameRenderer *renderer, const QString &spriteKey, QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent)
    , KGameRendererClient(renderer, spriteKey)
    , d_ptr(nullptr)
{
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

KGameRenderedItem::~KGameRenderedItem() = default;

void KGameRenderedItem::receivePixmap(const QPixmap &pixmap)
{
    QGraphicsPixmapItem::setPixmap(pixmap);
}
