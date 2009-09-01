/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2008 Andreas Pakulat <apaku@gmx.de>
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

#include "cardmaps.h"

#include "deck.h"
#include "version.h"

#include <KCardCache>
#include <KCardDeckInfo>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KSharedConfig>

cardMap *cardMap::_self = 0;

static QHash<AbstractCard::Rank,KCardInfo::Card> ranks;
static QHash<AbstractCard::Suit,KCardInfo::Suit> suits;

class cardMapPrivate
{
public:
    cardMapPrivate()
      : m_originalCardSize(1, 1),
        m_currentCardSize(0, 0)
    {}
    KCardCache m_cache;
    QSizeF m_originalCardSize;
    QSize m_currentCardSize;
};

cardMap::cardMap() : QObject()
{
    d = new cardMapPrivate();

    Q_ASSERT(!_self);
    kDebug(11111) << "cardMap\n";
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs(config, settings_group );

    updateTheme(cs);
    setCardWidth( cs.readEntry( "CardWidth", 100 ) );
    suits[AbstractCard::Clubs] = KCardInfo::Club;
    suits[AbstractCard::Spades] = KCardInfo::Spade;
    suits[AbstractCard::Diamonds] = KCardInfo::Diamond;
    suits[AbstractCard::Hearts] = KCardInfo::Heart;
    ranks[AbstractCard::Two] = KCardInfo::Two;
    ranks[AbstractCard::Three] = KCardInfo::Three;
    ranks[AbstractCard::Four] = KCardInfo::Four;
    ranks[AbstractCard::Five] = KCardInfo::Five;
    ranks[AbstractCard::Six] = KCardInfo::Six;
    ranks[AbstractCard::Seven] = KCardInfo::Seven;
    ranks[AbstractCard::Eight] = KCardInfo::Eight;
    ranks[AbstractCard::Nine] = KCardInfo::Nine;
    ranks[AbstractCard::Ten] = KCardInfo::Ten;
    ranks[AbstractCard::Jack] = KCardInfo::Jack;
    ranks[AbstractCard::Queen] = KCardInfo::Queen;
    ranks[AbstractCard::King] = KCardInfo::King;
    ranks[AbstractCard::Ace] = KCardInfo::Ace;
}

void cardMap::updateTheme(const KConfigGroup &cs)
{
    QString fronttheme = CardDeckInfo::frontTheme( cs );
    QString backtheme = CardDeckInfo::backTheme( cs );
    Q_ASSERT ( !backtheme.isEmpty() );
    Q_ASSERT ( !fronttheme.isEmpty() );

    d->m_cache.setFrontTheme( fronttheme );
    d->m_cache.setBackTheme( backtheme );

    d->m_originalCardSize = d->m_cache.defaultFrontSize( KCardInfo( KCardInfo::Spade, KCardInfo::Ace ) );
    Q_ASSERT( !d->m_originalCardSize.isNull() );
    d->m_currentCardSize = d->m_originalCardSize.toSize();
}

cardMap::~cardMap()
{
    delete d;
    if (_self == this)
      _self = 0;
}

QSize cardMap::cardSize() const
{
    return d->m_currentCardSize;
}

int cardMap::cardWidth() const
{
    return d->m_currentCardSize.width();
}

int cardMap::cardHeight() const
{
    return d->m_currentCardSize.height();
}

void cardMap::setCardWidth( int width )
{
    if ( width > 200 || width < 10 )
        return;

    int height = width * d->m_originalCardSize.height() / d->m_originalCardSize.width();
    QSize newSize( width, height );

    if ( newSize != d->m_currentCardSize )
    {
        KConfigGroup cs( KGlobal::config(), settings_group );
        cs.writeEntry( "CardWidth", d->m_currentCardSize.width() );

        d->m_currentCardSize = newSize;
        d->m_cache.setSize( newSize );
        if (Deck::deck())
            Deck::deck()->updatePixmaps();

        QTimer::singleShot( 200, this, SLOT(loadInBackground()) );;
    }
}

cardMap *cardMap::self() 
{
    if ( !_self )
        _self = new cardMap();
    return _self;
}

QPixmap cardMap::renderBackside( int variant )
{
    QPixmap pix = d->m_cache.backside( variant );
    return pix;
}

QPixmap cardMap::renderFrontside( AbstractCard::Rank r, AbstractCard::Suit s )
{
    QPixmap pix = d->m_cache.frontside( KCardInfo( suits[s], ranks[r] ) );
    return pix;
}

void cardMap::loadInBackground()
{
    d->m_cache.loadTheme( KCardCache::LoadFrontSide | KCardCache::Load52Cards );
}

#include "cardmaps.moc"
