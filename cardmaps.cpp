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

#include <stdio.h>
#include <unistd.h>

#include <qpainter.h>
//Added by qt3to4:
#include <QPixmap>
#include <QDir>

#include <kconfig.h>

#include <stdlib.h>

#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include "version.h"
#include <qimage.h>
#include <krandom.h>
#include <kcarddialog.h>
#include <kglobalsettings.h>
#include <qfileinfo.h>
#include <assert.h>
#include <kglobal.h>
#include <QSvgRenderer>
#include <QPixmapCache>
#include <QThread>
#include <QMutex>
#include <kconfiggroup.h>
#ifdef __GNUC__
#warning cardmap should not really require to know the instance!
#endif
#include "view.h"
#include "dealer.h"
#include "ksvgrenderer.h"
#include "card.h"

#include "cardcache.h"

cardMap *cardMap::_self = 0;

static QHash<AbstractCard::Rank,KCardInfo::Card> ranks;
static QHash<AbstractCard::Suit,KCardInfo::Suit> suits;

class cardMapPrivate
{
public:

    double _wantedCardWidth;
    mutable double _scale;
    KCardCache m_cache;
    QSizeF m_backSize;
    bool m_backsvg;
    bool m_frontsvg;
    QRect m_body;
};

cardMap::cardMap() : QObject()
{
    d = new cardMapPrivate();

    Q_ASSERT(!_self);
    kDebug(11111) << "cardMap\n";
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs(config, settings_group );

    updateTheme(cs);
    setWantedCardWidth( cs.readEntry( "CardWidth", 100 ) );
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
    QString fronttheme = cs.readEntry( "Cardname", "Oxygen-white (SVG)" );
    QString backtheme = cs.readEntry( "Deckname", "Oxygen (SVG)" );
    d->m_cache.setFrontTheme( fronttheme );
    d->m_cache.setBackTheme( backtheme );
    
    d->m_body = QRect();

    d->m_backsvg = KCardDialog::isSVGDeck( fronttheme );
    d->m_frontsvg = KCardDialog::isSVGDeck( backtheme );

    d->m_backSize = d->m_cache.defaultBackSize();

    Q_ASSERT( !d->m_backSize.isNull() );
    if ( !d->m_backsvg || !d->m_frontsvg )
        setWantedCardWidth( d->m_backSize.width() );

    if (PatienceView::instance() && PatienceView::instance()->dscene()) {
        PatienceView::instance()->dscene()->rescale(false);
        if ( d->m_backsvg || d->m_frontsvg )
            PatienceView::instance()->dscene()->relayoutPiles();
    }
}

cardMap::~cardMap()
{
    delete d;
    if (_self == this)
      _self = 0;
}

double cardMap::wantedCardHeight() const
{
    return d->_wantedCardWidth / d->m_backSize.width() * d->m_backSize.height();
}

double cardMap::wantedCardWidth() const
{
    return d->_wantedCardWidth;
}

void cardMap::triggerRescale()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs(config, settings_group );
    cs.writeEntry( "CardWidth", d->_wantedCardWidth );
    config->sync();
    if (  PatienceView::instance()->dscene() )
         PatienceView::instance()->dscene()->rescale(false);
}

void cardMap::setWantedCardWidth( double w )
{
    if ( w > 200 || w < 10 || d->_wantedCardWidth == w )
        return;

    if ( !d->m_backsvg && !d->m_frontsvg && w != d->m_backSize.width() )
        return;

    d->m_body = QRect();
    kDebug(11111) << "setWantedCardWidth" << w << " " << d->_wantedCardWidth;
    d->_wantedCardWidth = w;
    d->_scale = 0;
    d->m_cache.setSize( QSize( wantedCardWidth(), wantedCardHeight() ) );
    if (PatienceView::instance()->dscene())
        PatienceView::instance()->dscene()->rescale(false);
    triggerRescale();
}

cardMap *cardMap::self() {
    assert(_self);
    return _self;
}

QRect cardMap::opaqueRect() const { return d->m_body; }

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

#include "cardmaps.moc"
