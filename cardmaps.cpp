/***********************-*-C++-*-********

  cardmaps.cpp  defines pixmaps for playing cards

     Copyright (C) 1995  Paul Olav Tvete

 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.

****************************************/

#include "cardmaps.h"

#include "dealer.h"
#include "version.h"
#include "view.h"

#include <KCardCache>
#include <KCardDeckInfo>
#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KSharedConfig>


#ifdef __GNUC__
#warning cardmap should not really require to know the instance!
#endif

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
    _self = this;
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

    if (PatienceView::instance() && PatienceView::instance()->dscene()) {
        PatienceView::instance()->dscene()->relayoutPiles();
        PatienceView::instance()->dscene()->rescale(false);
    }
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

void cardMap::triggerRescale()
{
    KConfigGroup cs( KGlobal::config(), settings_group );
    cs.writeEntry( "CardWidth", d->m_currentCardSize.width() );

    if ( PatienceView::instance() && PatienceView::instance()->dscene() )
         PatienceView::instance()->dscene()->rescale(false);
}

void cardMap::setCardWidth( int width )
{
    if ( width > 200 || width < 10 )
        return;

    int height = width * d->m_originalCardSize.height() / d->m_originalCardSize.width();
    QSize newSize( width, height );

    if ( newSize != d->m_currentCardSize )
    {
        d->m_currentCardSize = newSize;
        d->m_cache.setSize( newSize );
        triggerRescale();

        QMetaObject::invokeMethod( this, "loadInBackground", Qt::QueuedConnection );
    }
}

cardMap *cardMap::self() 
{
    Q_ASSERT(_self);
    return _self;
}

QRect cardMap::opaqueRect() const
{
    return QRect();
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
