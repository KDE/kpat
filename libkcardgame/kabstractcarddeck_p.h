/*
 *  Copyright (C) 2009-2010 Parker Coates <coates@kde.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of 
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef KABSTRACTCARDDECK_P_H
#define KABSTRACTCARDDECK_P_H

#include "kabstractcarddeck.h"

#include "kcardtheme.h"
#include <KImageCache>

#include <QtCore/QHash>
class QImage;
#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtCore/QSizeF>
#include <QtCore/QStringList>
#include <QtCore/QThread>
#include <QPixmap>
class QSvgRenderer;


class RenderingThread : public QThread
{
    Q_OBJECT

public:
    RenderingThread( KAbstractCardDeckPrivate * d, QSize size, const QStringList & elements );
    void run();
    void halt();

Q_SIGNALS:
    void renderingDone( const QString & elementId, const QImage & image );

private:
    KAbstractCardDeckPrivate * const d;
    const QSize m_size;
    const QStringList m_elementsToRender;
    bool m_haltFlag;
    QMutex m_haltMutex;
};


struct CardElementData
{
    QPixmap cardPixmap;
    QList<KCard*> cardUsers;
};


class KAbstractCardDeckPrivate : public QObject
{
    Q_OBJECT

public:
    KAbstractCardDeckPrivate( KAbstractCardDeck * q );
    ~KAbstractCardDeckPrivate();

    QSvgRenderer * renderer();
    QImage renderCard( const QString & element, const QSize & size );
    QSizeF unscaledCardSize();
    QPixmap requestPixmap( quint32 id, bool faceUp );
    void updateCardSize( const QSize & size );
    void deleteThread();

public Q_SLOTS:
    void submitRendering( const QString & elementId, const QImage & image );
    void cardStartedAnimation( KCard * card );
    void cardStoppedAnimation( KCard * card );
    void checkIfAnimationIsDone();


public:
    KAbstractCardDeck * q;

    QSizeF originalCardSize;
    QSize currentCardSize;

    QList<KCard*> cards;
    QSet<KCard*> cardsWaitedFor;
    QTimer * animationCheckTimer;

    KCardTheme theme;
    KImageCache * cache;
    QSvgRenderer * svgRenderer;
    QMutex rendererMutex;
    RenderingThread * thread;

    QHash<QString,CardElementData> frontIndex;
    QHash<QString,CardElementData> backIndex;
};

#endif
