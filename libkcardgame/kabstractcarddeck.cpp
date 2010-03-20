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

#include "kabstractcarddeck_p.h"

#include "kcardtheme.h"
#include "kcardpile.h"
#include "shuffle.h"

#include <KDebug>

#include <QtCore/QTimer>
#include <QtGui/QPainter>


KAbstractCardDeckPrivate::KAbstractCardDeckPrivate( KAbstractCardDeck * q )
  : q( q )
{
};


void KAbstractCardDeckPrivate::cardStartedAnimation( KCard * card )
{
    Q_ASSERT( !cardsWaitedFor.contains( card ) );
    cardsWaitedFor.insert( card );
};


void KAbstractCardDeckPrivate::cardStoppedAnimation( KCard * card )
{
    Q_ASSERT( cardsWaitedFor.contains( card ) );
    cardsWaitedFor.remove( card );

    if ( cardsWaitedFor.isEmpty() )
        emit q->cardAnimationDone();
};


QPixmap KAbstractCardDeckPrivate::requestPixmap( QString elementId, bool immediate )
{
    QPixmap & stored = elementIdMapping[ elementId ].first;
    if ( stored.size() != currentCardSize )
    {
        if ( immediate || stored.isNull() )
        {
            stored = cache->renderCard( elementId );
        }
        else 
        {
            QPixmap pix = cache->renderCardIfCached( elementId );
            if ( !pix.isNull() )
                stored = pix;
            else
                stored = stored.scaled( currentCardSize );
        }
    }
    return stored;
}


void KAbstractCardDeckPrivate::pixmapUpdated( const QString & element, const QPixmap & pix )
{
    QPair<QPixmap,QList<KCard*> > & pair = elementIdMapping[ element ];
    pair.first = pix;
    foreach ( KCard * c, pair.second )
        c->update();
}


KAbstractCardDeck::KAbstractCardDeck( const KCardTheme & theme, QObject * parent )
  : QObject( parent ),
    d( new KAbstractCardDeckPrivate( this ) )
{
    d->cache = 0;
    setTheme( theme );
}


KAbstractCardDeck::~KAbstractCardDeck()
{
    foreach ( KCard * c, d->cards )
        delete c;
    d->cards.clear();

    delete d;
}


void KAbstractCardDeck::setDeckContents( QList<quint32> ids )
{
    foreach ( KCard * c, d->cards )
        delete c;
    d->cards.clear();
    d->cardsWaitedFor.clear();

    QHash<QString,QPair<QPixmap,QList<KCard*> > > oldMapping = d->elementIdMapping;
    d->elementIdMapping.clear();

    foreach ( quint32 id, ids )
    {
        KCard * c = new KCard( id, this );

        c->setObjectName( elementName( c->id() ) );

        connect( c, SIGNAL(animationStarted(KCard*)), d, SLOT(cardStartedAnimation(KCard*)) );
        connect( c, SIGNAL(animationStopped(KCard*)), d, SLOT(cardStoppedAnimation(KCard*)) );

        QString elementId = elementName( id, true );
        d->elementIdMapping[ elementId ].second.append( c );

        elementId = elementName( id, false );
        d->elementIdMapping[ elementId ].second.append( c );

        d->cards << c;
    }

    d->elementIds = d->elementIdMapping.keys();

    if ( d->currentCardSize.isValid() )
    {
        QHash<QString,QPair<QPixmap,QList<KCard*> > >::iterator it = d->elementIdMapping.begin();
        QHash<QString,QPair<QPixmap,QList<KCard*> > >::iterator end = d->elementIdMapping.end();
        while ( it != end )
        {
            if ( oldMapping.contains( it.key() ) )
                it.value().first = oldMapping[ it.key() ].first;

            it.value().first = d->requestPixmap( it.key(), false );

            ++it;
        }
    }
}


QList<KCard*> KAbstractCardDeck::cards() const
{
    return d->cards;
}


void KAbstractCardDeck::setCardWidth( int width )
{
    if ( width > 200 || width < 20 )
        return;

    int height = width * d->originalCardSize.height() / d->originalCardSize.width();
    QSize newSize( width, height );
    QSize oldSize = d->currentCardSize;

    if ( newSize != d->currentCardSize )
    {
        foreach ( KCard * c, d->cards )
            c->prepareGeometryChange();

        d->currentCardSize = newSize;
        d->cache->setSize( newSize );

        foreach ( KCard * c, d->cards )
            c->update();

        d->cache->loadInBackground( d->elementIds );
    }
}


int KAbstractCardDeck::cardWidth() const
{
    return d->currentCardSize.width();
}


void KAbstractCardDeck::setCardHeight( int height )
{
    setCardWidth( height * d->originalCardSize.width() / d->originalCardSize.height() );
}


int KAbstractCardDeck::cardHeight() const
{
    return d->currentCardSize.height();
}


QSize KAbstractCardDeck::cardSize() const
{
    return d->currentCardSize;
}


void KAbstractCardDeck::setTheme( const KCardTheme & theme )
{
    Q_ASSERT ( theme.isValid() );

    delete d->cache;
    d->cache = new KCardCache2( theme );
    connect( d->cache, SIGNAL(renderingDone(QString,QPixmap)), d, SLOT(pixmapUpdated(QString,QPixmap)) );

    d->originalCardSize = d->cache->naturalSize( "back" );
    Q_ASSERT( !d->originalCardSize.isNull() );
    d->currentCardSize = d->originalCardSize.toSize();
}


KCardTheme KAbstractCardDeck::theme() const
{
    return d->cache->theme();
}


bool KAbstractCardDeck::hasAnimatedCards() const
{
    return !d->cardsWaitedFor.isEmpty();
}


void KAbstractCardDeck::paintCard( QPainter * painter, quint32 id, bool faceUp, qreal highlightedness )
{
    QPixmap pix = d->requestPixmap( elementName( id, faceUp ), false );

    if ( highlightedness > 0 )
    {
        QPainter p( &pix );
        p.setCompositionMode( QPainter::CompositionMode_SourceAtop );
        p.setOpacity( 0.5 * highlightedness );
        p.fillRect( 0, 0, pix.width(), pix.height(), Qt::black );
    }

    painter->setRenderHint( QPainter::SmoothPixmapTransform );
    painter->drawPixmap( 0, 0, pix );
}


#include "kabstractcarddeck.moc"
#include "moc_kabstractcarddeck.cpp"
