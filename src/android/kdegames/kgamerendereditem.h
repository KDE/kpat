/*
    SPDX-FileCopyrightText: 2010 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KGAMERENDEREDITEM_H
#define KGAMERENDEREDITEM_H

// own
#include "kdegames_export.h"
#include "kgamerendererclient.h"
// Qt
#include <QGraphicsItem>
// Std
#include <memory>

class KGameRenderedItemPrivate;

/**
 * @class KGameRenderedItem kgamerendereditem.h <KGameRenderedItem>
 * @short A QGraphicsPixmapItem which reacts to theme changes automatically.
 *
 * This class is a QGraphicsPixmapItem which retrieves its pixmap from a
 * KGameRenderer, and updates it automatically when the KGameRenderer changes
 * the theme.
 * @since 4.6
 */
class KDEGAMES_EXPORT KGameRenderedItem : public QGraphicsPixmapItem, public KGameRendererClient
{
public:
    /// Creates a new KGameRenderedItem which renders the sprite with the
    /// given @a spriteKey as provided by the given @a renderer.
    KGameRenderedItem(KGameRenderer *renderer, const QString &spriteKey, QGraphicsItem *parent = nullptr);
    ~KGameRenderedItem() override;

protected:
    void receivePixmap(const QPixmap &pixmap) override;

private:
    friend class KGameRenderedItemPrivate;
    std::unique_ptr<KGameRenderedItemPrivate> const d_ptr;
    Q_DECLARE_PRIVATE(KGameRenderedItem)
};

#endif // KGAMERENDEREDITEM_H
