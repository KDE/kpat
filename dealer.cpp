/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009-2010 Parker Coates <parker.coates@kdemail.net>
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

#include "dealer.h"

#include "gamestate.h"
#include "messagebox.h"
#include "renderer.h"
#include "speeds.h"
#include "version.h"
#include "view.h"
#include "patsolve/patsolve.h"

#include "KCardTheme"

#include <KConfigGroup>
#include <KDebug>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRandom>
#include <KSharedConfig>
#include <KTemporaryFile>
#include <KIO/NetAccess>

#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtCore/QStack>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QGraphicsView>
#include <QtGui/QPainter>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QtXml/QDomDocument>

#include <cmath>

#define DEBUG_HINTS 0


namespace
{
    const qreal wonBoxToSceneSizeRatio = 0.7;

    QList<KCard*> shuffled( const QList<KCard*> & cards, int seed )
    {
        Q_ASSERT( seed > 0 );

        QList<KCard*> result = cards;
        for ( int i = result.size(); i > 1; --i )
        {
            // We use the same pseudorandom number generation algorithm as Windows
            // Freecell, so that game numbers are the same between the two applications.
            // For more inforation, see
            // http://support.microsoft.com/default.aspx?scid=kb;EN-US;Q28150
            seed = 214013 * seed + 2531011;
            int rand = ( seed >> 16 ) & 0x7fff;

            result.swap( i - 1, rand % i );
        }

        return result;
    }
}


class SolverThread : public QThread
{
    Q_OBJECT

public:
    SolverThread( Solver * solver )
      : m_solver( solver )
    {
    }

    virtual void run()
    {
        Solver::ExitStatus result = m_solver->patsolve();
        emit finished( result );
    }

    void abort()
    {
        {
            QMutexLocker lock( &m_solver->endMutex );
            m_solver->m_shouldEnd = true;
        }
        wait();
    }

signals:
    void finished( int result );

private:
    Solver * m_solver;
};


class DealerScene::DealerScenePrivate
{
public:
    int gameId;
    int gameNumber;

    bool autodropEnabled;
    bool solverEnabled;

    int actions;
    int neededFutureMoves;
    int loadedMoveCount;

    bool gameHasBeenWon;
    bool gameWasEverWinnable;
    bool gameStarted;
    bool gameRecorded;
    bool gameWasJustSaved;
    bool playerReceivedHelp;

    // we need a flag to avoid telling someone the game is lost
    // just because the winning animation moved the cards away
    bool toldAboutWonGame;
    bool toldAboutLostGame;

    KCard * peekedCard;

    Solver * solver;
    SolverThread * solverThread;
    MessageBox * wonItem;

    QTimer * updateSolver;
    QTimer * demoTimer;
    QTimer * dropTimer;

    qreal dropSpeedFactor;
    bool interruptAutoDrop;

    bool hintInProgress;
    bool dropInProgress;
    bool dealInProgress;
    bool demoInProgress;

    bool takeStateQueued;
    bool hintQueued;
    bool demoQueued;
    bool dropQueued;
    bool newCardsQueued;

    QList<QPair<KCard*,KCardPile*> > multiStepMoves;
    int multiStepDuration;

    QStack<GameState*> undoStack;
    GameState * currentState;
    QStack<GameState*> redoStack;

    QList<PatPile*> patPiles;
    QSet<KCard*> cardsNotToDrop;
    QList<MOVE> winMoves;
    QMap<KCard*,QPointF> initDealPositions;
};

int DealerScene::moveCount() const
{
    return d->loadedMoveCount + d->undoStack.size();
}


void DealerScene::saveGame(QDomDocument &doc)
{
    QDomElement dealer = doc.createElement("dealer");
    doc.appendChild(dealer);
    dealer.setAttribute("id", gameId());
    dealer.setAttribute("options", getGameOptions());
    dealer.setAttribute("number", QString::number(gameNumber()));
    dealer.setAttribute("moves", moveCount());
    dealer.setAttribute("started", d->gameStarted);
    QString data = getGameState();
    if (!data.isEmpty())
        dealer.setAttribute("data", data);

    foreach( PatPile * p, patPiles() )
    {
        QDomElement pile = doc.createElement("pile");
        pile.setAttribute("index", p->index());
        pile.setAttribute("z", p->zValue());

        foreach(const KCard * c, p->cards() )
        {
            QDomElement card = doc.createElement("card");
            card.setAttribute("suit", c->suit());
            card.setAttribute("value", c->rank());
            card.setAttribute("faceup", c->isFaceUp());
            pile.appendChild(card);
        }
        dealer.appendChild(pile);
     }

    d->gameWasJustSaved = true;
}

void DealerScene::openGame(QDomDocument &doc)
{
    resetInternals();

    QDomElement dealer = doc.documentElement();

    QString options = dealer.attribute("options");

    // Before KDE4.3, KPat didn't store game specific options in the save
    // file. This could cause crashes when loading a Spider game with a
    // different number of suits than the current setting. Similarly, in 
    // Klondike the number of cards drawn from the deck was forgotten, but
    // this never caused crashes. Fortunately, in Spider we can count the
    // number of suits ourselves. For Klondike, there is no way to recover
    // that information.
    if (gameId() == 17 && options.isEmpty())
    {
        QSet<int> suits;
        QDomNodeList cardElements = dealer.elementsByTagName("card");
        for (int i = 0; i < cardElements.count(); ++i)
            suits.insert(cardElements.item(i).toElement().attribute("suit").toInt());
        options = QString::number(suits.count());
    }

    setGameOptions(options);
    d->gameNumber = dealer.attribute("number").toInt();
    d->loadedMoveCount = dealer.attribute("moves").toInt();
    d->gameStarted = bool(dealer.attribute("started").toInt());

    QDomNodeList pileNodes = dealer.elementsByTagName("pile");

    QMultiHash<quint32,KCard*> cards;
    foreach ( KCard * c, deck()->cards() )
        cards.insert( c->id(), c );

    foreach (PatPile *p, patPiles())
    {
        p->clear();
        for (int i = 0; i < pileNodes.count(); ++i)
        {
            QDomElement pile = pileNodes.item(i).toElement();
            if (pile.attribute("index").toInt() == p->index())
            {
                QDomNodeList cardNodes = pile.elementsByTagName("card");
                for (int j = 0; j < cardNodes.count(); ++j)
                {
                    QDomElement card = cardNodes.item(j).toElement();
                    int s = card.attribute("suit").toInt();
                    int r = card.attribute("value").toInt();
                    
                    KCard * c = cards.take( ( s << 4 ) + r );
                    if (!c)
                        continue;

                    c->setFaceUp(card.attribute("faceup").toInt());
                    p->add(c);
                }
                break;
            }
        }
        updatePileLayout( p, 0 );
    }
    setGameState( dealer.attribute("data") );

    emit updateMoves( moveCount());

    takeState();
}

// ================================================================
//                         class DealerScene


DealerScene::DealerScene()
{
    setItemIndexMethod(QGraphicsScene::NoIndex);

    d = new DealerScenePrivate();

    d->autodropEnabled = false;
    d->solverEnabled = false;
    d->gameNumber = 0;

    d->gameHasBeenWon = false;
    d->gameRecorded = false;
    d->gameStarted = false;
    d->gameWasJustSaved = false;
    d->playerReceivedHelp = false;
    d->actions = 0;
    d->solver = 0;
    d->solverThread = 0;
    d->neededFutureMoves = 1;
    d->toldAboutLostGame = false;
    d->toldAboutWonGame = false;
    d->updateSolver = new QTimer(this);
    d->updateSolver->setInterval( 250 );
    d->updateSolver->setSingleShot( true );
    d->loadedMoveCount = 0;
    d->peekedCard = 0;
    connect( d->updateSolver, SIGNAL(timeout()), SLOT(stopAndRestartSolver()) );

    d->demoTimer = new QTimer(this);
    d->demoTimer->setSingleShot( true );
    connect( d->demoTimer, SIGNAL(timeout()), SLOT(demo()) );
    d->dropSpeedFactor = 1;

    d->interruptAutoDrop = false;

    d->hintInProgress = false;
    d->dealInProgress = false;
    d->demoInProgress = false;
    d->dropInProgress = false;

    d->takeStateQueued = false;
    d->hintQueued = false;
    d->demoQueued = false;
    d->dropQueued = false;
    d->newCardsQueued = false;

    connect( this, SIGNAL(cardAnimationDone()), this, SLOT(animationDone()) );

    d->dropTimer = new QTimer( this );
    connect( d->dropTimer, SIGNAL(timeout()), this, SLOT(drop()) );
    d->dropTimer->setSingleShot( true );

    d->wonItem = new MessageBox();
    d->wonItem->setZValue( 2000 );
    d->wonItem->hide();
    addItem( d->wonItem );

    d->currentState = 0;

    connect( this, SIGNAL(cardDoubleClicked(KCard*)), this, SLOT(tryAutomaticMove(KCard*)) );
    // Make rightClick == doubleClick. See bug #151921
    connect( this, SIGNAL(cardRightClicked(KCard*)), this, SLOT(tryAutomaticMove(KCard*)) );
}

DealerScene::~DealerScene()
{
    stop();

    disconnect();
    if ( d->solverThread )
        d->solverThread->abort();
    delete d->solverThread;
    d->solverThread = 0;
    delete d->solver;
    d->solver = 0;
    qDeleteAll( d->undoStack );
    delete d->currentState;
    qDeleteAll( d->redoStack );
    delete d->wonItem;

    delete d;
}


void DealerScene::addPile( KCardPile * pile )
{
    d->patPiles.clear();

    KCardScene::addPile( pile );
}


void DealerScene::removePile( KCardPile * pile )
{
    d->patPiles.clear();

    KCardScene::removePile( pile );
}


QList<PatPile*> DealerScene::patPiles() const
{
    if ( d->patPiles.isEmpty() )
    {
        foreach( KCardPile * p, piles() )
        {
            PatPile * pp = dynamic_cast<PatPile*>( p );
            if ( pp )
                d->patPiles << pp;
        }
    }

    return d->patPiles;
}


void DealerScene::cardsMoved( const QList<KCard*> & cards, KCardPile * oldPile, KCardPile * newPile )
{
    PatPile * newPatPile = dynamic_cast<PatPile*>( newPile );
    PatPile * oldPatPile = dynamic_cast<PatPile*>( oldPile );

    if ( oldPatPile && oldPatPile->isFoundation() && newPatPile && !newPatPile->isFoundation() )
    {
        foreach ( KCard * c, cards )
            d->cardsNotToDrop.insert( c );
    }
    else
    {
        foreach ( KCard * c, cards )
            d->cardsNotToDrop.remove( c );
    }

    if ( !d->dropInProgress && !d->dealInProgress )
    {
        d->gameStarted = true;
        takeState();
    }
}


void DealerScene::startHint()
{
    stopDemo();
    stopDrop();

    if ( isCardAnimationRunning() )
    {
        d->hintQueued = true;
        return;
    }

    if ( isKeyboardModeActive() )
        setKeyboardModeActive( false );

    QList<QGraphicsItem*> toHighlight;
    foreach ( const MoveHint & h, getHints() )
        toHighlight << h.card();

    if ( !d->winMoves.isEmpty() )
    {
        MOVE m = d->winMoves.first();
        MoveHint mh = solver()->translateMove( m );
        if ( mh.isValid() )
            toHighlight << mh.card();
    }

    d->hintInProgress = !toHighlight.isEmpty();
    setHighlightedItems( toHighlight );
    emit hintActive( d->hintInProgress );
}


void DealerScene::stopHint()
{
    if ( d->hintInProgress )
    {
        d->hintInProgress = false;
        clearHighlightedItems();
        emit hintActive( false );
    }
}


bool DealerScene::isHintActive() const
{
    return d->hintInProgress;
}

QList<MoveHint> DealerScene::getSolverHints()
{
    QList<MoveHint> hintList;

    if ( d->solverThread && d->solverThread->isRunning() )
        d->solverThread->abort();

    solver()->translate_layout();
    bool debug = false;
#if DEBUG_HINTS
    debug = true;
#endif
    solver()->patsolve( 1, debug );

    foreach ( const MOVE & m, solver()->firstMoves )
    {
        MoveHint mh = solver()->translateMove( m );
	hintList << mh;
    }
    return hintList;
}

QList<MoveHint> DealerScene::getHints()
{
    if ( solver() )
        return getSolverHints();

    QList<MoveHint> hintList;
    foreach (PatPile * store, patPiles())
    {
        if (store->isFoundation() || store->isEmpty())
            continue;

        QList<KCard*> cards = store->cards();
        while (cards.count() && !cards.first()->isFaceUp())
            cards.erase(cards.begin());

        QList<KCard*>::Iterator iti = cards.begin();
        while (iti != cards.end())
        {
            if (allowedToRemove(store, (*iti)))
            {
                foreach (PatPile * dest, patPiles())
                {
                    int cardIndex = store->indexOf(*iti);
                    if (cardIndex == 0 && dest->isEmpty() && !dest->isFoundation())
                        continue;

                    if (!checkAdd(dest, dest->cards(), cards))
                        continue;

                    if (dest->isFoundation())
                    {
                        hintList << MoveHint( *iti, dest, 127 );
                    }
                    else
                    {
                        QList<KCard*> cardsBelow = cards.mid(0, cardIndex);
 
                        // if it could be here as well, then it's no use
                        if ((cardsBelow.isEmpty() && !dest->isEmpty()) || !checkAdd(store, cardsBelow, cards))
                        {
                            hintList << MoveHint( *iti, dest, 0 );
                        }
                        else if (checkPrefering(dest, dest->cards(), cards)
                                 && !checkPrefering(store, cardsBelow, cards))
                        { // if checkPrefers says so, we add it nonetheless
                            hintList << MoveHint( *iti, dest, 10 );
                        }
                    }
                }
            }
            cards.erase(iti);
            iti = cards.begin();
        }
    }
    return hintList;
}

static bool prioSort(const MoveHint &c1, const MoveHint &c2) 
{
  return c1.priority() < c2.priority();
}


MoveHint DealerScene::chooseHint()
{
    if ( !d->winMoves.isEmpty() )
    {
        MOVE m = d->winMoves.takeFirst();
        MoveHint mh = solver()->translateMove( m );

#if DEBUG_HINTS
        if ( m.totype == O_Type )
            fprintf( stderr, "move from %d out (at %d) Prio: %d\n", m.from,
                     m.turn_index, m.pri );
        else
            fprintf( stderr, "move from %d to %d (%d) Prio: %d\n", m.from, m.to,
                     m.turn_index, m.pri );
#endif

        return mh;
    }

    QList<MoveHint> hintList = getHints();

    if ( hintList.isEmpty() )
    {
        return MoveHint();
    }
    else
    {
        // Generate a random number with an exponentional distribution averaging 1/4.
        qreal randomExp = qMin<qreal>( -log( 1 - qreal( KRandom::random() ) / RAND_MAX ) / 4, 1 );
        int randomIndex =  randomExp * ( hintList.size() - 1 );

	qSort(hintList.begin(), hintList.end(), prioSort);
        return hintList.at( randomIndex );
    }
}


void DealerScene::startNew(int gameNumber)
{
    if (gameNumber != -1)
        d->gameNumber = qBound( 1, gameNumber, INT_MAX );

    // Only record the statistics and reset gameStarted if  we're starting a
    // new game number or we're restarting a game we've already won.
    if (gameNumber != -1 || d->gameHasBeenWon)
    {
        recordGameStatistics();
        d->gameRecorded = false;
        d->gameStarted = false;
    }

    if ( isCardAnimationRunning() )
    {
        QTimer::singleShot( 100, this, SLOT( startNew() ) );
        return;
    }

    if ( d->solverThread && d->solverThread->isRunning() )
        d->solverThread->abort();

    resetInternals();

    emit updateMoves( 0 );

    foreach( KCardPile * p, piles() )
        p->clear();

    d->dealInProgress = true;
    restart( shuffled( deck()->cards(), d->gameNumber ) );
    d->dealInProgress = false;

    takeState();
    update();
}

void DealerScene::resetInternals()
{
    stop();

    setKeyboardModeActive( false );

    d->gameHasBeenWon = false;
    d->wonItem->hide();

    qDeleteAll( d->undoStack );
    d->undoStack.clear();
    delete d->currentState;
    d->currentState = 0;
    qDeleteAll( d->redoStack );
    d->redoStack.clear();

    d->gameWasJustSaved = false;
    d->gameWasEverWinnable = false;
    d->toldAboutLostGame = false;
    d->toldAboutWonGame = false;
    d->loadedMoveCount = 0;

    d->playerReceivedHelp = false;

    d->dealInProgress = false;

    d->dropInProgress = false;
    d->dropSpeedFactor = 1;
    d->cardsNotToDrop.clear();

    foreach (KCard * c, deck()->cards())
    {
        c->disconnect( this );
        c->stopAnimation();
    }

    emit solverStateChanged( QString() );
    emit hintPossible( true );
    emit demoPossible( true );
    emit dropPossible( true );
}

QPointF posAlongRect( qreal distOnRect, const QRectF & rect )
{
    if ( distOnRect < rect.width() )
        return rect.topLeft() + QPointF( distOnRect, 0 );
    distOnRect -= rect.width();
    if ( distOnRect < rect.height() )
        return rect.topRight() + QPointF( 0, distOnRect );
    distOnRect -= rect.height();
    if ( distOnRect < rect.width() )
        return rect.bottomRight() + QPointF( -distOnRect, 0 );
    distOnRect -= rect.width();
    return rect.bottomLeft() + QPointF( 0, -distOnRect );
}

void DealerScene::won()
{
    if (d->gameHasBeenWon)
        return;

    d->gameHasBeenWon = true;
    d->toldAboutWonGame = true;

    stopDemo();
    recordGameStatistics();

    emit solverStateChanged( QString() );

    emit demoPossible( false );
    emit hintPossible( false );
    emit dropPossible( false );
    emit newCardsPossible( false );
    emit undoPossible( false );

    setKeyboardModeActive( false );

    qreal speed = sqrt( width() * width() + height() * height() ) / ( DURATION_WON );

    QRectF justOffScreen = sceneRect().adjusted( -deck()->cardWidth(), -deck()->cardHeight(), 0, 0 );
    qreal spacing = 2 * ( justOffScreen.width() + justOffScreen.height() ) / deck()->cards().size();
    qreal distOnRect = 0;

    foreach ( KCard *c, deck()->cards() )
    {
        distOnRect += spacing;
        QPointF pos2 = posAlongRect( distOnRect, justOffScreen );
        QPointF delta = c->pos() - pos2;
        qreal dist = sqrt( delta.x() * delta.x() + delta.y() * delta.y() );

        c->setFaceUp( true );
        c->animate( pos2, c->zValue(), 0, true, false, dist / speed );
    }

    connect(deck(), SIGNAL(cardAnimationDone()), this, SLOT(showWonMessage()));
}

void DealerScene::showWonMessage()
{
    disconnect(deck(), SIGNAL(cardAnimationDone()), this, SLOT(showWonMessage()));

    // It shouldn't be necessary to stop the demo yet again here, but we
    // get crashes if we don't. Will have to look into this further.
    stopDemo();

    // Hide all cards to prevent them from showing up accidentally if the 
    // window is resized.
    foreach ( KCard * c, deck()->cards() )
        c->hide();

    updateWonItem();
    d->wonItem->show();
}

void DealerScene::updateWonItem()
{
    const qreal aspectRatio = Renderer::self()->aspectRatioOfElement("message_frame");
    int boxWidth;
    int boxHeight;
    if ( width() < aspectRatio * height() )
    {
        boxWidth = width() * wonBoxToSceneSizeRatio;
        boxHeight = boxWidth / aspectRatio;
    }
    else
    {
        boxHeight = height() * wonBoxToSceneSizeRatio;
        boxWidth = boxHeight * aspectRatio;
    }
    d->wonItem->setSize( QSize( boxWidth, boxHeight ) );

    if ( d->playerReceivedHelp )
        d->wonItem->setMessage( i18n( "Congratulations! We have won." ) );
    else
        d->wonItem->setMessage( i18n( "Congratulations! You have won." ) );

    d->wonItem->setPos( QPointF( (width() - boxWidth) / 2, (height() - boxHeight) / 2 )
                        + sceneRect().topLeft() );
}


bool DealerScene::allowedToAdd( const KCardPile * pile, const QList<KCard*> & cards ) const
{
    if ( !pile->isEmpty() && !pile->topCard()->isFaceUp() )
        return false;

    const PatPile * p = dynamic_cast<const PatPile*>( pile );
    return p && checkAdd( p, p->cards(), cards );
}


bool DealerScene::allowedToRemove( const KCardPile * pile, const KCard * card ) const
{
    const PatPile * p = dynamic_cast<const PatPile*>( pile );
    QList<KCard*> cards = pile->topCardsDownTo( card );
    return p
           && card->isFaceUp()
           && !cards.isEmpty()
           && checkRemove( p, cards );
}


bool DealerScene::checkAdd( const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards ) const
{
    Q_UNUSED( pile )
    Q_UNUSED( oldCards )
    Q_UNUSED( newCards )
    return false;
}


bool DealerScene::checkRemove(const PatPile * pile, const QList<KCard*> & cards) const
{
    Q_UNUSED( pile )
    Q_UNUSED( cards )
    return false;
}


bool DealerScene::checkPrefering( const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards ) const
{
    Q_UNUSED( pile )
    Q_UNUSED( oldCards )
    Q_UNUSED( newCards )
    return false;
}


void DealerScene::mousePressEvent( QGraphicsSceneMouseEvent * e )
{
    stop();

    KCard * card = qgraphicsitem_cast<KCard*>( itemAt( e->scenePos() ) );

    if ( d->peekedCard )
    {
        e->accept();
    }
    else if ( e->button() == Qt::RightButton
              && card
              && card->pile()
              && card != card->pile()->topCard()
              && cardsBeingDragged().isEmpty()
              && !isCardAnimationRunning() )
    {
        e->accept();
        d->peekedCard = card;
        QPointF pos2( card->x() + deck()->cardWidth() / 3.0, card->y() - deck()->cardHeight() / 3.0 );
        card->setZValue( card->zValue() + 0.1 );
        card->animate( pos2, card->zValue(), 20, card->isFaceUp(), false, DURATION_FANCYSHOW );
    }
    else
    {
        KCardScene::mousePressEvent( e );
        if ( !cardsBeingDragged().isEmpty() )
            emit cardsPickedUp();
    }
}


void DealerScene::mouseReleaseEvent( QGraphicsSceneMouseEvent * e )
{
    stop();

    if ( e->button() == Qt::RightButton && d->peekedCard && d->peekedCard->pile() )
    {
        e->accept();
        updatePileLayout( d->peekedCard->pile(), DURATION_FANCYSHOW );
        d->peekedCard = 0;
    }
    else
    {
        bool hadCards = !cardsBeingDragged().isEmpty();
        KCardScene::mouseReleaseEvent( e );
        if ( hadCards && cardsBeingDragged().isEmpty() )
            emit cardsPutDown();
    }
}


void DealerScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent * e )
{
    stop();

    KCardScene::mouseDoubleClickEvent( e );
}


bool DealerScene::tryAutomaticMove( KCard * card )
{
    if ( !isCardAnimationRunning()
         && card
         && card->pile()
         && card == card->pile()->topCard()
         && card->isFaceUp()
         && allowedToRemove( card->pile(), card ) )
    {
        QList<KCard*> cardList = QList<KCard*>() << card;

        foreach ( PatPile * p, patPiles() )
        {
            if ( p->isFoundation() && allowedToAdd( p, cardList ) )
            {
                moveCardToPile( card , p, DURATION_MOVE );
                return true;
            }
        }
    }

    return false;
}


void DealerScene::undo()
{
    undoOrRedo( true );
}


void DealerScene::redo()
{
    undoOrRedo( false );
}


void DealerScene::undoOrRedo( bool undo )
{
    stop();

    if ( isCardAnimationRunning() )
        return;

    QStack<GameState*> & from = undo ? d->undoStack : d->redoStack;
    QStack<GameState*> & to = undo ? d->redoStack : d->undoStack;

    if ( !from.isEmpty() && d->currentState )
    {
        to.push( d->currentState );
        d->currentState = from.pop();
        setState( d->currentState );

        emit updateMoves( moveCount() );
        emit undoPossible( !d->undoStack.isEmpty() );
        emit redoPossible( !d->redoStack.isEmpty() );

        if ( d->toldAboutLostGame ) // everything's possible again
        {
            hintPossible( true );
            demoPossible( true );
            dropPossible( true );
            d->toldAboutLostGame = false;
            d->toldAboutWonGame = false;
        }

        if ( d->currentState->solvability == Solver::SolutionExists )
        {
            d->winMoves = d->currentState->winningMoves;
            emit solverStateChanged( i18n("Solver: This game is winnable.") );
        }
        else if ( d->currentState->solvability == Solver::NoSolutionExists )
        {
            d->winMoves.clear();
            if ( d->gameWasEverWinnable )
                emit solverStateChanged( i18n("Solver: This game is no longer winnable.") );
            else
                emit solverStateChanged( i18n("Solver: This game cannot be won.") );
        }
        else if ( d->currentState->solvability == Solver::UnableToDetermineSolvability )
        {
            d->winMoves.clear();
            emit solverStateChanged( i18n("Solver: Unable to determine if this game is winnable.") );
        }
        else
        {
            emit solverStateChanged( QString() );
            if ( d->solver )
                startSolver();
        }
    }
}


void DealerScene::takeState()
{
    if ( isCardAnimationRunning() )
    {
        d->takeStateQueued = true;
        return;
    }

    if ( !isDemoActive() )
        d->winMoves.clear();

    GameState * newState = getState();

    if ( d->currentState && *newState == *(d->currentState) )
    {
        delete newState;
        return;
    }

    if ( d->currentState )
    {
        d->undoStack.push( d->currentState );
        qDeleteAll( d->redoStack );
        d->redoStack.clear();
    }
    d->currentState = newState;

    emit redoPossible( false );
    emit undoPossible( !d->undoStack.isEmpty() );
    emit updateMoves( moveCount() );

    d->gameWasJustSaved = false;
    if ( isGameWon() )
    {
        won();
        return;
    }

    if ( !d->toldAboutWonGame && !d->toldAboutLostGame && isGameLost() )
    {
        emit hintPossible( false );
        emit demoPossible( false );
        emit dropPossible( false );
        emit solverStateChanged( i18n( "Solver: This game is lost." ) );
        d->toldAboutLostGame = true;
        stopDemo();
        return;
    }

    if ( !isDemoActive() && !isCardAnimationRunning() && d->solver )
        startSolver();

    if ( autoDropEnabled() && !isDropActive() && !isDemoActive() )
    {
        if ( d->interruptAutoDrop )
            d->interruptAutoDrop = false;
        else
            startDrop();
    }
}


GameState *DealerScene::getState()
{
    QSet<CardState> cardStates;
    cardStates.reserve( deck()->cards().size() );
    foreach (PatPile * p, patPiles())
    {
        foreach (KCard * c, p->cards())
        {
            CardState s;
            s.card = c;
            s.pile = c->pile();
            if (!s.pile) {
                kDebug() << c->objectName() << "has no valid parent.";
                Q_ASSERT(false);
                continue;
            }
            s.cardIndex = s.pile->indexOf( c );
            s.faceUp = c->isFaceUp();
            s.takenDown = d->cardsNotToDrop.contains( c );
            cardStates << s;
        }
    }

    return new GameState( cardStates, getGameState() );
}


void DealerScene::setState( GameState * state )
{
    d->cardsNotToDrop.clear();

    QSet<KCard*> cardsFromFoundations;
    foreach (PatPile *p, patPiles())
    {
        if ( p->isFoundation() )
            foreach (KCard *c, p->cards())
                cardsFromFoundations.insert( c );
        p->clear();
    }

    QMap<KCard*,int> cardIndices;
    foreach ( const CardState & s, state->cards )
    {
        KCard *c = s.card;
        c->setFaceUp( s.faceUp );
        s.pile->add( c );
        cardIndices.insert( c, s.cardIndex );

        PatPile * p = dynamic_cast<PatPile*>(c->pile());
        if ( s.takenDown || (cardsFromFoundations.contains( c ) && !(p && p->isFoundation())) )
            d->cardsNotToDrop.insert( c );
    }

    foreach( PatPile * p, patPiles() )
    {
        foreach ( KCard * c, p->cards() )
            p->swapCards( p->indexOf( c ), cardIndices.value( c ) );

        updatePileLayout( p, 0 );
    }

    // restore game-specific information
    setGameState( state->gameData );
}


void DealerScene::setSolverEnabled(bool a)
{
    d->solverEnabled = a;
}


void DealerScene::setAutoDropEnabled( bool enabled )
{
    d->autodropEnabled = enabled;
}


bool DealerScene::autoDropEnabled() const
{
    return d->autodropEnabled;
}


void DealerScene::startDrop()
{
    stopHint();
    stopDemo();

    if ( isCardAnimationRunning() )
    {
        d->dropQueued = true;
        return;
    }

    d->dropInProgress = true;
    d->interruptAutoDrop = false;
    d->dropSpeedFactor = 1;
    emit dropActive( true );

    drop();
}


void DealerScene::stopDrop()
{
    if ( d->dropInProgress )
    {
        d->dropTimer->stop();
        d->dropInProgress = false;
        emit dropActive( false );

        if ( autoDropEnabled() && d->takeStateQueued )
            d->interruptAutoDrop = true;
    }
}


bool DealerScene::isDropActive() const
{
    return d->dropInProgress;
}


bool DealerScene::drop()
{
    foreach ( const MoveHint & mh, getHints() )
    {
        if ( mh.pile()
             && mh.pile()->isFoundation()
             && mh.priority() > 120
             && !d->cardsNotToDrop.contains( mh.card() ) )
        {
            QList<KCard*> cards = mh.card()->pile()->topCardsDownTo( mh.card() );

            QMap<KCard*,QPointF> oldPositions;
            foreach ( KCard * c, cards )
                oldPositions.insert( c, c->pos() );

            moveCardsToPile( cards, mh.pile(), DURATION_MOVE );

            int count = 0;
            foreach ( KCard * c, cards )
            {
                c->completeAnimation();
                QPointF destPos = c->pos();
                c->setPos( oldPositions.value( c ) );

                int duration = speedUpTime( DURATION_AUTODROP + count * DURATION_AUTODROP / 10 );
                c->animate( destPos, c->zValue(), 0, c->isFaceUp(), true, duration );

                ++count;
            }

            d->dropSpeedFactor *= AUTODROP_SPEEDUP_FACTOR;

            takeState();

            return true;
        }
    }

    d->dropInProgress = false;
    emit dropActive( false );

    return false;
}

int DealerScene::speedUpTime( int delay ) const
{
    if ( delay < DURATION_AUTODROP_MINIMUM )
        return delay;
    else
        return qMax<int>( delay * d->dropSpeedFactor, DURATION_AUTODROP_MINIMUM );
}

void DealerScene::stopAndRestartSolver()
{
    if ( d->toldAboutLostGame || d->toldAboutWonGame ) // who cares?
        return;

    if ( d->solverThread && d->solverThread->isRunning() )
    {
        d->solverThread->abort();
    }

    if ( isCardAnimationRunning() )
    {
        startSolver();
        return;
    }

    slotSolverEnded();
}

void DealerScene::slotSolverEnded()
{
    if ( d->solverThread && d->solverThread->isRunning() )
        return;

    d->solver->translate_layout();
    d->winMoves.clear();
    emit solverStateChanged( i18n("Solver: Calculating...") );
    if ( !d->solverThread ) {
        d->solverThread = new SolverThread( d->solver );
        connect( d->solverThread, SIGNAL(finished(int)), this, SLOT(slotSolverFinished(int)));
    }
    d->solverThread->start( d->solverEnabled ? QThread::IdlePriority : QThread::NormalPriority );
}

void DealerScene::slotSolverFinished( int result )
{
    switch ( result )
    {
    case Solver::SolutionExists:
        d->winMoves = d->solver->winMoves;
        d->gameWasEverWinnable = true;
        emit solverStateChanged( i18n("Solver: This game is winnable.") );
        break;
    case Solver::NoSolutionExists:
        if ( d->gameWasEverWinnable )
            emit solverStateChanged( i18n("Solver: This game is no longer winnable.") );
        else
            emit solverStateChanged( i18n("Solver: This game cannot be won.") );
        break;
    case Solver::UnableToDetermineSolvability:
        emit solverStateChanged( i18n("Solver: Unable to determine if this game is winnable.") );
        break;
    case Solver::SearchAborted:
        startSolver();
        break;
    case Solver::MemoryLimitReached:
        break;
    }

    if ( d->currentState )
    {
        d->currentState->solvability = static_cast<Solver::ExitStatus>( result );
        if ( result == Solver::SolutionExists )
            d->currentState->winningMoves = d->winMoves;
    }
}


int DealerScene::gameNumber() const
{
    return d->gameNumber;
}


void DealerScene::stop()
{
    stopHint();
    stopDemo();
    stopDrop();
}


void DealerScene::animationDone()
{
    Q_ASSERT( !isCardAnimationRunning() );

    if ( !d->multiStepMoves.isEmpty() )
    {
        continueMultiStepMove();
        return;
    }

    if ( d->takeStateQueued )
    {
        d->takeStateQueued = false;
        takeState();
    }

    if ( d->demoInProgress )
    {
        d->demoTimer->start( TIME_BETWEEN_MOVES );
    }
    else if ( d->dropInProgress )
    {
        d->dropTimer->start( speedUpTime( TIME_BETWEEN_MOVES ) );
    }
    else if ( d->newCardsQueued )
    {
        d->newCardsQueued = false;
        newCards();
    }
    else if ( d->hintQueued )
    {
        d->hintQueued = false;
        startHint();
    }
    else if ( d->demoQueued )
    {
        d->demoQueued = false;
        startDemo();
    }
    else if ( d->dropQueued )
    {
        d->dropQueued = false;
        startDrop();
    }
}


void DealerScene::startDemo()
{
    stopHint();
    stopDrop();

    if ( isCardAnimationRunning() )
    {
        d->demoQueued = true;
        return;
    }

    d->demoInProgress = true;
    d->playerReceivedHelp = true;
    d->gameStarted = true;

    demo();
}


void DealerScene::stopDemo()
{
    if ( d->demoInProgress )
    {
        d->demoTimer->stop();
        d->demoInProgress = false;
        emit demoActive( false );
    }
}


bool DealerScene::isDemoActive() const
{
    return d->demoInProgress;
}


void DealerScene::demo()
{
    if ( isCardAnimationRunning() )
    {
        d->demoQueued = true;
        return;
    }

    d->demoInProgress = true;
    d->playerReceivedHelp = true;
    d->gameStarted = true;
    clearHighlightedItems();

    d->demoTimer->stop();

    MoveHint mh = chooseHint();
    if ( mh.isValid() )
    {
        kDebug(mh.pile()->topCard()) << "Moving" << mh.card()->objectName()
                                 << "from the" << mh.card()->pile()->objectName()
                                 << "pile to the" << mh.pile()->objectName()
                                 << "pile, putting it on top of" << mh.pile()->topCard()->objectName();
        kDebug(!mh.pile()->topCard()) << "Moving" << mh.card()->objectName()
                                  << "from the" << mh.card()->pile()->objectName()
                                  << "pile to the" << mh.pile()->objectName()
                                  << "pile, which is empty";
        Q_ASSERT(mh.card()->pile() == 0
                 || allowedToRemove(mh.card()->pile(), mh.card()));

        QList<KCard*> empty;
        QList<KCard*> cards = mh.card()->pile()->cards();
        bool after = false;
        for (QList<KCard*>::Iterator it = cards.begin(); it != cards.end(); ++it) {
            if (*it == mh.card())
                after = true;
            if (after)
                empty.append(*it);
        }

        Q_ASSERT(!empty.isEmpty());

        QMap<KCard*,QPointF> oldPositions;

        foreach (KCard *c, empty) {
            c->completeAnimation();
            c->setFaceUp(true);
            oldPositions.insert(c, c->pos());
        }

        Q_ASSERT(mh.card());
        Q_ASSERT(mh.card()->pile());
        Q_ASSERT(mh.pile());
        Q_ASSERT(mh.card()->pile() != mh.pile());
        Q_ASSERT(mh.pile()->isFoundation() || allowedToAdd(mh.pile(), empty));

        moveCardsToPile( empty, mh.pile(), DURATION_MOVE );

        foreach (KCard *c, empty) {
            c->completeAnimation();
            QPointF destPos = c->pos();
            c->setPos(oldPositions.value(c));
            c->animate(destPos, c->zValue(), 0, c->isFaceUp(), true, DURATION_DEMO);
        }
    }
    else if ( !newCards() )
    {
        if (isGameWon())
        {
            won();
        }
        else
        {
            stopDemo();
            slotSolverEnded();
        }
        return;
    }

    emit demoActive( true );
    takeState();
}


void DealerScene::drawDealRowOrRedeal()
{
    stop();

    if ( isCardAnimationRunning() )
    {
        d->newCardsQueued = true;
        return;
    }

    d->newCardsQueued = false;
    newCards();
}


bool DealerScene::newCards()
{
    return false;
}


void DealerScene::setSolver( Solver *s) {
    delete d->solver;
    delete d->solverThread;
    d->solver = s;
    d->solverThread = 0;
}

bool DealerScene::isGameWon() const
{
    foreach (PatPile *p, patPiles())
    {
        if (!p->isFoundation() && !p->isEmpty())
            return false;
    }
    return true;
}

void DealerScene::startSolver() const
{
    if( d->solverEnabled )
        d->updateSolver->start();
}


bool DealerScene::isGameLost() const
{
    if ( solver() )
    {
        if ( d->solverThread && d->solverThread->isRunning() )
            d->solverThread->abort();

        solver()->translate_layout();
        return solver()->patsolve( neededFutureMoves() ) == Solver::NoSolutionExists;
    }
    return false;
}

void DealerScene::recordGameStatistics()
{
    // Don't record the game if it was never started, if it is unchanged since
    // it was last saved (allowing the user to close KPat after saving without
    // it recording a loss) or if it has already been recorded.//         takeState(); // copying it again
    if ( d->gameStarted && !d->gameWasJustSaved && !d->gameRecorded )
    {
        int id = oldId();

        QString totalPlayedKey = QString("total%1").arg( id );
        QString wonKey = QString("won%1").arg( id );
        QString winStreakKey = QString("winstreak%1").arg( id );
        QString maxWinStreakKey = QString("maxwinstreak%1").arg( id );
        QString loseStreakKey = QString("loosestreak%1").arg( id );
        QString maxLoseStreakKey = QString("maxloosestreak%1").arg( id );

        KConfigGroup config(KGlobal::config(), scores_group);

        int totalPlayed = config.readEntry( totalPlayedKey, 0 );
        int won = config.readEntry( wonKey, 0 );
        int winStreak = config.readEntry( winStreakKey, 0 );
        int maxWinStreak = config.readEntry( maxWinStreakKey, 0 );
        int loseStreak = config.readEntry( loseStreakKey, 0 );
        int maxLoseStreak = config.readEntry( maxLoseStreakKey, 0 );

        ++totalPlayed;

        if ( d->gameHasBeenWon )
        {
            ++won;
            ++winStreak;
            maxWinStreak = qMax( winStreak, maxWinStreak );
            loseStreak = 0;
        }
        else
        {
            ++loseStreak;
            maxLoseStreak = qMax( loseStreak, maxLoseStreak );
            winStreak = 0;
        }

        config.writeEntry( totalPlayedKey, totalPlayed );
        config.writeEntry( wonKey, won );
        config.writeEntry( winStreakKey, winStreak );
        config.writeEntry( maxWinStreakKey, maxWinStreak );
        config.writeEntry( loseStreakKey, loseStreak );
        config.writeEntry( maxLoseStreakKey, maxLoseStreak );

        d->gameRecorded = true;
    }
}

void DealerScene::relayoutScene()
{
    KCardScene::relayoutScene();

    if ( d->wonItem->isVisible() )
        updateWonItem();
}

void DealerScene::setGameId(int id) { d->gameId = id; }
int DealerScene::gameId() const { return d->gameId; }

void DealerScene::setActions(int actions) { d->actions = actions; }
int DealerScene::actions() const { return d->actions; }

QList<QAction*> DealerScene::configActions() const { return QList<QAction*>(); }

Solver *DealerScene::solver() const { return d->solver; }

int DealerScene::neededFutureMoves() const { return d->neededFutureMoves; }
void DealerScene::setNeededFutureMoves( int i ) { d->neededFutureMoves = i; }


void DealerScene::setDeckContents( int copies, const QList<KCardDeck::Suit> & suits )
{
    Q_ASSERT( copies >= 1 );
    Q_ASSERT( !suits.isEmpty() );

    // Note that the order in which the cards are created can not be changed
    // without breaking the game numbering. For historical reasons, KPat
    // generates card by rank and then by suit, rather than the more common
    // suit then rank ordering.
    QList<quint32> ids;
    for ( int i = 0; i < copies; ++i )
        foreach ( const KCardDeck::Rank & r, KCardDeck::standardRanks() )
            foreach ( const KCardDeck::Suit & s, suits )
                ids << KCardDeck::getId( s, r );

    deck()->setDeckContents( ids );
}


QString DealerScene::save_it()
{
    // If the game has been won, there's no current state to save.
    if ( d->gameHasBeenWon )
        return QString();

    KTemporaryFile file;
    file.setAutoRemove(false);
    file.open();
    QDomDocument doc("kpat");
    saveGame(doc);
    QTextStream stream (&file);
    stream << doc.toString();
    stream.flush();
    file.flush();
    // .close will make fileName() return ""
    QString filename = file.fileName();
    file.close();
    return filename;
}

QImage DealerScene::createDump() const
{
    const QSize previewSize( 480, 320 );

    foreach ( KCard * c, deck()->cards() )
        c->completeAnimation();

    QMultiMap<qreal,QGraphicsItem*> itemsByZ;
    foreach ( QGraphicsItem * item, items() )
    {
        Q_ASSERT( item->zValue() >= 0 );
        itemsByZ.insert( item->zValue(), item );
    }

    QImage img( contentArea().size().toSize(), QImage::Format_ARGB32 );
    img.fill( Qt::transparent );
    QPainter p( &img );

    foreach ( QGraphicsItem * item, itemsByZ )
    {
        if ( item->isVisible() )
        {
            p.save();
            p.setTransform( item->deviceTransform( p.worldTransform() ), false );
            item->paint( &p, 0 );
            p.restore();
        }
    }

    p.end();

    img = img.scaled( previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation );

    QImage img2( previewSize, QImage::Format_ARGB32 );
    img2.fill( Qt::transparent );
    QPainter p2( &img2 );
    p2.drawImage( (img2.width() - img.width()) / 2, (img2.height() - img.height()) / 2, img );
    p2.end();

    return img2;
}


void DealerScene::mapOldId( int id )
{
    Q_UNUSED( id );
}


int DealerScene::oldId() const
{
    return gameId();
}


QString DealerScene::getGameState() const
{
    return QString();
}


void DealerScene::setGameState( const QString & state )
{
    Q_UNUSED( state );
}


QString DealerScene::getGameOptions() const
{
    return QString();
}


void DealerScene::setGameOptions( const QString & options )
{
    Q_UNUSED( options );
}


bool DealerScene::allowedToStartNewGame()
{
    // Check if the user is already running a game, and if she is,
    // then ask if she wants to abort it.
    return !d->gameStarted
           || d->gameWasJustSaved
           || d->toldAboutWonGame
           || d->toldAboutLostGame
           || KMessageBox::warningContinueCancel(0,
                     i18n("A new game has been requested, but there is already a game in progress.\n\n"
                          "A loss will be recorded in the statistics if the current game is abandoned."),
                     i18n("Abandon Current Game?"),
                     KGuiItem(i18n("Abandon Current Game")),
                     KStandardGuiItem::cancel(),
                     "careaboutstats"
                    ) == KMessageBox::Continue;
}

void DealerScene::addCardForDeal( KCardPile * pile, KCard * card, bool faceUp, QPointF startPos )
{
    Q_ASSERT( card );
    Q_ASSERT( pile );

    card->setFaceUp( faceUp );
    pile->add( card );
    d->initDealPositions.insert( card, startPos );
}


void DealerScene::startDealAnimation()
{
    qreal speed = sqrt( width() * width() + height() * height() ) / ( DURATION_DEAL );
    foreach ( PatPile * p, patPiles() )
    {
        updatePileLayout( p, 0 );
        foreach ( KCard * c, p->cards() )
        {
            if ( !d->initDealPositions.contains( c ) )
                continue;

            QPointF pos2 = c->pos();
            c->setPos( d->initDealPositions.value( c ) );

            QPointF delta = c->pos() - pos2;
            qreal dist = sqrt( delta.x() * delta.x() + delta.y() * delta.y() );
            int duration = qRound( dist / speed );
            c->animate( pos2, c->zValue(), 0, c->isFaceUp(), false, duration );
        }
    }
    d->initDealPositions.clear();
}


void DealerScene::multiStepMove( const QList<KCard*> & cards,
                                 KCardPile * pile,
                                 const QList<KCardPile*> & freePiles,
                                 const QList<KCardPile*> & freeCells,
                                 int duration )
{
    Q_ASSERT( cards.size() == 1 || !freePiles.isEmpty() || !freeCells.isEmpty() );

    d->multiStepMoves.clear();
    d->multiStepDuration = duration;

    multiStepSubMove( cards, pile, freePiles, freeCells );
    continueMultiStepMove();
}


void DealerScene::multiStepSubMove( QList<KCard*> cards,
                                    KCardPile * pile,
                                    QList<KCardPile*> freePiles,
                                    const QList<KCardPile*> & freeCells )
{
    // Note that cards and freePiles are passed by value, as we need to make a
    // local copy anyway.

    // Using n free cells, we can move a run of n+1 cards. If we want to move
    // more than that, we have to recursively move some of our cards to one of
    // the free piles temporarily.
    const int freeCellsPlusOne = freeCells.size() + 1;
    int cardsToSubMove = cards.size() - freeCellsPlusOne;

    QList<QPair<KCardPile*,QList<KCard*> > > tempMoves;
    while ( cardsToSubMove > 0 )
    {
        int tempMoveSize;
        if ( cardsToSubMove <= freePiles.size() * freeCellsPlusOne )
        {
            // If the cards that have to be submoved can be spread across the
            // the free piles without putting more than freeCellsPlusOne cards
            // on each one, we do so. This means that none of our submoves will
            // need further submoves, which keeps the total move count down. We
            // Just to a simple rounding up integer division.
            tempMoveSize = (cardsToSubMove + freePiles.size() - 1) / freePiles.size();
        }
        else
        {
            // Otherwise, we use the space optimal method that gets the cards
            // moved using a minimal number of piles, but uses more submoves.
            tempMoveSize = freeCellsPlusOne;
            while ( tempMoveSize * 2 < cardsToSubMove )
                tempMoveSize *= 2;
        }

        QList<KCard*> subCards;
        for ( int i = 0; i < tempMoveSize; ++i )
            subCards.prepend( cards.takeLast() );

        Q_ASSERT( !freePiles.isEmpty() );
        KCardPile * nextPile = freePiles.takeFirst();

        tempMoves << qMakePair( nextPile, subCards );
        multiStepSubMove( subCards, nextPile, freePiles, freeCells );

        cardsToSubMove -= tempMoveSize;
    }

    // Move cards to free cells.
    for ( int i = 0; i < cards.size() - 1; ++i )
    {
        KCard * c = cards.at( cards.size() - 1 - i );
        d->multiStepMoves << qMakePair( c, freeCells[i] );
    }

    // Move bottom card to destination pile.
    d->multiStepMoves << qMakePair( cards.first(), pile );

    // Move cards from free cells to destination pile.
    for ( int i = 1; i < cards.size(); ++i )
        d->multiStepMoves << qMakePair( cards.at( i ), pile );

    // If we just moved the bottomost card of the source pile, it must now be
    // empty and we won't need it any more. So we return it to the list of free
    // piles.
    KCardPile * sourcePile = cards.first()->pile();
    if ( sourcePile->at( 0 ) == cards.first() )
        freePiles << sourcePile;

    // If we had to do any submoves, we now move those cards from their
    // temporary pile to the destination pile and free up their temporary pile.
    while ( !tempMoves.isEmpty() )
    {
        QPair<KCardPile*, QList<KCard*> > m = tempMoves.takeLast();
        multiStepSubMove( m.second, pile, freePiles, freeCells );
        freePiles << m.first;
    }
}


void DealerScene::continueMultiStepMove()
{
    Q_ASSERT( !d->multiStepMoves.isEmpty() );
    Q_ASSERT( !isCardAnimationRunning() );

    QPair<KCard*,KCardPile*> m = d->multiStepMoves.takeFirst();
    KCard * card = m.first;
    KCardPile * dest = m.second;
    KCardPile * source = card->pile();

    Q_ASSERT( card == source->topCard() );
    Q_ASSERT( allowedToAdd( dest, QList<KCard*>() << card ) );

    d->multiStepDuration = qMax<int>( d->multiStepDuration * 0.9, 50 );

    dest->add( card );
    card->raise();
    updatePileLayout( dest, d->multiStepDuration );
    updatePileLayout( source, d->multiStepDuration );

    if ( d->multiStepMoves.isEmpty() )
        takeState();
}


#include "dealer.moc"
#include "moc_dealer.cpp"
