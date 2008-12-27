/***********************-*-C++-*-********

  klondike.cpp  implements a patience card game

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

//
// 7 positions, alternating red and black
//

****************************************/

#include "klondike.h"
#include "version.h"
#include "deck.h"
#include "cardmaps.h"
#include "patsolve/klondike.h"
#include "view.h"

#include <cassert>

#include <QApplication>

#include <kdebug.h>
#include <klocale.h>
#include <kselectaction.h>
#include <kxmlguiwindow.h>
#include <kactioncollection.h>
#include <kxmlguifactory.h>
#include <kconfiggroup.h>

KlondikePile::KlondikePile( int _index, int _draw, DealerScene* parent)
    : Pile(_index, parent), m_draw( _draw )
{
}

void KlondikePile::setDraws( int _draw )
{
    m_draw = _draw;
}

QSizeF KlondikePile::cardOffset( const Card *c ) const
{
    if ( indexOf( c ) > cardsLeft() - m_draw )
        return QSizeF( 1.25, 0 );
    return QSizeF( 0, 0 );
}

void KlondikePile::relayoutCards()
{
    m_relayoutTimer->stop();
    int car = m_cards.count();
    qreal x = pos().x();
    qreal y = pos().y();
    qreal diff = 0;
    qreal z = zValue() + 1;
    for (CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        if ( ( *it )->animated() )
            continue;
        //kDebug() << "car" << car << " " << p;
        ( *it )->setPos( QPointF( x + diff * cardMap::self()->wantedCardWidth(), y ) );
        ( *it )->setSpread( QSizeF( diff * 10, 0 ) );
        ( *it )->setZValue( z );
        z = z+1;
        if ( car > m_draw )
        {
            --car;
        } else
            diff += 0.125;
    }
}

Klondike::Klondike()
    : DealerScene( )
{
    // The units of the follwoing constants are pixels
    const double margin = 2; // between card piles and board edge
    const double hspacing = 10. / 6 + 0.2; // horizontal spacing between card piles
    const double vspacing = 10. / 4; // vertical spacing between card piles

    Deck::create_deck(this);
    Deck::deck()->setPilePos(margin, margin );
    KConfigGroup cg(KGlobal::config(), settings_group );
    EasyRules = cg.readEntry( "KlondikeEasy", true);

    pile = new KlondikePile( 13, EasyRules ? 1 : 3, this);
    pile->setObjectName( "pile" );
    pile->setReservedSpace( QSizeF( 19, 10 ) );

    pile->setPilePos(margin + 10 + hspacing, margin);
    // Move the visual representation of the pile to the intended position
    // on the game board.

    pile->setAddFlags( Pile::disallow );
    pile->setRemoveFlags(Pile::Default);

    for( int i = 0; i < 7; i++ )
    {
        play[ i ] = new Pile( i + 5, this);
        play[i]->setPilePos(margin + (10. + hspacing) * i, margin + 10. + vspacing);
        play[i]->setAddType(Pile::KlondikeStore);
        play[i]->setRemoveFlags(Pile::several | Pile::autoTurnTop | Pile::wholeColumn);
        play[i]->setObjectName( QString( "play%1" ).arg( i ) );
        play[i]->setReservedSpace( QSizeF( 10, 10 + play[i]->spread() * 7 ) );
    }

    for( int i = 0; i < 4; i++ )
    {
        target[ i ] = new Pile( i + 1, this );
        target[i]->setPilePos(margin + (3 + i) * (10 + hspacing), margin);
        target[i]->setAddType(Pile::KlondikeTarget);
        if (EasyRules) // change default
            target[i]->setRemoveFlags(Pile::Default);
        else
            target[i]->setRemoveType(Pile::KlondikeTarget);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    setSolver( new KlondikeSolver( this, pile->draw() ) );
    redealt = false;

    options = new KSelectAction(i18n("Options"), this );

    KXmlGuiWindow *xmlgui = PatienceView::instance()->parent();

    xmlgui->actionCollection()->addAction("dealer_options", options);
    options->addAction( "Draw 1" );
    options->addAction( "Draw 3" );

    options->setCurrentItem( EasyRules ? 0 : 1 );

    QList<QAction*> actionlist;
    actionlist.append( options );

    xmlgui->guiFactory()->plugActionList( xmlgui, QString::fromLatin1("dealer_options"), actionlist);
    connect( options, SIGNAL( triggered( int ) ), SLOT( gameTypeChanged() ) );
}

Card *Klondike::demoNewCards() {
    deal3();
    if ( Deck::deck()->isEmpty() )
        return pile->top();
    else
        return Deck::deck()->top();
}

void Klondike::restart() {
    Deck::deck()->collectAndShuffle();
    redealt = false;
    deal();
}

void Klondike::gameTypeChanged()
{
    if ( demoActive() || isGameWon()  )
       return;

    setEasy( options->currentItem() == 0 );
}

void Klondike::setEasy( bool _EasyRules )
{
    EasyRules = _EasyRules;
    pile->setDraws( EasyRules ? 1 : 3 );
    options->setCurrentItem( EasyRules ? 0 : 1 );

    for( int i = 0; i < 4; i++ ) {
        if (EasyRules) // change default
            target[i]->setRemoveFlags(Pile::Default);
        else
            target[i]->setRemoveType(Pile::KlondikeTarget);
    }
    KConfigGroup cg(KGlobal::config(), settings_group );
    cg.writeEntry( "KlondikeEasy", EasyRules);
    cg.sync();

    startNew();
}

void Klondike::deal3()
{
    if (Deck::deck()->isEmpty())
    {
        redeal();
        return;
    }

    // move the cards back on the deck, so we can have three new
    for (int i = 0; i < pile->cardsLeft(); ++i)
        pile->at( i )->setSpread( QSizeF( 0, 0 ) );
    pile->relayoutCards();

    for (int flipped = 0; flipped < pile->draw() ; ++flipped) {

        Card *item = Deck::deck()->nextCard();
        if (!item) {
            kDebug(11111) << "deck empty!!!\n";
            return;
        }
        pile->add(item, true); // facedown, nospread
        if (flipped < pile->draw() - 1)
            item->setSpread( QSizeF( pile->spread() * 0.6, 0 ) );
        item->stopAnimation();
        // move back to flip
        item->setPos( Deck::deck()->pos() );

        item->flipTo( pile->x() + 0.125 * flipped * cardMap::self()->wantedCardWidth(), pile->y(), 200 + 80 * ( flipped + 1 ) );
    }

    // we need to look that many steps in the future to see if we can loose
    setNeededFutureMoves( Deck::deck()->cardsLeft() + pile->cardsLeft() );
}

//  Add cards from  pile to deck, in reverse direction
void Klondike::redeal() {

    CardList pilecards = pile->cards();
    if (EasyRules)
        // the remaining cards in deck should be on top
        // of the new deck
        pilecards += Deck::deck()->cards();

    for (int count = pilecards.count() - 1; count >= 0; --count)
    {
        Card *card = pilecards[count];
	card->stopAnimation();
	Deck::deck()->add(card, true); // facedown, nospread
    }

    redealt = true;
}

void Klondike::deal() {
    for(int round=0; round < 7; round++)
        for (int i = round; i < 7; i++ )
            play[i]->add(Deck::deck()->nextCard(), i != round && true);
}

bool Klondike::cardClicked(Card *c) {
    kDebug(11111) << "card clicked" << c->rank() << " " << c->suit();

    if (DealerScene::cardClicked(c))
        return true;

    if (c->source() == Deck::deck()) {
        pileClicked(Deck::deck());
        return true;
    }

    return false;
}

void Klondike::pileClicked(Pile *c) {
    kDebug(11111) << "pile clicked";
    DealerScene::pileClicked(c);

    if (c == Deck::deck()) {
        deal3();
    }
}

bool Klondike::startAutoDrop()
{
    bool pileempty = pile->isEmpty();
    if (!DealerScene::startAutoDrop())
        return false;
    if (pile->isEmpty() && !pileempty)
        deal3();
    return true;
}

static class LocalDealerInfo0 : public DealerInfo
{
public:
    LocalDealerInfo0() : DealerInfo(I18N_NOOP("Klondike"), 18) { addOldId(0); addOldId(13); }
    virtual DealerScene *createGame() { return new Klondike(); }
} ldi0;

void Klondike::mapOldId(int id)
{
   switch (id) {
   case 13: // draw 3
       setEasy( false );
   case 0: // draw 1
       setEasy( true );
   }
}

#include "klondike.moc"
