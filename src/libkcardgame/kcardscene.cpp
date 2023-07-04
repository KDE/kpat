/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009-2010 Parker Coates <coates@kde.org>
 *
 * License of original code:
 * -------------------------------------------------------------------------
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation.
 *
 *   This file is provided AS IS with no warranties of any kind.  The author
 *   shall have no liability with respect to the infringement of copyrights,
 *   trade secrets or any patents by this file or any part thereof.  In no
 *   event will the author be liable for any lost revenue or profits or
 *   other special, indirect and consequential damages.
 * -------------------------------------------------------------------------
 *
 * License of modifications/additions made after 2009-01-01:
 * -------------------------------------------------------------------------
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -------------------------------------------------------------------------
 */

#include "kcardscene.h"

// own
#include "kabstractcarddeck.h"
#include "kcardpile.h"
// Qt
#include <QGraphicsSceneWheelEvent>
#include <QPainter>
#include <QPointer>
// Std
#include <cmath>

namespace
{
const int cardMoveDuration = 230;

void setItemHighlight(QGraphicsItem *item, bool highlight)
{
    KCard *card = qgraphicsitem_cast<KCard *>(item);
    if (card) {
        card->setHighlighted(highlight);
        return;
    }

    KCardPile *pile = qgraphicsitem_cast<KCardPile *>(item);
    if (pile) {
        pile->setHighlighted(highlight);
        return;
    }
}

QGraphicsItem *toGraphicsItem(QObject *object)
{
    if (KCard *card = qobject_cast<KCard *>(object))
        return card;

    if (KCardPile *pile = qobject_cast<KCardPile *>(object))
        return pile;

    Q_ASSERT(!object);

    return nullptr;
}
}

class KCardScenePrivate : public QObject
{
public:
    KCardScenePrivate(KCardScene *p);

    KCardPile *bestDestinationPileUnderCards();
    void sendCardsToPile(KCardPile *pile, QList<KCard *> cards, qreal rate, bool isSpeed, bool flip);
    void changeFocus(int pileChange, int cardChange);
    void updateKeyboardFocus();

    KCardScene *const q;

    KAbstractCardDeck *deck;
    QList<KCardPile *> piles;
    QHash<const KCardPile *, QRectF> pileAreas;
    QSet<QGraphicsItem *> highlightedItems;

    QList<KCard *> cardsBeingDragged;
    QPointF startOfDrag;
    bool dragStarted;

    KCardScene::SceneAlignment alignment;
    qreal layoutMargin;
    qreal layoutSpacing;
    QSizeF contentSize;

    bool keyboardMode;
    int keyboardPileIndex;
    int keyboardCardIndex;
    QPointer<QObject> keyboardFocusItem;

    bool sizeHasBeenSet;
};

KCardScenePrivate::KCardScenePrivate(KCardScene *p)
    : QObject(p)
    , q(p)
{
    dragStarted = false;
}

KCardPile *KCardScenePrivate::bestDestinationPileUnderCards()
{
    QSet<KCardPile *> targets;
    const auto items = q->collidingItems(cardsBeingDragged.first(), Qt::IntersectsItemBoundingRect);
    for (QGraphicsItem *item : items) {
        KCardPile *p = qgraphicsitem_cast<KCardPile *>(item);
        if (!p) {
            KCard *c = qgraphicsitem_cast<KCard *>(item);
            if (c)
                p = c->pile();
        }
        if (p)
            targets << p;
    }

    KCardPile *bestTarget = nullptr;
    qreal bestArea = 1;

    for (KCardPile *p : std::as_const(targets)) {
        if (p != cardsBeingDragged.first()->pile() && q->allowedToAdd(p, cardsBeingDragged)) {
            QRectF targetRect = p->sceneBoundingRect();
            const auto cards = p->cards();
            for (KCard *c : cards)
                targetRect |= c->sceneBoundingRect();

            QRectF intersection = targetRect & cardsBeingDragged.first()->sceneBoundingRect();
            qreal area = intersection.width() * intersection.height();
            if (area > bestArea) {
                bestTarget = p;
                bestArea = area;
            }
        }
    }

    return bestTarget;
}

void KCardScenePrivate::sendCardsToPile(KCardPile *pile, QList<KCard *> newCards, qreal rate, bool isSpeed, bool flip)
{
    if (pile->isEmpty() && newCards.isEmpty())
        return;

    int oldCardCount = pile->count();

    for (int i = 0; i < newCards.size(); ++i) {
        // If we're flipping the cards, we have to add them to the pile in
        // reverse order.
        KCard *c = newCards[flip ? newCards.size() - 1 - i : i];

        // The layout of the card within the pile may depend on whether it is
        // face up or down. Therefore, we must flip the card, add it to the
        // pile, calculate its position, flip it back, then start the animation
        // which will flip it one more time.
        if (flip)
            c->setFaceUp(!c->isFaceUp());

        pile->add(c);
    }

    const QSize cardSize = deck->cardSize();
    const qreal cardUnit = (deck->cardWidth() + deck->cardHeight()) / 2.0;
    const QList<KCard *> cards = pile->cards();
    const QList<QPointF> positions = pile->cardPositions();

    qreal minX = 0;
    qreal maxX = 0;
    qreal minY = 0;
    qreal maxY = 0;
    for (const QPointF &pos : positions) {
        minX = qMin(minX, pos.x());
        maxX = qMax(maxX, pos.x());
        minY = qMin(minY, pos.y());
        maxY = qMax(maxY, pos.y());
    }

    QPointF absLayoutPos = pile->layoutPos();
    if (absLayoutPos.x() < 0)
        absLayoutPos.rx() += contentSize.width() / cardSize.width() - 1;
    if (absLayoutPos.y() < 0)
        absLayoutPos.ry() += contentSize.height() / cardSize.height() - 1;

    QRectF available = pileAreas.value(pile, QRectF());
    qreal availableTop = absLayoutPos.y() - available.top();
    qreal availableBottom = available.bottom() - (absLayoutPos.y() + 1);
    qreal availableLeft = absLayoutPos.x() - available.left();
    qreal availableRight = available.right() - (absLayoutPos.x() + 1);

    qreal scaleTop = (minY >= 0) ? 1 : qMin<qreal>(availableTop / -minY, 1);
    qreal scaleBottom = (maxY <= 0) ? 1 : qMin<qreal>(availableBottom / maxY, 1);
    qreal scaleY = qMin(scaleTop, scaleBottom);

    qreal scaleLeft = (minX >= 0) ? 1 : qMin<qreal>(availableLeft / -minX, 1);
    qreal scaleRight = (maxX <= 0) ? 1 : qMin<qreal>(availableRight / maxX, 1);
    qreal scaleX = qMin(scaleLeft, scaleRight);

    QList<QPointF> realPositions;
    QList<qreal> distances;
    qreal maxDistance = 0;
    for (int i = 0; i < cards.size(); ++i) {
        QPointF pos(pile->x() + positions[i].x() * scaleX * cardSize.width(), pile->y() + positions[i].y() * scaleY * cardSize.height());
        realPositions << pos;

        qreal distance = 0;
        if (isSpeed && i >= oldCardCount) {
            QPointF delta = pos - cards[i]->pos();
            distance = sqrt(delta.x() * delta.x() + delta.y() * delta.y()) / cardUnit;
            if (distance > maxDistance)
                maxDistance = distance;
        }
        distances << distance;
    }

    qreal z = pile->zValue();
    int layoutDuration = isSpeed ? qMin<int>(cardMoveDuration, maxDistance / rate * 1000) : rate;

    for (int i = 0; i < cards.size(); ++i) {
        bool isNewCard = i >= oldCardCount;

        int duration = (isNewCard && isSpeed) ? distances[i] / rate * 1000 : layoutDuration;

        // Honour the pile's autoTurnTop property.
        bool face = cards[i]->isFaceUp() || (cards[i] == pile->topCard() && pile->autoTurnTop());

        if (isNewCard && flip) {
            // The card will be flipped as part of the animation, so we return
            // it to its original face up/down position before starting the
            // animation.
            cards[i]->setFaceUp(!cards[i]->isFaceUp());
        }

        // Each card has a Z value 1 greater than the card below it.
        ++z;

        cards[i]->animate(realPositions[i], z, 0, face, isNewCard, duration);
    }
}

void KCardScenePrivate::changeFocus(int pileChange, int cardChange)
{
    if (!keyboardMode) {
        q->setKeyboardModeActive(true);
        return;
    }

    if (pileChange) {
        KCardPile *pile;
        KCardPile::KeyboardFocusHint hint;
        do {
            keyboardPileIndex += pileChange;
            if (keyboardPileIndex < 0)
                keyboardPileIndex = piles.size() - 1;
            else if (keyboardPileIndex >= piles.size())
                keyboardPileIndex = 0;

            pile = piles.at(keyboardPileIndex);
            hint = cardsBeingDragged.isEmpty() ? pile->keyboardSelectHint() : pile->keyboardDropHint();
        } while (hint == KCardPile::NeverFocus);

        if (!pile->isEmpty()) {
            if (hint == KCardPile::AutoFocusTop || hint == KCardPile::ForceFocusTop) {
                keyboardCardIndex = pile->count() - 1;
            } else if (hint == KCardPile::AutoFocusDeepestRemovable) {
                keyboardCardIndex = pile->count() - 1;
                while (keyboardCardIndex > 0 && q->allowedToRemove(pile, pile->at(keyboardCardIndex - 1)))
                    --keyboardCardIndex;
            } else if (hint == KCardPile::AutoFocusDeepestFaceUp) {
                keyboardCardIndex = pile->count() - 1;
                while (keyboardCardIndex > 0 && pile->at(keyboardCardIndex - 1)->isFaceUp())
                    --keyboardCardIndex;
            } else if (hint == KCardPile::AutoFocusBottom) {
                keyboardCardIndex = 0;
            }
        }
    }

    if (cardChange) {
        KCardPile *pile = piles.at(keyboardPileIndex);
        if (cardChange < 0 && keyboardCardIndex >= pile->count()) {
            keyboardCardIndex = qMax(0, pile->count() - 2);
        } else {
            keyboardCardIndex += cardChange;
            if (keyboardCardIndex < 0)
                keyboardCardIndex = pile->count() - 1;
            else if (keyboardCardIndex >= pile->count())
                keyboardCardIndex = 0;
        }
    }

    updateKeyboardFocus();
}

void KCardScenePrivate::updateKeyboardFocus()
{
    setItemHighlight(toGraphicsItem(keyboardFocusItem), false);

    if (!keyboardMode) {
        keyboardFocusItem.clear();
        keyboardPileIndex = 0;
        keyboardCardIndex = 0;
        return;
    }

    KCardPile *pile = piles.at(keyboardPileIndex);
    KCardPile::KeyboardFocusHint hint = cardsBeingDragged.isEmpty() ? pile->keyboardSelectHint() : pile->keyboardDropHint();

    if (!cardsBeingDragged.isEmpty() && cardsBeingDragged.first()->pile() == pile) {
        int index = pile->indexOf(cardsBeingDragged.first());
        if (index == 0)
            keyboardFocusItem = pile;
        else
            keyboardFocusItem = pile->at(index - 1);
    } else if (pile->isEmpty()) {
        keyboardFocusItem = pile;
    } else if (keyboardCardIndex >= pile->count() || hint == KCardPile::ForceFocusTop) {
        keyboardFocusItem = pile->topCard();
    } else {
        keyboardFocusItem = pile->at(keyboardCardIndex);
    }

    QGraphicsItem *focusItem = toGraphicsItem(keyboardFocusItem);
    Q_ASSERT(focusItem);

    setItemHighlight(focusItem, true);

    QPointF delta = focusItem->pos() - startOfDrag;
    startOfDrag = focusItem->pos();
    for (KCard *c : std::as_const(cardsBeingDragged))
        c->setPos(c->pos() + delta);
}

KCardScene::KCardScene(QObject *parent)
    : QGraphicsScene(parent)
    , d(new KCardScenePrivate(this))
{
    d->deck = nullptr;
    d->alignment = AlignHCenter | AlignHSpread;
    d->layoutMargin = 0.15;
    d->layoutSpacing = 0.15;
    d->keyboardMode = false;
    d->keyboardPileIndex = 0;
    d->keyboardCardIndex = 0;
    d->keyboardFocusItem.clear();
    d->sizeHasBeenSet = false;
}

KCardScene::~KCardScene()
{
    const auto currentPiles = d->piles;
    for (KCardPile *p : currentPiles) {
        removePile(p);
        delete p;
    }
    Q_ASSERT(d->piles.isEmpty());
    d->piles.clear();
}

void KCardScene::setDeck(KAbstractCardDeck *deck)
{
    if (d->deck)
        disconnect(d->deck, &KAbstractCardDeck::cardAnimationDone, this, &KCardScene::cardAnimationDone);

    d->deck = deck;

    if (d->deck)
        connect(d->deck, &KAbstractCardDeck::cardAnimationDone, this, &KCardScene::cardAnimationDone);
}

KAbstractCardDeck *KCardScene::deck() const
{
    return d->deck;
}

bool KCardScene::isCardAnimationRunning() const
{
    return d->deck && d->deck->hasAnimatedCards();
}

QList<KCard *> KCardScene::cardsBeingDragged() const
{
    return d->cardsBeingDragged;
}

void KCardScene::addPile(KCardPile *pile)
{
    KCardScene *origScene = dynamic_cast<KCardScene *>(pile->scene());
    if (origScene)
        origScene->removePile(pile);

    addItem(pile);
    const auto cards = pile->cards();
    for (KCard *c : cards)
        addItem(c);
    d->piles.append(pile);
}

void KCardScene::removePile(KCardPile *pile)
{
    const auto cards = pile->cards();
    for (KCard *c : cards)
        removeItem(c);
    removeItem(pile);
    d->piles.removeAll(pile);
}

QList<KCardPile *> KCardScene::piles() const
{
    return d->piles;
}

void KCardScene::setSceneAlignment(KCardScene::SceneAlignment alignment)
{
    if (alignment != d->alignment) {
        d->alignment = alignment;
        relayoutScene();
    }
}

KCardScene::SceneAlignment KCardScene::sceneAlignment() const
{
    return d->alignment;
}

void KCardScene::setLayoutMargin(qreal margin)
{
    if (margin != d->layoutMargin) {
        d->layoutMargin = margin;
        relayoutScene();
    }
}

qreal KCardScene::layoutMargin() const
{
    return d->layoutMargin;
}

void KCardScene::setLayoutSpacing(qreal spacing)
{
    if (spacing != d->layoutSpacing) {
        d->layoutSpacing = spacing;
        relayoutScene();
    }
}

qreal KCardScene::layoutSpacing() const
{
    return d->layoutSpacing;
}

QRectF KCardScene::contentArea() const
{
    return QRectF(QPoint(0, 0), d->contentSize);
}

void KCardScene::resizeScene(const QSize &size)
{
    d->sizeHasBeenSet = true;
    setSceneRect(QRectF(sceneRect().topLeft(), size));
    relayoutScene();
}

void KCardScene::relayoutScene()
{
    if (!d->sizeHasBeenSet || !d->deck)
        return;

    qreal usedWidth = 1;
    qreal usedHeight = 1;
    qreal extraWidth = 0;
    qreal extraHeight = 0;
    const auto piles = this->piles();
    for (const KCardPile *p : piles) {
        if (p->layoutPos().x() >= 0)
            usedWidth = qMax(usedWidth, p->layoutPos().x() + 1 + p->rightPadding());
        else
            extraWidth = qMax(extraWidth, p->leftPadding() + 1 + p->rightPadding());

        if (p->layoutPos().y() >= 0)
            usedHeight = qMax(usedHeight, p->layoutPos().y() + 1 + p->bottomPadding());
        else
            extraHeight = qMax(extraHeight, p->topPadding() + 1 + p->bottomPadding());
    }
    if (extraWidth) {
        qreal hSpacing = d->layoutSpacing * (1 + d->deck->cardHeight() / d->deck->cardWidth()) / 2;
        usedWidth += hSpacing + extraWidth;
    }
    if (extraHeight) {
        qreal vSpacing = d->layoutSpacing * (1 + d->deck->cardWidth() / d->deck->cardHeight()) / 2;
        usedHeight += vSpacing + extraHeight;
    }

    // Add the border to the size of the contents
    QSizeF sizeToFit(usedWidth + 2 * d->layoutMargin, usedHeight + 2 * d->layoutMargin);

    qreal scaleX = width() / (d->deck->cardWidth() * sizeToFit.width());
    qreal scaleY = height() / (d->deck->cardHeight() * sizeToFit.height());
    qreal n_scaleFactor = qMin(scaleX, scaleY);

    d->deck->setCardWidth(n_scaleFactor * d->deck->cardWidth());

    int usedPixelWidth = usedWidth * d->deck->cardWidth();
    int usedPixelHeight = usedHeight * d->deck->cardHeight();
    int pixelHMargin = d->layoutMargin * d->deck->cardWidth();
    int pixelVMargin = d->layoutMargin * d->deck->cardHeight();

    int contentWidth;
    int xOffset;
    if (d->alignment & AlignLeft) {
        contentWidth = usedPixelWidth;
        xOffset = pixelHMargin;
    } else if (d->alignment & AlignRight) {
        contentWidth = usedPixelWidth;
        xOffset = width() - contentWidth - pixelHMargin;
    } else if (d->alignment & AlignHCenter) {
        contentWidth = usedPixelWidth;
        xOffset = (width() - contentWidth) / 2;
    } else {
        contentWidth = width() - 2 * d->layoutMargin * d->deck->cardWidth();
        xOffset = pixelHMargin;
    }

    int contentHeight;
    int yOffset;
    if (d->alignment & AlignTop) {
        contentHeight = usedPixelHeight;
        yOffset = pixelVMargin;
    } else if (d->alignment & AlignBottom) {
        contentHeight = usedPixelHeight;
        yOffset = height() - contentHeight - pixelVMargin;
    } else if (d->alignment & AlignVCenter) {
        contentHeight = usedPixelHeight;
        yOffset = (height() - contentHeight) / 2;
    } else {
        contentHeight = height() - 2 * d->layoutMargin * d->deck->cardHeight();
        yOffset = pixelVMargin;
    }

    d->contentSize = QSizeF(contentWidth, contentHeight);
    setSceneRect(-xOffset, -yOffset, width(), height());

    recalculatePileLayouts();
    for (KCardPile *p : piles)
        updatePileLayout(p, 0);
}

void KCardScene::recalculatePileLayouts()
{
    if (!d->sizeHasBeenSet || !d->deck)
        return;

    QSize cardSize = d->deck->cardSize();
    const qreal contentWidth = d->contentSize.width() / cardSize.width();
    const qreal contentHeight = d->contentSize.height() / cardSize.height();
    const qreal spacing = d->layoutSpacing;

    QList<KCardPile *> visiblePiles;
    QHash<KCardPile *, QRectF> reserve;
    QHash<const KCardPile *, QRectF> &areas = d->pileAreas;
    areas.clear();
    const auto piles = this->piles();
    for (KCardPile *p : piles) {
        QPointF layoutPos = p->layoutPos();
        if (layoutPos.x() < 0)
            layoutPos.rx() += contentWidth - 1;
        if (layoutPos.y() < 0)
            layoutPos.ry() += contentHeight - 1;

        p->setPos(layoutPos.x() * cardSize.width(), layoutPos.y() * cardSize.height());
        p->setGraphicSize(cardSize);

        if (p->isVisible()) {
            visiblePiles << p;

            reserve[p] = QRectF(layoutPos, QSize(1, 1)).adjusted(-p->leftPadding(), -p->topPadding(), p->rightPadding(), p->bottomPadding());
            areas[p] = reserve[p];
        }
    }

    // Grow piles down
    for (KCardPile *p1 : std::as_const(visiblePiles)) {
        if (p1->heightPolicy() == KCardPile::GrowDown) {
            areas[p1].setBottom(contentHeight);
            for (KCardPile *p2 : std::as_const(visiblePiles)) {
                if (p2 != p1 && areas[p1].intersects(areas[p2])) {
                    if (p2->heightPolicy() == KCardPile::GrowUp)
                        areas[p1].setBottom((reserve[p1].bottom() + reserve[p2].top() - spacing) / 2);
                    else
                        areas[p1].setBottom(reserve[p2].top() - spacing);
                }
            }
        }
    }

    // Grow piles up
    for (KCardPile *p1 : std::as_const(visiblePiles)) {
        if (p1->heightPolicy() == KCardPile::GrowUp) {
            areas[p1].setTop(0);
            for (KCardPile *p2 : std::as_const(visiblePiles)) {
                if (p2 != p1 && areas[p1].intersects(areas[p2])) {
                    if (p2->heightPolicy() == KCardPile::GrowDown)
                        areas[p1].setTop((reserve[p1].top() + reserve[p2].bottom() + spacing) / 2);
                    else
                        areas[p1].setTop(reserve[p2].bottom() + spacing);
                }
            }
        }
    }

    // Grow piles right
    for (KCardPile *p1 : std::as_const(visiblePiles)) {
        if (p1->widthPolicy() == KCardPile::GrowRight) {
            areas[p1].setRight(contentWidth);
            for (KCardPile *p2 : std::as_const(visiblePiles)) {
                if (p2 != p1 && areas[p1].intersects(areas[p2])) {
                    if (p2->widthPolicy() == KCardPile::GrowLeft)
                        areas[p1].setRight((reserve[p1].right() + reserve[p2].left() - spacing) / 2);
                    else
                        areas[p1].setRight(reserve[p2].left() - spacing);
                }
            }
        }
    }

    // Grow piles left
    for (KCardPile *p1 : std::as_const(visiblePiles)) {
        if (p1->widthPolicy() == KCardPile::GrowLeft) {
            areas[p1].setLeft(0);
            for (KCardPile *p2 : std::as_const(visiblePiles)) {
                if (p2 != p1 && areas[p1].intersects(areas[p2])) {
                    if (p2->widthPolicy() == KCardPile::GrowRight)
                        areas[p1].setLeft((reserve[p1].left() + reserve[p2].right() + spacing) / 2);
                    else
                        areas[p1].setLeft(reserve[p2].right() + spacing);
                }
            }
        }
    }
}

void KCardScene::setHighlightedItems(const QList<QGraphicsItem *> &items)
{
    const QSet<QGraphicsItem *> s = QSet<QGraphicsItem *>(items.cbegin(), items.cend());
    const auto unhighlightedItems = d->highlightedItems.subtract(s);
    for (QGraphicsItem *i : unhighlightedItems)
        setItemHighlight(i, false);
    for (QGraphicsItem *i : s)
        setItemHighlight(i, true);
    d->highlightedItems = s;
}

void KCardScene::clearHighlightedItems()
{
    for (QGraphicsItem *i : std::as_const(d->highlightedItems))
        setItemHighlight(i, false);
    d->highlightedItems.clear();
}

QList<QGraphicsItem *> KCardScene::highlightedItems() const
{
    return d->highlightedItems.values();
}

void KCardScene::moveCardsToPile(const QList<KCard *> &cards, KCardPile *pile, int duration)
{
    if (cards.isEmpty())
        return;

    KCardPile *source = cards.first()->pile();
    d->sendCardsToPile(pile, cards, duration, false, false);
    if (source)
        d->sendCardsToPile(source, QList<KCard *>(), duration, false, false);
    cardsMoved(cards, source, pile);
}

void KCardScene::moveCardToPile(KCard *card, KCardPile *pile, int duration)
{
    moveCardsToPile(QList<KCard *>() << card, pile, duration);
}

void KCardScene::moveCardsToPileAtSpeed(const QList<KCard *> &cards, KCardPile *pile, qreal velocity)
{
    if (cards.isEmpty())
        return;

    KCardPile *source = cards.first()->pile();
    d->sendCardsToPile(pile, cards, velocity, true, false);
    if (source)
        d->sendCardsToPile(source, QList<KCard *>(), cardMoveDuration, false, false);
    cardsMoved(cards, source, pile);
}

void KCardScene::moveCardToPileAtSpeed(KCard *card, KCardPile *pile, qreal velocity)
{
    moveCardsToPileAtSpeed(QList<KCard *>() << card, pile, velocity);
}

void KCardScene::flipCardsToPile(const QList<KCard *> &cards, KCardPile *pile, int duration)
{
    if (cards.isEmpty())
        return;

    KCardPile *source = cards.first()->pile();
    d->sendCardsToPile(pile, cards, duration, false, true);
    if (source)
        d->sendCardsToPile(source, QList<KCard *>(), duration, false, false);
    cardsMoved(cards, source, pile);
}

void KCardScene::flipCardToPile(KCard *card, KCardPile *pile, int duration)
{
    flipCardsToPile(QList<KCard *>() << card, pile, duration);
}

void KCardScene::flipCardsToPileAtSpeed(const QList<KCard *> &cards, KCardPile *pile, qreal velocity)
{
    if (cards.isEmpty())
        return;

    KCardPile *source = cards.first()->pile();
    d->sendCardsToPile(pile, cards, velocity, true, true);
    if (source)
        d->sendCardsToPile(source, QList<KCard *>(), cardMoveDuration, false, false);
    cardsMoved(cards, source, pile);
}

void KCardScene::flipCardToPileAtSpeed(KCard *card, KCardPile *pile, qreal velocity)
{
    flipCardsToPileAtSpeed(QList<KCard *>() << card, pile, velocity);
}

void KCardScene::keyboardFocusLeft()
{
    d->changeFocus(-1, 0);
}

void KCardScene::keyboardFocusRight()
{
    d->changeFocus(+1, 0);
}

void KCardScene::keyboardFocusUp()
{
    d->changeFocus(0, -1);
}

void KCardScene::keyboardFocusDown()
{
    d->changeFocus(0, +1);
}

void KCardScene::keyboardFocusCancel()
{
    setKeyboardModeActive(false);
}

void KCardScene::keyboardFocusSelect()
{
    if (!isKeyboardModeActive()) {
        setKeyboardModeActive(true);
        return;
    }

    if (d->cardsBeingDragged.isEmpty()) {
        KCardPile *pile = d->piles.at(d->keyboardPileIndex);
        if (pile->isEmpty())
            return;

        if (d->keyboardCardIndex >= pile->count())
            d->keyboardCardIndex = pile->count() - 1;

        KCard *card = pile->at(d->keyboardCardIndex);
        d->cardsBeingDragged = card->pile()->topCardsDownTo(card);
        if (allowedToRemove(card->pile(), d->cardsBeingDragged.first())) {
            d->startOfDrag = d->keyboardCardIndex > 0 ? pile->at(d->keyboardCardIndex - 1)->pos() : pile->pos();

            QPointF offset = d->startOfDrag - card->pos() + QPointF(d->deck->cardWidth(), d->deck->cardHeight()) / 10.0;
            for (KCard *c : std::as_const(d->cardsBeingDragged)) {
                c->stopAnimation();
                c->raise();
                c->setPos(c->pos() + offset);
            }
            d->dragStarted = true;
            d->updateKeyboardFocus();
        } else {
            d->cardsBeingDragged.clear();
        }
    } else {
        KCardPile *destination = d->bestDestinationPileUnderCards();
        if (destination)
            cardsDroppedOnPile(d->cardsBeingDragged, destination);
        else
            updatePileLayout(d->cardsBeingDragged.first()->pile(), cardMoveDuration);

        QGraphicsItem *toFocus = d->cardsBeingDragged.first();
        d->cardsBeingDragged.clear();
        d->dragStarted = false;
        setKeyboardFocus(toFocus);
    }
}

void KCardScene::setKeyboardFocus(QGraphicsItem *item)
{
    KCard *c = qgraphicsitem_cast<KCard *>(item);
    if (c && c->pile()) {
        KCardPile *p = c->pile();
        d->keyboardPileIndex = d->piles.indexOf(p);
        d->keyboardCardIndex = p->indexOf(c);
    } else {
        KCardPile *p = qgraphicsitem_cast<KCardPile *>(item);
        if (p) {
            d->keyboardPileIndex = d->piles.indexOf(p);
            d->keyboardCardIndex = 0;
        }
    }
    d->updateKeyboardFocus();
}

void KCardScene::setKeyboardModeActive(bool keyboardMode)
{
    if (!d->keyboardMode && keyboardMode) {
        clearHighlightedItems();
        d->keyboardMode = true;
        d->updateKeyboardFocus();
    } else if (d->keyboardMode && !keyboardMode) {
        if (!d->cardsBeingDragged.isEmpty())
            updatePileLayout(d->cardsBeingDragged.first()->pile(), cardMoveDuration);
        d->cardsBeingDragged.clear();

        d->keyboardMode = false;
        d->updateKeyboardFocus();
    }
}

bool KCardScene::isKeyboardModeActive() const
{
    return d->keyboardMode;
}

void KCardScene::updatePileLayout(KCardPile *pile, int duration)
{
    d->sendCardsToPile(pile, QList<KCard *>(), duration, false, false);
}

bool KCardScene::allowedToAdd(const KCardPile *pile, const QList<KCard *> &cards) const
{
    Q_UNUSED(pile)
    Q_UNUSED(cards)
    return true;
}

bool KCardScene::allowedToRemove(const KCardPile *pile, const KCard *card) const
{
    Q_UNUSED(pile)
    Q_UNUSED(card)
    return true;
}

void KCardScene::cardsDroppedOnPile(const QList<KCard *> &cards, KCardPile *pile)
{
    moveCardsToPile(cards, pile, cardMoveDuration);
}

void KCardScene::cardsMoved(const QList<KCard *> &cards, KCardPile *oldPile, KCardPile *newPile)
{
    Q_UNUSED(cards);
    Q_UNUSED(oldPile);
    Q_UNUSED(newPile);
}

void KCardScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (isKeyboardModeActive())
        setKeyboardModeActive(false);

    const QList<QGraphicsItem *> itemsAtPoint = items(e->scenePos());
    QGraphicsItem *item = itemsAtPoint.isEmpty() ? nullptr : itemsAtPoint.first();
    KCard *card = qgraphicsitem_cast<KCard *>(item);
    KCardPile *pile = qgraphicsitem_cast<KCardPile *>(item);

    if (e->button() == Qt::LeftButton && (card || pile)) {
        e->accept();

        if (card && !isCardAnimationRunning() && !d->cardsBeingDragged.contains(card)) {
            QList<KCard *> cards = card->pile()->topCardsDownTo(card);

            if (allowedToRemove(card->pile(), cards.first())) {
                d->cardsBeingDragged = cards;
                for (KCard *c : std::as_const(d->cardsBeingDragged)) {
                    c->stopAnimation();
                    c->raise();
                }
            }

            d->dragStarted = false;
            d->startOfDrag = e->scenePos();
        }
    } else {
        QGraphicsScene::mousePressEvent(e);
    }
}

void KCardScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
    if (!d->cardsBeingDragged.isEmpty()) {
        e->accept();

        if (!d->dragStarted) {
            bool overCard = d->cardsBeingDragged.first()->sceneBoundingRect().contains(e->scenePos());
            QPointF delta = e->scenePos() - d->startOfDrag;
            qreal distanceSquared = delta.x() * delta.x() + delta.y() * delta.y();
            // Ignore the move event until we've moved at least 4 pixels or the
            // cursor leaves the card.
            if (distanceSquared > 16.0 || !overCard) {
                d->dragStarted = true;
                // If the cursor hasn't left the card, then continue the drag from
                // the current position, which makes it look smoother.
                if (overCard)
                    d->startOfDrag = e->scenePos();
            }
        }

        if (d->dragStarted) {
            for (KCard *card : std::as_const(d->cardsBeingDragged))
                card->setPos(card->pos() + e->scenePos() - d->startOfDrag);
            d->startOfDrag = e->scenePos();

            QList<QGraphicsItem *> toHighlight;
            KCardPile *dropPile = d->bestDestinationPileUnderCards();
            if (dropPile) {
                if (dropPile->isEmpty())
                    toHighlight << dropPile;
                else
                    toHighlight << dropPile->topCard();
            }

            setHighlightedItems(toHighlight);
        }
    } else {
        QGraphicsScene::mouseMoveEvent(e);
    }
}

void KCardScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    const QList<QGraphicsItem *> itemsAtPoint = items(e->scenePos());
    QGraphicsItem *topItem = itemsAtPoint.isEmpty() ? nullptr : itemsAtPoint.first();
    KCard *card = qgraphicsitem_cast<KCard *>(topItem);
    KCardPile *pile = qgraphicsitem_cast<KCardPile *>(topItem);

    if (e->button() == Qt::LeftButton && !d->dragStarted && !d->cardsBeingDragged.isEmpty()) {
        updatePileLayout(d->cardsBeingDragged.first()->pile(), cardMoveDuration);
        d->cardsBeingDragged.clear();
    }

    if (e->button() == Qt::LeftButton && !d->cardsBeingDragged.isEmpty()) {
        e->accept();

        KCardPile *destination = d->bestDestinationPileUnderCards();
        if (destination)
            cardsDroppedOnPile(d->cardsBeingDragged, destination);
        else
            updatePileLayout(d->cardsBeingDragged.first()->pile(), cardMoveDuration);
        d->cardsBeingDragged.clear();
        d->dragStarted = false;
        clearHighlightedItems();
    } else if (card && !isCardAnimationRunning()) {
        e->accept();
        if (e->button() == Qt::LeftButton) {
            Q_EMIT cardClicked(card);
            if (card->pile())
                Q_EMIT card->pile()->clicked(card);
        } else if (e->button() == Qt::RightButton) {
            Q_EMIT cardRightClicked(card);
            if (card->pile())
                Q_EMIT card->pile()->rightClicked(card);
        }
    } else if (pile && !isCardAnimationRunning()) {
        e->accept();
        if (e->button() == Qt::LeftButton) {
            Q_EMIT pileClicked(pile);
            Q_EMIT pile->clicked(nullptr);
        } else if (e->button() == Qt::RightButton) {
            Q_EMIT pileRightClicked(pile);
            Q_EMIT pile->rightClicked(nullptr);
        }
    } else {
        QGraphicsScene::mouseReleaseEvent(e);
    }
}

void KCardScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)
{
    const QList<QGraphicsItem *> itemsAtPoint = items(e->scenePos());
    QGraphicsItem *topItem = itemsAtPoint.isEmpty() ? nullptr : itemsAtPoint.first();
    KCard *card = qgraphicsitem_cast<KCard *>(topItem);
    KCardPile *pile = qgraphicsitem_cast<KCardPile *>(topItem);

    if (!d->cardsBeingDragged.isEmpty()) {
        updatePileLayout(d->cardsBeingDragged.first()->pile(), cardMoveDuration);
        d->cardsBeingDragged.clear();
    }

    if (card && e->button() == Qt::LeftButton && !isCardAnimationRunning()) {
        e->accept();
        Q_EMIT cardDoubleClicked(card);
        if (card->pile())
            Q_EMIT card->pile()->doubleClicked(card);
    } else if (pile && e->button() == Qt::LeftButton && !isCardAnimationRunning()) {
        e->accept();
        Q_EMIT pileDoubleClicked(pile);
        Q_EMIT pile->doubleClicked(nullptr);
    } else {
        QGraphicsScene::mouseDoubleClickEvent(e);
    }
}

void KCardScene::wheelEvent(QGraphicsSceneWheelEvent *e)
{
    if (d->deck && e->modifiers() & Qt::ControlModifier) {
        e->accept();

        qreal scaleFactor = pow(2, e->delta() / qreal(10 * 120));
        int newWidth = d->deck->cardWidth() * scaleFactor;
        d->deck->setCardWidth(newWidth);
        recalculatePileLayouts();
        const auto piles = this->piles();
        for (KCardPile *p : piles)
            updatePileLayout(p, 0);
    } else {
        QGraphicsScene::wheelEvent(e);
    }
}

void KCardScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(painter)
    Q_UNUSED(rect)
}

#include "moc_kcardscene.cpp"
