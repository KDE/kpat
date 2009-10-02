/*---------------------------------------------------------------------------

  spider.cpp  implements a patience card game

     Copyright (C) 2003  Josh Metzler

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

---------------------------------------------------------------------------*/

#include "spider.h"

#include "deck.h"
#include "speeds.h"
#include "version.h"
#include "view.h"
#include "patsolve/spider.h"

#include <KActionCollection>
#include <KConfigGroup>
#include <KDebug>
#include <KLocale>
#include <KRandom>
#include <KSelectAction>
#include <KXMLGUIFactory>
#include <KXmlGuiWindow>


void SpiderPile::moveCards(CardList &c, Pile *to)
{
    Pile::moveCards(c, to);

    // if this is a leg pile, don't do anything special
    if ( to->checkIndex() == 0 )
        return;

    // if the top card of the list I just moved is an Ace,
    // the run I just moved is the same suit as the pile,
    // and the destination pile now has more than 12 cards,
    // then it could have a full deck that needs removed.
    if (c.last()->rank() == Card::Ace &&
        c.first()->suit() == to->top()->suit() &&
        to->cardsLeft() > 12) {
            Spider *b = dynamic_cast<Spider*>(dscene());
            if (b) {
                b->checkPileDeck(to);
            }
        }
}

//-------------------------------------------------------------------------//

Spider::Spider()
    : DealerScene(), m_leg(0), m_redeal(0)
{
    const qreal dist_x = 11.2;

    KConfigGroup cg(KGlobal::config(), settings_group );
    m_suits = cg.readEntry( "SpiderSuits", 2);

    Deck::create_deck(this, 2, m_suits);

    // I deal the cards into 'redeal' piles, so hide the deck
    Deck::deck()->setVisible(false);

    // Dealing the cards out into 5 piles so the user can see how many
    // sets of 10 cards are left to be dealt out
    for( int column = 0; column < 5; column++ ) {
        redeals[column] = new Pile(column + 1, this);
        redeals[column]->setPilePos( 0 - dist_x / 3 * ( 5 - column ), -1);
        redeals[column]->setZValue(12 * ( 5-column ));
        redeals[column]->setCheckIndex(0);
        redeals[column]->setAddFlags(Pile::disallow);
        redeals[column]->setRemoveFlags(Pile::disallow);
        redeals[column]->setGraphicVisible( false );
        redeals[column]->setObjectName( QString( "redeals%1" ).arg( column ) );
        connect(redeals[column], SIGNAL(clicked(Card*)), SLOT(newCards()));
    }
    redeals[0]->setReservedSpace( QSizeF( -50, 10 ) );

    // The 10 playing piles
    for( int column = 0; column < 10; column++ ) {
        stack[column] = new SpiderPile(column + 6, this);
        stack[column]->setPilePos(1 + dist_x * column, 1);
        stack[column]->setZValue(20);
        stack[column]->setCheckIndex(1);
        stack[column]->setAddFlags(Pile::addSpread | Pile::several);
        stack[column]->setRemoveFlags(Pile::several |
                                      Pile::autoTurnTop | Pile::wholeColumn);
        stack[column]->setObjectName( QString( "stack%1" ).arg( column ) );
        stack[column]->setReservedSpace( QSizeF( 10, 35 ) );
    }

    // The 8 'legs' so named by me because spiders have 8 legs - why
    // else the name Spider?
    for( int column = 0; column < 8; column++ ) {
        legs[column] = new Pile(column + 16, this);
        legs[column]->setPilePos(1 + dist_x / 3 * column, -1);
        legs[column]->setZValue(column+1);
        legs[column]->setCheckIndex(0);
        legs[column]->setAddFlags(Pile::disallow);
        legs[column]->setRemoveFlags(Pile::disallow);
        legs[column]->setTarget(true);
        legs[column]->setGraphicVisible( false );
        legs[column]->setZValue(14 * column);
        legs[column]->setObjectName( QString( "legs%1" ).arg( column ) );
    }

    // Moving an A-K run to a leg is not really an autoDrop - the
    // user should have no choice.  Also, it must be moved A first, ...
    // up to K so the King will be on top.
    setAutoDropEnabled(false);
    setActions(DealerScene::Hint | DealerScene::Demo | DealerScene::Deal);
    setSolver( new SpiderSolver( this ) );

    options = new KSelectAction(i18n("Spider &Options"), this );
    options->addAction( i18n("1 Suit (Easy)") );
    options->addAction( i18n("2 Suits (Medium)") );
    options->addAction( i18n("4 Suits (Hard)") );
    if ( m_suits == 1 )
        options->setCurrentItem( 0 );
    else if ( m_suits == 2 )
        options->setCurrentItem( 1 );
    else
        options->setCurrentItem( 2 );
    connect( options, SIGNAL(triggered(int)), SLOT(gameTypeChanged()) );

    if ( PatienceView::instance() )
    {
        KXmlGuiWindow * xmlgui = PatienceView::instance()->mainWindow();
        if ( xmlgui )
        {
            xmlgui->actionCollection()->addAction("dealer_options", options);
            QList<QAction*> actionlist;
            actionlist.append( options );
            xmlgui->guiFactory()->plugActionList( xmlgui, "dealer_options", actionlist);
        }
    }
}

void Spider::gameTypeChanged()
{
    stopDemo();

    if ( allowedToStartNewGame() )
    {
        if ( options->currentItem() == 0 )
            setSuits( 1 );
        else if ( options->currentItem() == 1 )
            setSuits( 2 );
        else
            setSuits( 4 );
        startNew( gameNumber() );
    }
    else
    {
        // If we're not allowed, reset the option to
        // the current number of suits.
        if ( m_suits == 1 )
            options->setCurrentItem( 0 );
        else if ( m_suits == 2 )
            options->setCurrentItem( 1 );
        else
            options->setCurrentItem( 2 );
    }
}

void Spider::setSuits(int suits)
{
    if ( suits != m_suits )
    {
        m_suits = suits;

        stopDemo();
        unmarkAll();
        Deck::destroy_deck();
        Deck::create_deck(this, 2, m_suits);
        Deck::deck()->setVisible(false);
        KConfigGroup cg(KGlobal::config(), settings_group );
        cg.writeEntry( "SpiderSuits", m_suits);
        cg.sync();

        if ( m_suits == 1 )
            options->setCurrentItem( 0 );
        else if ( m_suits == 2 )
            options->setCurrentItem( 1 );
        else
            options->setCurrentItem( 2 );
    }
}

void Spider::cardStopped(Card * t)
{
    for( int column = 0; column < 10; column++ ) {
        Card *t = stack[column]->top();
        if (t && !t->isFaceUp())
           t->flipTo(t->x(), t->y(), DURATION_FLIP );
    }
    t->disconnect(this, SLOT( cardStopped( Card* ) ) );
    setWaiting( false );
}

//-------------------------------------------------------------------------//

bool Spider::checkAdd(int /*checkIndex*/, const Pile *c1, const CardList& c2) const
{
    // assuming the cardlist is a valid unit, since I allowed
    // it to be removed - can drop any card on empty pile or
    // on any suit card of one higher rank
    if (c1->isEmpty() || c1->top()->rank() == c2.first()->rank()+1)
        return true;

    return false;
}

bool Spider::checkRemove(int /*checkIndex*/, const Pile *p, const Card *c) const
{
    // if the pile from c up is decreasing by 1 and all the same suit, ok
    // note that this is true if c is the top card
    const Card *before;
    int index = p->indexOf(c);
    while (c != p->top()) {
        before = c;
        c = p->at(++index);
        if (before->suit() != c->suit() || before->rank() != c->rank()+1)
            return false;
    }
    return true;
}

//-------------------------------------------------------------------------//

QString Spider::getGameState()
{
    return QString::number(m_leg*10 + m_redeal);
}

void Spider::setGameState(const QString &stream)
{
    int i = stream.toInt();

    m_leg = i/10;

    if (m_redeal > i%10) {
        for (m_redeal--; m_redeal > i%10; m_redeal--)
            redeals[m_redeal]->setVisible(true);
        redeals[m_redeal]->setVisible(true);
    } else
        for (; m_redeal < i%10; m_redeal++)
            redeals[m_redeal]->setVisible(false);

    emit newCardsPossible(m_redeal <= 4);
}

QString Spider::getGameOptions() const
{
    return QString::number(Deck::deck()->suitsNum());
}

void Spider::setGameOptions(const QString& options)
{
    setSuits(options.toInt());
}

//-------------------------------------------------------------------------//

void Spider::restart()
{
    Deck::deck()->collectAndShuffle();
    deal();
    emit newCardsPossible(true);
}

//-------------------------------------------------------------------------//

CardList Spider::getRun(Card *c) const
{
    CardList result;

    Pile *p = c->source();
    if (!p || p->isEmpty())
        return result;

    result.append(c);

    Card::Suit s = c->suit();
    int v = c->rank();

    int index = p->indexOf(c);
    c = p->at(--index);
    while (index >= 0 && c->realFace() &&
        c->suit() == s && c->rank() == ++v)
    {
        result.prepend(c);
        c = p->at(--index);
    }

    return result;
}

bool Spider::checkPileDeck(Pile *check, bool checkForDemo)
{
    if ( checkForDemo && demoActive() )
        return false;

    //kDebug(11111) << "check for run";
    if (check->isEmpty())
        return false;

    if (check->top()->rank() == Card::Ace) {
        // just using the CardList to see if this goes to King
        CardList run = getRun(check->top());
        if (run.first()->rank() == Card::King) {
            CardList::iterator it = run.end();
            qreal z = 1;
            while (it != run.begin()) {
                --it;
                legs[m_leg]->add( *it );
                // ( *it )->setSpread( QSize( 0, 0 ) );
                ( *it )->moveTo( legs[m_leg]->x(), legs[m_leg]->y(), legs[m_leg]->zValue() + z, 300 + int( z ) * 30 );
                z += 1;
            }
            connect(run.first(), SIGNAL(stopped(Card*)), SLOT(cardStopped(Card*)));
            setWaiting( true );
            /*if ( demoActive() )
              newDemoMove( run.first() );*/
            m_leg++;

            return true;
        }
    }
    return false;
}

//-------------------------------------------------------------------------//

void Spider::deal()
{
    unmarkAll();

    m_leg = 0;
    m_redeal = 0;

    int column = 0;
    // deal face down cards (5 to first 4 piles, 4 to last 6)
    for (int i = 0; i < 44; i++ ) {
        stack[column]->add(Deck::deck()->nextCard(), true);
        column = (column + 1) % 10;
    }
    // deal face up cards, one to each pile
    for (int i = 0; i < 10; i++ ) {
        stack[column]->add(Deck::deck()->nextCard(), false);
        column = (column + 1) % 10;
    }
    // deal the remaining cards into 5 'redeal' piles
    for (int column = 0; column < 5; column++ )
        for (int i = 0; i < 10; i++ )
            redeals[column]->add(Deck::deck()->nextCard(), true);

    // make the redeal piles visible
    for (int i = 0; i < 5; i++ )
        redeals[i]->setVisible(true);
}

Card *Spider::newCards()
{
    if ( demoActive() )
    {
        for ( int i = 0; i < 10; i++ )
        {
            if ( checkPileDeck( stack[i], false ) )
                return legs[m_leg-1]->top();
        }
    }

    if (m_redeal > 4)
        return 0;

    unmarkAll();

    for (int column = 0; column < 10; column++) {
        stack[column]->add(redeals[m_redeal]->top(), false);

        // I may put an Ace on a K->2 pile so it could need cleared.
        if (stack[column]->top()->rank() == Card::Ace)
            checkPileDeck(stack[column]);
    }
    redeals[m_redeal++]->setVisible(false);

    takeState();
    considerGameStarted();
    if (m_redeal > 4)
        emit newCardsPossible(false);

    return stack[0]->top();
}

static class LocalDealerInfo17 : public DealerInfo
{
public:
    LocalDealerInfo17() : DealerInfo(I18N_NOOP("Spider"), 17) { addOldId(14); addOldId(15); addOldId(16); }
    virtual DealerScene *createGame() const { return new Spider(); }
} ldi17;

void Spider::mapOldId(int id)
{
   switch (id) {
   case 14:
       setSuits(1);
       break;
   case 15:
       setSuits(2);
       break;
   case 16:
       setSuits(4);
       break;
   }
}

//-------------------------------------------------------------------------//

#include "spider.moc"

//-------------------------------------------------------------------------//

