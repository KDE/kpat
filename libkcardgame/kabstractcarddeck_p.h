/*
 *  Copyright (C) 2009-2010 Parker Coates <parker.coates@kdemail.org>
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

#include "kabstractcarddeck.h"

#include "kcardcache.h"

#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QSizeF>
#include <QtCore/QStringList>


class KAbstractCardDeckPrivate : public QObject
{
    Q_OBJECT

public:
    KAbstractCardDeckPrivate( KAbstractCardDeck * q );

public slots:
    void cardStartedAnimation( KCard * card );
    void cardStoppedAnimation( KCard * card );
    QPixmap requestPixmap( QString elementId, bool immediate );
    void pixmapUpdated( const QString & element, const QPixmap & pix );

public:
    KAbstractCardDeck * q;

    KCardCache2 * cache;
    QSizeF originalCardSize;
    QSize currentCardSize;

    QList<KCard*> cards;
    QSet<KCard*> cardsWaitedFor;

    QStringList elementIds;
    QHash<QString,QPair<QPixmap,QList<KCard*> > > elementIdMapping;
};
