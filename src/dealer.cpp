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

#include "dealer.h"

// own
#include "dealerinfo.h"
#include "kpat_debug.h"
#include "messagebox.h"
#include "patsolve/solverinterface.h"
#include "renderer.h"
#include "shuffle.h"
// KCardGame
#include <KCardTheme>
// KF
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
// Qt
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QRandomGenerator>
#include <QThread>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
// Std
#include <cmath>

namespace
{
const qreal wonBoxToSceneSizeRatio = 0.7;

QString solverStatusMessage(int status, bool everWinnable)
{
    switch (status) {
    case SolverInterface::SolutionExists:
        return i18n("Solver: This game is winnable.");
    case SolverInterface::NoSolutionExists:
        return everWinnable ? i18n("Solver: This game is no longer winnable.") : i18n("Solver: This game cannot be won.");
    case SolverInterface::UnableToDetermineSolvability:
        return i18n("Solver: Unable to determine if this game is winnable.");
    case SolverInterface::SearchAborted:
    case SolverInterface::MemoryLimitReached:
    default:
        return QString();
    }
}

int readIntAttribute(const QXmlStreamReader &xml, const QString &key, bool *ok = nullptr)
{
    QStringView value = xml.attributes().value(key);
    return QString::fromRawData(value.data(), value.length()).toInt(ok);
}

QString suitToString(int suit)
{
    switch (suit) {
    case KCardDeck::Clubs:
        return QStringLiteral("clubs");
    case KCardDeck::Diamonds:
        return QStringLiteral("diamonds");
    case KCardDeck::Hearts:
        return QStringLiteral("hearts");
    case KCardDeck::Spades:
        return QStringLiteral("spades");
    default:
        return QString();
    }
}

QString rankToString(int rank)
{
    switch (rank) {
    case KCardDeck::Ace:
        return QStringLiteral("ace");
    case KCardDeck::Two:
        return QStringLiteral("two");
    case KCardDeck::Three:
        return QStringLiteral("three");
    case KCardDeck::Four:
        return QStringLiteral("four");
    case KCardDeck::Five:
        return QStringLiteral("five");
    case KCardDeck::Six:
        return QStringLiteral("six");
    case KCardDeck::Seven:
        return QStringLiteral("seven");
    case KCardDeck::Eight:
        return QStringLiteral("eight");
    case KCardDeck::Nine:
        return QStringLiteral("nine");
    case KCardDeck::Ten:
        return QStringLiteral("ten");
    case KCardDeck::Jack:
        return QStringLiteral("jack");
    case KCardDeck::Queen:
        return QStringLiteral("queen");
    case KCardDeck::King:
        return QStringLiteral("king");
    default:
        return QString();
    }
}
}

class SolverThread : public QThread
{
    Q_OBJECT

public:
    SolverThread(SolverInterface *solver)
        : m_solver(solver)
    {
    }

    void run() override
    {
        SolverInterface::ExitStatus result = m_solver->patsolve();
        Q_EMIT finished(result);
    }

    void abort()
    {
        m_solver->stopExecution();
        wait();
    }

Q_SIGNALS:
    void finished(int result);

private:
    SolverInterface *m_solver;
};

int DealerScene::moveCount() const
{
    return m_loadedMoveCount + m_undoStack.size();
}

void DealerScene::saveLegacyFile(QIODevice *io)
{
    QXmlStreamWriter xml(io);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(-1);
    xml.writeStartDocument();

    xml.writeDTD(QStringLiteral("<!DOCTYPE kpat>"));

    xml.writeStartElement(QStringLiteral("dealer"));
    xml.writeAttribute(QStringLiteral("id"), QString::number(gameId()));
    xml.writeAttribute(QStringLiteral("options"), getGameOptions());
    xml.writeAttribute(QStringLiteral("number"), QString::number(gameNumber()));
    xml.writeAttribute(QStringLiteral("moves"), QString::number(moveCount()));
    xml.writeAttribute(QStringLiteral("started"), QString::number(m_dealStarted));
    xml.writeAttribute(QStringLiteral("data"), getGameState());

    const auto patPiles = this->patPiles();
    for (const PatPile *p : patPiles) {
        xml.writeStartElement(QStringLiteral("pile"));
        xml.writeAttribute(QStringLiteral("index"), QString::number(p->index()));
        xml.writeAttribute(QStringLiteral("z"), QString::number(p->zValue()));

        const auto cards = p->cards();
        for (const KCard *c : cards) {
            xml.writeStartElement(QStringLiteral("card"));
            xml.writeAttribute(QStringLiteral("suit"), QString::number(c->suit()));
            xml.writeAttribute(QStringLiteral("value"), QString::number(c->rank()));
            xml.writeAttribute(QStringLiteral("faceup"), QString::number(c->isFaceUp()));
            xml.writeAttribute(QStringLiteral("z"), QString::number(c->zValue()));
            xml.writeEndElement();
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndDocument();

    m_dealWasJustSaved = true;
}

bool DealerScene::loadLegacyFile(QIODevice *io)
{
    resetInternals();

    QXmlStreamReader xml(io);

    xml.readNextStartElement();

    // Before KDE4.3, KPat didn't store game specific options in the save
    // file. This could cause crashes when loading a Spider game with a
    // different number of suits than the current setting. Similarly, in
    // Klondike the number of cards drawn from the deck was forgotten, but
    // this never caused crashes. Fortunately, in Spider we can count the
    // number of suits ourselves. For Klondike, there is no way to recover
    // that information.
    QString options = xml.attributes().value(QStringLiteral("options")).toString();
    if (gameId() == 17 && options.isEmpty()) {
        QSet<int> suits;
        while (!xml.atEnd())
            if (xml.readNextStartElement() && xml.name() == QLatin1String("card"))
                suits << readIntAttribute(xml, QStringLiteral("suit"));
        options = QString::number(suits.size());

        // "Rewind" back to the <dealer> tag. Yes, this is ugly.
        xml.clear();
        io->reset();
        xml.setDevice(io);
        xml.readNextStartElement();
    }
    setGameOptions(options);

    m_dealNumber = readIntAttribute(xml, QStringLiteral("number"));
    m_loadedMoveCount = readIntAttribute(xml, QStringLiteral("moves"));
    m_dealStarted = readIntAttribute(xml, QStringLiteral("started"));
    QString gameStateData = xml.attributes().value(QStringLiteral("data")).toString();

    QMultiHash<quint32, KCard *> cards;
    const auto deckCards = deck()->cards();
    for (KCard *c : deckCards)
        cards.insert((c->id() & 0xffff), c);

    QHash<int, PatPile *> piles;
    const auto patPiles = this->patPiles();
    for (PatPile *p : patPiles)
        piles.insert(p->index(), p);

    // Loop through <pile>s.
    while (xml.readNextStartElement()) {
        if (xml.name() != QLatin1String("pile")) {
            qCWarning(KPAT_LOG) << "Expected a \"pile\" tag. Found a" << xml.name() << "tag.";
            return false;
        }

        bool ok;
        int index = readIntAttribute(xml, QStringLiteral("index"), &ok);
        QHash<int, PatPile *>::const_iterator it = piles.constFind(index);
        if (!ok || it == piles.constEnd()) {
            qCWarning(KPAT_LOG) << "Unrecognized pile index:" << xml.attributes().value(QStringLiteral("index"));
            return false;
        }

        PatPile *p = it.value();
        p->clear();

        // Loop through <card>s.
        while (xml.readNextStartElement()) {
            if (xml.name() != QLatin1String("card")) {
                qCWarning(KPAT_LOG) << "Expected a \"card\" tag. Found a" << xml.name() << "tag.";
                return false;
            }

            bool suitOk, rankOk, faceUpOk;
            int suit = readIntAttribute(xml, QStringLiteral("suit"), &suitOk);
            int rank = readIntAttribute(xml, QStringLiteral("value"), &rankOk);
            bool faceUp = readIntAttribute(xml, QStringLiteral("faceup"), &faceUpOk);

            quint32 id = KCardDeck::getId(KCardDeck::Suit(suit), KCardDeck::Rank(rank), 0);
            KCard *card = cards.take(id);

            if (!suitOk || !rankOk || !faceUpOk || !card) {
                qCWarning(KPAT_LOG) << "Unrecognized card: suit=" << xml.attributes().value(QStringLiteral("suit"))
                                    << " value=" << xml.attributes().value(QStringLiteral("value"))
                                    << " faceup=" << xml.attributes().value(QStringLiteral("faceup"));
                return false;
            }

            card->setFaceUp(faceUp);
            p->add(card);

            xml.skipCurrentElement();
        }
        updatePileLayout(p, 0);
    }

    setGameState(gameStateData);

    Q_EMIT updateMoves(moveCount());
    takeState();

    return true;
}

void DealerScene::saveFile(QIODevice *io)
{
    QXmlStreamWriter xml(io);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(-1);
    xml.writeStartDocument();

    xml.writeStartElement(QStringLiteral("kpat-game"));
    xml.writeAttribute(QStringLiteral("game-type"), m_di->baseIdString());
    if (!getGameOptions().isEmpty())
        xml.writeAttribute(QStringLiteral("game-type-options"), getGameOptions());
    xml.writeAttribute(QStringLiteral("deal-number"), QString::number(gameNumber()));

    if (!m_currentState) {
        deck()->stopAnimations();
        takeState();
    }

    QList<GameState *> allStates;
    for (int i = 0; i < m_undoStack.size(); ++i)
        allStates << m_undoStack.at(i);
    allStates << m_currentState;
    for (int i = m_redoStack.size() - 1; i >= 0; --i)
        allStates << m_redoStack.at(i);

    QString lastGameSpecificState;

    for (int i = 0; i < allStates.size(); ++i) {
        const GameState *state = allStates.at(i);
        xml.writeStartElement(QStringLiteral("state"));
        if (state->stateData != lastGameSpecificState) {
            xml.writeAttribute(QStringLiteral("game-specific-state"), state->stateData);
            lastGameSpecificState = state->stateData;
        }
        if (i == m_undoStack.size())
            xml.writeAttribute(QStringLiteral("current"), QStringLiteral("true"));

        for (const CardStateChange &change : std::as_const(state->changes)) {
            xml.writeStartElement(QStringLiteral("move"));
            xml.writeAttribute(QStringLiteral("pile"), change.newState.pile->objectName());
            xml.writeAttribute(QStringLiteral("position"), QString::number(change.newState.index));

            bool faceChanged = !change.oldState.pile || change.oldState.faceUp != change.newState.faceUp;

            for (const KCard *card : change.cards) {
                xml.writeStartElement(QStringLiteral("card"));
                xml.writeAttribute(QStringLiteral("id"), QStringLiteral("%1").arg(card->id(), 7, 10, QLatin1Char('0')));
                xml.writeAttribute(QStringLiteral("suit"), suitToString(card->suit()));
                xml.writeAttribute(QStringLiteral("rank"), rankToString(card->rank()));
                if (faceChanged)
                    xml.writeAttribute(QStringLiteral("turn"), change.newState.faceUp ? QStringLiteral("face-up") : QStringLiteral("face-down"));
                xml.writeEndElement();
            }

            xml.writeEndElement();
        }
        xml.writeEndElement();
    }

    xml.writeEndElement();
    xml.writeEndDocument();

    m_dealWasJustSaved = true;
}

// arg_takeState is default true, but we disable it for command line usage of this function
bool DealerScene::loadFile(QIODevice *io, bool arg_takeState)
{
    resetInternals();

    bool reenableAutoDrop = autoDropEnabled();
    setAutoDropEnabled(false);

    QXmlStreamReader xml(io);

    xml.readNextStartElement();

    if (xml.name() != QLatin1String("kpat-game")) {
        qCWarning(KPAT_LOG) << "First tag is not \"kpat-game\"";
        return false;
    }

    m_dealNumber = readIntAttribute(xml, QStringLiteral("deal-number"));
    setGameOptions(xml.attributes().value(QStringLiteral("game-type-options")).toString());

    QMultiHash<quint32, KCard *> cardHash;
    const auto cards = deck()->cards();
    for (KCard *c : cards)
        cardHash.insert(c->id(), c);

    QHash<QString, KCardPile *> pileHash;
    const auto piles = this->piles();
    for (KCardPile *p : piles)
        pileHash.insert(p->objectName(), p);

    int undosToDo = -1;

    while (xml.readNextStartElement()) {
        if (xml.name() != QLatin1String("state")) {
            qCWarning(KPAT_LOG) << "Expected a \"state\" tag. Found a" << xml.name() << "tag.";
            return false;
        }

        if (xml.attributes().hasAttribute(QStringLiteral("game-specific-state")))
            setGameState(xml.attributes().value(QStringLiteral("game-specific-state")).toString());

        if (undosToDo > -1)
            ++undosToDo;
        else if (xml.attributes().value(QStringLiteral("current")) == QLatin1String("true"))
            undosToDo = 0;

        while (xml.readNextStartElement()) {
            if (xml.name() != QLatin1String("move")) {
                qCWarning(KPAT_LOG) << "Expected a \"move\" tag. Found a" << xml.name() << "tag.";
                return false;
            }

            QString pileName = xml.attributes().value(QStringLiteral("pile")).toString();
            KCardPile *pile = pileHash.value(pileName);

            bool indexOk;
            int index = readIntAttribute(xml, QStringLiteral("position"), &indexOk);

            if (!pile || !indexOk) {
                qCWarning(KPAT_LOG) << "Unrecognized pile or index.";
                return false;
            }

            while (xml.readNextStartElement()) {
                if (xml.name() != QLatin1String("card")) {
                    qCWarning(KPAT_LOG) << "Expected a \"card\" tag. Found a" << xml.name() << "tag.";
                    return false;
                }

                KCard *card = cardHash.value(readIntAttribute(xml, QStringLiteral("id")));
                if (!card) {
                    qCWarning(KPAT_LOG) << "Unrecognized card.";
                    return false;
                }

                if (xml.attributes().value(QStringLiteral("turn")) == QLatin1String("face-up"))
                    card->setFaceUp(true);
                else if (xml.attributes().value(QStringLiteral("turn")) == QLatin1String("face-down"))
                    card->setFaceUp(false);

                pile->insert(index, card);

                ++index;
                xml.skipCurrentElement();
            }
        }
        if (arg_takeState) {
            takeState();
        }
    }

    m_loadedMoveCount = 0;
    m_dealStarted = moveCount() > 0;
    Q_EMIT updateMoves(moveCount());

    while (undosToDo > 0) {
        undo();
        --undosToDo;
    }

    for (KCardPile *p : piles)
        updatePileLayout(p, 0);

    setAutoDropEnabled(reenableAutoDrop);

    return true;
}

DealerScene::DealerScene(const DealerInfo *di)
    : m_di(di)
    , m_solver(nullptr)
    , m_solverThread(nullptr)
    , m_peekedCard(nullptr)
    , m_dealNumber(0)
    , m_loadedMoveCount(0)
    , m_neededFutureMoves(1)
    , m_supportedActions(0)
    , m_autoDropEnabled(false)
    , m_solverEnabled(false)
    , m_dealStarted(false)
    , m_dealWasEverWinnable(false)
    , m_dealHasBeenWon(false)
    , m_dealWasJustSaved(false)
    , m_statisticsRecorded(false)
    , m_playerReceivedHelp(false)
    , m_toldAboutWonGame(false)
    , m_toldAboutLostGame(false)
    , m_dropSpeedFactor(1)
    , m_interruptAutoDrop(false)
    , m_dealInProgress(false)
    , m_hintInProgress(false)
    , m_demoInProgress(false)
    , m_dropInProgress(false)
    , m_hintQueued(false)
    , m_demoQueued(false)
    , m_dropQueued(false)
    , m_newCardsQueued(false)
    , m_takeStateQueued(false)
    , m_currentState(nullptr)
{
    setItemIndexMethod(QGraphicsScene::NoIndex);

    m_solverUpdateTimer.setInterval(250);
    m_solverUpdateTimer.setSingleShot(true);
    connect(&m_solverUpdateTimer, &QTimer::timeout, this, &DealerScene::stopAndRestartSolver);

    m_demoTimer.setSingleShot(true);
    connect(&m_demoTimer, &QTimer::timeout, this, &DealerScene::demo);

    m_dropTimer.setSingleShot(true);
    connect(&m_dropTimer, &QTimer::timeout, this, &DealerScene::drop);

    m_wonItem = new MessageBox();
    m_wonItem->setZValue(2000);
    m_wonItem->hide();
    addItem(m_wonItem);

    connect(this, &DealerScene::cardAnimationDone, this, &DealerScene::animationDone);

    connect(this, &DealerScene::cardDoubleClicked, this, &DealerScene::tryAutomaticMove);
    // Make rightClick == doubleClick. See bug #151921
    connect(this, &DealerScene::cardRightClicked, this, &DealerScene::tryAutomaticMove);
}

DealerScene::~DealerScene()
{
    stop();

    disconnect();
    if (m_solverThread)
        m_solverThread->abort();
    delete m_solverThread;
    m_solverThread = nullptr;
    delete m_solver;
    m_solver = nullptr;
    qDeleteAll(m_undoStack);
    delete m_currentState;
    qDeleteAll(m_redoStack);
    delete m_wonItem;
}

void DealerScene::addPatPile(PatPile *pile)
{
    if (!m_patPiles.contains(pile))
        m_patPiles.append(pile);
}

void DealerScene::removePatPile(PatPile *pile)
{
    m_patPiles.removeAll(pile);
}

void DealerScene::clearPatPiles()
{
    const auto patPiles = this->patPiles();
    for (PatPile *p : patPiles) {
        removePatPile(p);
        removePile(p);
    }
}

QList<PatPile *> DealerScene::patPiles() const
{
    return m_patPiles;
}

void DealerScene::cardsMoved(const QList<KCard *> &cards, KCardPile *oldPile, KCardPile *newPile)
{
    PatPile *newPatPile = dynamic_cast<PatPile *>(newPile);
    PatPile *oldPatPile = dynamic_cast<PatPile *>(oldPile);

    if (oldPatPile && oldPatPile->isFoundation() && newPatPile && !newPatPile->isFoundation()) {
        for (KCard *c : cards)
            m_cardsRemovedFromFoundations.insert(c);
    } else {
        for (KCard *c : cards)
            m_cardsRemovedFromFoundations.remove(c);
    }

    if (!m_dropInProgress && !m_dealInProgress) {
        m_dealStarted = true;
        takeState();
    }
}

void DealerScene::startHint()
{
    stopDemo();
    stopDrop();

    if (isCardAnimationRunning()) {
        m_hintQueued = true;
        return;
    }

    if (isKeyboardModeActive())
        setKeyboardModeActive(false);

    QList<QGraphicsItem *> toHighlight;
    const auto moveHints = getHints();
    for (const MoveHint &h : moveHints)
        toHighlight << h.card();

    if (!m_winningMoves.isEmpty()) {
        MOVE m = m_winningMoves.first();
        MoveHint mh = solver()->translateMove(m);
        if (mh.isValid())
            toHighlight << mh.card();
    }

    m_hintInProgress = !toHighlight.isEmpty();
    setHighlightedItems(toHighlight);
    Q_EMIT hintActive(m_hintInProgress);
}

void DealerScene::stopHint()
{
    if (m_hintInProgress) {
        m_hintInProgress = false;
        clearHighlightedItems();
        Q_EMIT hintActive(false);
    }
}

bool DealerScene::isHintActive() const
{
    return m_hintInProgress;
}

QList<MoveHint> DealerScene::getSolverHints()
{
    QList<MoveHint> hintList;

    if (m_solverThread && m_solverThread->isRunning())
        m_solverThread->abort();

    solver()->translate_layout();
    solver()->patsolve(1);

    const auto moves = solver()->firstMoves();
    for (const MOVE &m : moves) {
        MoveHint mh = solver()->translateMove(m);
        hintList << mh;
    }
    return hintList;
}

QList<MoveHint> DealerScene::getHints()
{
    if (solver())
        return getSolverHints();

    return bruteForceHints();
}

QList<MoveHint> DealerScene::bruteForceHints()
{
    QList<MoveHint> hintList;
    const auto patPiles = this->patPiles();
    for (PatPile *store : patPiles) {
        if (store->isFoundation() || store->isEmpty())
            continue;

        QList<KCard *> cards = store->cards();
        while (cards.count() && !cards.first()->isFaceUp())
            cards.erase(cards.begin());

        QList<KCard *>::Iterator iti = cards.begin();
        while (iti != cards.end()) {
            if (allowedToRemove(store, (*iti))) {
                for (PatPile *dest : patPiles) {
                    int cardIndex = store->indexOf(*iti);
                    if (cardIndex == 0 && dest->isEmpty() && !dest->isFoundation())
                        continue;

                    if (!checkAdd(dest, dest->cards(), cards))
                        continue;

                    if (dest->isFoundation()) {
                        hintList << MoveHint(*iti, dest, 127);
                    } else {
                        QList<KCard *> cardsBelow = cards.mid(0, cardIndex);

                        // if it could be here as well, then it's no use
                        if ((cardsBelow.isEmpty() && !dest->isEmpty()) || !checkAdd(store, cardsBelow, cards)) {
                            hintList << MoveHint(*iti, dest, 0);
                        } else if (checkPrefering(dest, dest->cards(), cards)
                                   && !checkPrefering(store, cardsBelow, cards)) { // if checkPrefers says so, we add it nonetheless
                            hintList << MoveHint(*iti, dest, 10);
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
    if (!m_winningMoves.isEmpty()) {
        MOVE m = m_winningMoves.takeFirst();
        MoveHint mh = solver()->translateMove(m);

#ifdef CHOOSE_HINT_DEBUG
        if (m.totype == O_Type)
            fprintf(stderr, "move from %d out (at %d) Prio: %d\n", m.from, m.turn_index, m.pri);
        else
            fprintf(stderr, "move from %d to %d (%d) Prio: %d\n", m.from, m.to, m.turn_index, m.pri);
#endif

        return mh;
    }

    QList<MoveHint> hintList = getHints();

    if (hintList.isEmpty()) {
        return MoveHint();
    } else {
        // Generate a random number with an exponentional distribution averaging 1/4.
        qreal randomExp = qMin<qreal>(-log(1 - QRandomGenerator::global()->generateDouble()) / 4, 1);
        int randomIndex = randomExp * (hintList.size() - 1);

        std::sort(hintList.begin(), hintList.end(), prioSort);
        return hintList.at(randomIndex);
    }
}

void DealerScene::startNew(int dealNumber)
{
    if (dealNumber != -1)
        m_dealNumber = qBound(1, dealNumber, INT_MAX);

    // Only record the statistics and reset gameStarted if  we're starting a
    // new game number or we're restarting a game we've already won.
    if (dealNumber != -1 || m_dealHasBeenWon) {
        recordGameStatistics();
        m_statisticsRecorded = false;
        m_dealStarted = false;
    }

    if (isCardAnimationRunning()) {
        QTimer::singleShot(100, this, SLOT(startNew()));
        return;
    }

    if (m_solverThread && m_solverThread->isRunning())
        m_solverThread->abort();

    resetInternals();

    Q_EMIT updateMoves(0);

    const auto piles = this->piles();
    for (KCardPile *p : piles)
        p->clear();

    m_dealInProgress = true;
    restart(KpatShuffle::shuffled<KCard *>(deck()->cards(), m_dealNumber));
    m_dealInProgress = false;

    takeState();
    update();
}

void DealerScene::resetInternals()
{
    stop();

    setKeyboardModeActive(false);

    m_dealHasBeenWon = false;
    m_wonItem->hide();

    qDeleteAll(m_undoStack);
    m_undoStack.clear();
    delete m_currentState;
    m_currentState = nullptr;
    qDeleteAll(m_redoStack);
    m_redoStack.clear();
    m_lastKnownCardStates.clear();

    m_dealWasJustSaved = false;
    m_dealWasEverWinnable = false;
    m_toldAboutLostGame = false;
    m_toldAboutWonGame = false;
    m_loadedMoveCount = 0;

    m_playerReceivedHelp = false;

    m_dealInProgress = false;

    m_dropInProgress = false;
    m_dropSpeedFactor = 1;
    m_cardsRemovedFromFoundations.clear();

    const auto cards = deck()->cards();
    for (KCard *c : cards) {
        c->disconnect(this);
        c->stopAnimation();
    }

    Q_EMIT solverStateChanged(QString());
    Q_EMIT gameInProgress(true);
}

QPointF posAlongRect(qreal distOnRect, const QRectF &rect)
{
    if (distOnRect < rect.width())
        return rect.topLeft() + QPointF(distOnRect, 0);
    distOnRect -= rect.width();
    if (distOnRect < rect.height())
        return rect.topRight() + QPointF(0, distOnRect);
    distOnRect -= rect.height();
    if (distOnRect < rect.width())
        return rect.bottomRight() + QPointF(-distOnRect, 0);
    distOnRect -= rect.width();
    return rect.bottomLeft() + QPointF(0, -distOnRect);
}

void DealerScene::won()
{
    if (m_dealHasBeenWon)
        return;

    m_dealHasBeenWon = true;
    m_toldAboutWonGame = true;

    stopDemo();
    recordGameStatistics();

    Q_EMIT solverStateChanged(QString());

    Q_EMIT newCardsPossible(false);
    Q_EMIT undoPossible(false);
    Q_EMIT redoPossible(false);
    Q_EMIT gameInProgress(false);

    setKeyboardModeActive(false);

    qreal speed = sqrt(width() * width() + height() * height()) / (DURATION_WON);

    QRectF justOffScreen = sceneRect().adjusted(-deck()->cardWidth(), -deck()->cardHeight(), 0, 0);
    qreal spacing = 2 * (justOffScreen.width() + justOffScreen.height()) / deck()->cards().size();
    qreal distOnRect = 0;

    const auto cards = deck()->cards();
    for (KCard *c : cards) {
        distOnRect += spacing;
        QPointF pos2 = posAlongRect(distOnRect, justOffScreen);
        QPointF delta = c->pos() - pos2;
        qreal dist = sqrt(delta.x() * delta.x() + delta.y() * delta.y());

        c->setFaceUp(true);
        c->animate(pos2, c->zValue(), 0, true, false, dist / speed);
    }

    connect(deck(), &KAbstractCardDeck::cardAnimationDone, this, &DealerScene::showWonMessage);
}

void DealerScene::showWonMessage()
{
    disconnect(deck(), &KAbstractCardDeck::cardAnimationDone, this, &DealerScene::showWonMessage);

    // It shouldn't be necessary to stop the demo yet again here, but we
    // get crashes if we don't. Will have to look into this further.
    stopDemo();

    // Hide all cards to prevent them from showing up accidentally if the
    // window is resized.
    const auto cards = deck()->cards();
    for (KCard *c : cards)
        c->hide();

    updateWonItem();
    m_wonItem->show();
}

void DealerScene::updateWonItem()
{
    const qreal aspectRatio = Renderer::self()->aspectRatioOfElement(QStringLiteral("message_frame"));
    int boxWidth;
    int boxHeight;
    if (width() < aspectRatio * height()) {
        boxWidth = width() * wonBoxToSceneSizeRatio;
        boxHeight = boxWidth / aspectRatio;
    } else {
        boxHeight = height() * wonBoxToSceneSizeRatio;
        boxWidth = boxHeight * aspectRatio;
    }
    m_wonItem->setSize(QSize(boxWidth, boxHeight));

    if (m_playerReceivedHelp)
        m_wonItem->setMessage(i18n("Congratulations! We have won."));
    else
        m_wonItem->setMessage(i18n("Congratulations! You have won."));

    m_wonItem->setPos(QPointF((width() - boxWidth) / 2, (height() - boxHeight) / 2) + sceneRect().topLeft());
}

bool DealerScene::allowedToAdd(const KCardPile *pile, const QList<KCard *> &cards) const
{
    if (!pile->isEmpty() && !pile->topCard()->isFaceUp())
        return false;

    const PatPile *p = dynamic_cast<const PatPile *>(pile);
    return p && checkAdd(p, p->cards(), cards);
}

bool DealerScene::allowedToRemove(const KCardPile *pile, const KCard *card) const
{
    const PatPile *p = dynamic_cast<const PatPile *>(pile);
    QList<KCard *> cards = pile->topCardsDownTo(card);
    return p && card->isFaceUp() && !cards.isEmpty() && checkRemove(p, cards);
}

bool DealerScene::checkAdd(const PatPile *pile, const QList<KCard *> &oldCards, const QList<KCard *> &newCards) const
{
    Q_UNUSED(pile)
    Q_UNUSED(oldCards)
    Q_UNUSED(newCards)
    return false;
}

bool DealerScene::checkRemove(const PatPile *pile, const QList<KCard *> &cards) const
{
    Q_UNUSED(pile)
    Q_UNUSED(cards)
    return false;
}

bool DealerScene::checkPrefering(const PatPile *pile, const QList<KCard *> &oldCards, const QList<KCard *> &newCards) const
{
    Q_UNUSED(pile)
    Q_UNUSED(oldCards)
    Q_UNUSED(newCards)
    return false;
}

void DealerScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    stop();

    const QList<QGraphicsItem *> itemsAtPoint = items(e->scenePos());
    KCard *card = qgraphicsitem_cast<KCard *>(itemsAtPoint.isEmpty() ? nullptr : itemsAtPoint.first());

    if (m_peekedCard) {
        e->accept();
    } else if (e->button() == Qt::RightButton && card && card->pile() && card != card->pile()->topCard() && cardsBeingDragged().isEmpty()
               && !isCardAnimationRunning()) {
        e->accept();
        m_peekedCard = card;
        QPointF pos2(card->x() + deck()->cardWidth() / 3.0, card->y() - deck()->cardHeight() / 3.0);
        card->setZValue(card->zValue() + 0.1);
        card->animate(pos2, card->zValue(), 20, card->isFaceUp(), false, DURATION_FANCYSHOW);
    } else {
        KCardScene::mousePressEvent(e);
        if (!cardsBeingDragged().isEmpty())
            Q_EMIT cardsPickedUp();
    }
}

void DealerScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    stop();

    if (e->button() == Qt::RightButton && m_peekedCard && m_peekedCard->pile()) {
        e->accept();
        updatePileLayout(m_peekedCard->pile(), DURATION_FANCYSHOW);
        m_peekedCard = nullptr;
    } else {
        bool hadCards = !cardsBeingDragged().isEmpty();
        KCardScene::mouseReleaseEvent(e);
        if (hadCards && cardsBeingDragged().isEmpty())
            Q_EMIT cardsPutDown();
    }
}

void DealerScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)
{
    if (!m_dealHasBeenWon) {
        stop();
        KCardScene::mouseDoubleClickEvent(e);
    } else {
        Q_EMIT newDeal();
    }
}

bool DealerScene::tryAutomaticMove(KCard *card)
{
    if (!isCardAnimationRunning() && card && card->pile() && card == card->pile()->topCard() && card->isFaceUp() && allowedToRemove(card->pile(), card)) {
        QList<KCard *> cardList = QList<KCard *>() << card;

        const auto patPiles = this->patPiles();
        for (PatPile *p : patPiles) {
            if (p->isFoundation() && allowedToAdd(p, cardList)) {
                moveCardToPile(card, p, DURATION_MOVE);
                return true;
            }
        }
    }

    return false;
}

void DealerScene::undo()
{
    undoOrRedo(true);
}

void DealerScene::redo()
{
    undoOrRedo(false);
}

void DealerScene::undoOrRedo(bool undo)
{
    stop();

    if (isCardAnimationRunning())
        return;

    // The undo and redo actions are almost identical, except for where states
    // are pulled from and pushed to, so to keep things generic, we use
    // direction dependent const references throughout this code.
    QStack<GameState *> &fromStack = undo ? m_undoStack : m_redoStack;
    QStack<GameState *> &toStack = undo ? m_redoStack : m_undoStack;

    if (!fromStack.isEmpty() && m_currentState) {
        // If we're undoing, we use the oldStates of the changes of the current
        // state. If we're redoing, we use the newStates of the changes of the
        // nextState.
        const QList<CardStateChange> &changes = undo ? m_currentState->changes : fromStack.top()->changes;

        // Update the currentState pointer and undo/redo stacks.
        toStack.push(m_currentState);
        m_currentState = fromStack.pop();
        setGameState(m_currentState->stateData);

        QSet<KCardPile *> pilesAffected;

        for (const CardStateChange &change : changes) {
            CardState sourceState = undo ? change.newState : change.oldState;
            CardState destState = undo ? change.oldState : change.newState;

            PatPile *sourcePile = dynamic_cast<PatPile *>(sourceState.pile);
            PatPile *destPile = dynamic_cast<PatPile *>(destState.pile);
            bool notDroppable = destState.takenDown || ((sourcePile && sourcePile->isFoundation()) && !(destPile && destPile->isFoundation()));

            pilesAffected << sourceState.pile << destState.pile;

            for (KCard *c : change.cards) {
                m_lastKnownCardStates.insert(c, destState);

                c->setFaceUp(destState.faceUp);
                destState.pile->insert(destState.index, c);

                if (notDroppable)
                    m_cardsRemovedFromFoundations.insert(c);
                else
                    m_cardsRemovedFromFoundations.remove(c);

                ++sourceState.index;
                ++destState.index;
            }
        };

        // At this point all cards should be in the right piles, but not
        // necessarily at the right positions within those piles. So we
        // run through the piles involved and swap card positions until
        // everything is back in its place, then relayout the piles.
        for (KCardPile *p : std::as_const(pilesAffected)) {
            int i = 0;
            while (i < p->count()) {
                int index = m_lastKnownCardStates.value(p->at(i)).index;
                if (i == index)
                    ++i;
                else
                    p->swapCards(i, index);
            }

            updatePileLayout(p, 0);
        }

        Q_EMIT updateMoves(moveCount());
        Q_EMIT undoPossible(!m_undoStack.isEmpty());
        Q_EMIT redoPossible(!m_redoStack.isEmpty());

        if (m_toldAboutLostGame) // everything's possible again
        {
            gameInProgress(true);
            m_toldAboutLostGame = false;
            m_toldAboutWonGame = false;
        }

        int solvability = m_currentState->solvability;
        m_winningMoves = m_currentState->winningMoves;

        Q_EMIT solverStateChanged(solverStatusMessage(solvability, m_dealWasEverWinnable));

        if (m_solver && (solvability == SolverInterface::SearchAborted || solvability == SolverInterface::MemoryLimitReached)) {
            startSolver();
        }
    }
}

void DealerScene::takeState()
{
    if (isCardAnimationRunning()) {
        m_takeStateQueued = true;
        return;
    }

    if (!isDemoActive())
        m_winningMoves.clear();

    QList<CardStateChange> changes;

    const auto piles = this->piles();
    for (KCardPile *p : piles) {
        QList<KCard *> currentRun;
        CardState oldRunState;
        CardState newRunState;

        for (int i = 0; i < p->count(); ++i) {
            KCard *c = p->at(i);

            const CardState &oldState = m_lastKnownCardStates.value(c);
            CardState newState(p, i, c->isFaceUp(), m_cardsRemovedFromFoundations.contains(c));

            // The card has changed.
            if (newState != oldState) {
                // There's a run in progress, but this card isn't part of it.
                if (!currentRun.isEmpty()
                    && (oldState.pile != oldRunState.pile || (oldState.index != -1 && oldState.index != oldRunState.index + currentRun.size())
                        || oldState.faceUp != oldRunState.faceUp || newState.faceUp != newRunState.faceUp || oldState.takenDown != oldRunState.takenDown
                        || newState.takenDown != newRunState.takenDown)) {
                    changes << CardStateChange(oldRunState, newRunState, currentRun);
                    currentRun.clear();
                }

                // This card is the start of a new run.
                if (currentRun.isEmpty()) {
                    oldRunState = oldState;
                    newRunState = newState;
                }

                currentRun << c;

                m_lastKnownCardStates.insert(c, newState);
            }
        }
        // Add the last run, if any.
        if (!currentRun.isEmpty()) {
            changes << CardStateChange(oldRunState, newRunState, currentRun);
        }
    }

    // If nothing has changed, we're done.
    if (changes.isEmpty() && m_currentState && m_currentState->stateData == getGameState()) {
        return;
    }

    if (m_currentState) {
        m_undoStack.push(m_currentState);
        qDeleteAll(m_redoStack);
        m_redoStack.clear();
    }
    m_currentState = new GameState(changes, getGameState());

    Q_EMIT redoPossible(false);
    Q_EMIT undoPossible(!m_undoStack.isEmpty());
    Q_EMIT updateMoves(moveCount());

    m_dealWasJustSaved = false;
    if (isGameWon()) {
        won();
        return;
    }

    if (!m_toldAboutWonGame && !m_toldAboutLostGame && isGameLost()) {
        Q_EMIT gameInProgress(false);
        Q_EMIT solverStateChanged(i18n("Solver: This game is lost."));
        m_toldAboutLostGame = true;
        stopDemo();
        return;
    }

    if (!isDemoActive() && !isCardAnimationRunning() && m_solver)
        startSolver();

    if (autoDropEnabled() && !isDropActive() && !isDemoActive() && m_redoStack.isEmpty()) {
        if (m_interruptAutoDrop)
            m_interruptAutoDrop = false;
        else
            m_dropQueued = true;
    }
}

void DealerScene::setSolverEnabled(bool a)
{
    m_solverEnabled = a;
}

void DealerScene::setAutoDropEnabled(bool enabled)
{
    m_autoDropEnabled = enabled;
}

bool DealerScene::autoDropEnabled() const
{
    return m_autoDropEnabled;
}

void DealerScene::startDrop()
{
    stopHint();
    stopDemo();

    if (isCardAnimationRunning()) {
        m_dropQueued = true;
        return;
    }

    m_dropInProgress = true;
    m_interruptAutoDrop = false;
    m_dropSpeedFactor = 1;
    Q_EMIT dropActive(true);

    drop();
}

void DealerScene::stopDrop()
{
    if (m_dropInProgress) {
        m_dropTimer.stop();
        m_dropInProgress = false;
        Q_EMIT dropActive(false);

        if (autoDropEnabled() && m_takeStateQueued)
            m_interruptAutoDrop = true;
    }
}

bool DealerScene::isDropActive() const
{
    return m_dropInProgress;
}

bool DealerScene::drop()
{
    const auto moveHints = getHints();
    for (const MoveHint &mh : moveHints) {
        if (mh.pile() && mh.pile()->isFoundation() && mh.priority() > 120 && !m_cardsRemovedFromFoundations.contains(mh.card())) {
            QList<KCard *> cards = mh.card()->pile()->topCardsDownTo(mh.card());

            QMap<KCard *, QPointF> oldPositions;
            for (KCard *c : std::as_const(cards))
                oldPositions.insert(c, c->pos());

            moveCardsToPile(cards, mh.pile(), DURATION_MOVE);

            int count = 0;
            for (KCard *c : std::as_const(cards)) {
                c->completeAnimation();
                QPointF destPos = c->pos();
                c->setPos(oldPositions.value(c));

                int duration = speedUpTime(DURATION_AUTODROP + count * DURATION_AUTODROP / 10);
                c->animate(destPos, c->zValue(), 0, c->isFaceUp(), true, duration);

                ++count;
            }

            m_dropSpeedFactor *= AUTODROP_SPEEDUP_FACTOR;

            takeState();

            return true;
        }
    }

    m_dropInProgress = false;
    Q_EMIT dropActive(false);

    return false;
}

int DealerScene::speedUpTime(int delay) const
{
    if (delay < DURATION_AUTODROP_MINIMUM)
        return delay;
    else
        return qMax<int>(delay * m_dropSpeedFactor, DURATION_AUTODROP_MINIMUM);
}

void DealerScene::stopAndRestartSolver()
{
    if (m_toldAboutLostGame || m_toldAboutWonGame) // who cares?
        return;

    if (m_solverThread && m_solverThread->isRunning()) {
        m_solverThread->abort();
    }

    if (isCardAnimationRunning()) {
        startSolver();
        return;
    }

    slotSolverEnded();
}

void DealerScene::slotSolverEnded()
{
    if (m_solverThread && m_solverThread->isRunning())
        return;

    m_solver->translate_layout();
    m_winningMoves.clear();
    Q_EMIT solverStateChanged(i18n("Solver: Calculating…"));
    if (!m_solverThread) {
        m_solverThread = new SolverThread(m_solver);
        connect(m_solverThread, &SolverThread::finished, this, &DealerScene::slotSolverFinished);
    }
    m_solverThread->start(m_solverEnabled ? QThread::IdlePriority : QThread::NormalPriority);
}

void DealerScene::slotSolverFinished(int result)
{
    if (result == SolverInterface::SolutionExists) {
        m_winningMoves = m_solver->winMoves();
        m_dealWasEverWinnable = true;
    }

    Q_EMIT solverStateChanged(solverStatusMessage(result, m_dealWasEverWinnable));

    if (m_currentState) {
        m_currentState->solvability = static_cast<SolverInterface::ExitStatus>(result);
        m_currentState->winningMoves = m_winningMoves;
    }

    if (result == SolverInterface::SearchAborted)
        startSolver();
}

int DealerScene::gameNumber() const
{
    return m_dealNumber;
}

void DealerScene::stop()
{
    stopHint();
    stopDemo();
    stopDrop();
}

void DealerScene::animationDone()
{
    Q_ASSERT(!isCardAnimationRunning());

    if (!m_multiStepMoves.isEmpty()) {
        continueMultiStepMove();
        return;
    }

    if (m_takeStateQueued) {
        m_takeStateQueued = false;
        takeState();
    }

    if (m_demoInProgress) {
        m_demoTimer.start(TIME_BETWEEN_MOVES);
    } else if (m_dropInProgress) {
        m_dropTimer.start(speedUpTime(TIME_BETWEEN_MOVES));
    } else if (m_newCardsQueued) {
        m_newCardsQueued = false;
        newCards();
    } else if (m_hintQueued) {
        m_hintQueued = false;
        startHint();
    } else if (m_demoQueued) {
        m_demoQueued = false;
        startDemo();
    } else if (m_dropQueued) {
        m_dropQueued = false;
        startDrop();
    }
}

void DealerScene::startDemo()
{
    stopHint();
    stopDrop();

    if (isCardAnimationRunning()) {
        m_demoQueued = true;
        return;
    }

    m_demoInProgress = true;
    m_playerReceivedHelp = true;
    m_dealStarted = true;

    demo();
}

void DealerScene::stopDemo()
{
    if (m_demoInProgress) {
        m_demoTimer.stop();
        m_demoInProgress = false;
        Q_EMIT demoActive(false);
    }
}

bool DealerScene::isDemoActive() const
{
    return m_demoInProgress;
}

void DealerScene::demo()
{
    if (isCardAnimationRunning()) {
        m_demoQueued = true;
        return;
    }

    m_demoInProgress = true;
    m_playerReceivedHelp = true;
    m_dealStarted = true;
    clearHighlightedItems();

    m_demoTimer.stop();

    MoveHint mh = chooseHint();
    if (mh.isValid()) {
        KCard *card = mh.card();
        Q_ASSERT(card);
        KCardPile *sourcePile = mh.card()->pile();
        Q_ASSERT(sourcePile);
        Q_ASSERT(allowedToRemove(sourcePile, card));
        PatPile *destPile = mh.pile();
        Q_ASSERT(destPile);
        Q_ASSERT(sourcePile != destPile);
        QList<KCard *> cards = sourcePile->topCardsDownTo(card);
        Q_ASSERT(allowedToAdd(destPile, cards));

        if (destPile->isEmpty()) {
            qCDebug(KPAT_LOG) << "Moving" << card->objectName() << "from the" << sourcePile->objectName() << "pile to the" << destPile->objectName()
                              << "pile, which is empty";
        } else {
            qCDebug(KPAT_LOG) << "Moving" << card->objectName() << "from the" << sourcePile->objectName() << "pile to the" << destPile->objectName()
                              << "pile, putting it on top of" << destPile->topCard()->objectName();
        }

        moveCardsToPile(cards, destPile, DURATION_DEMO);
    } else if (!newCards()) {
        if (isGameWon()) {
            won();
        } else {
            stopDemo();
            slotSolverEnded();
        }
        return;
    }

    Q_EMIT demoActive(true);
    takeState();
}

void DealerScene::drawDealRowOrRedeal()
{
    stop();

    if (isCardAnimationRunning()) {
        m_newCardsQueued = true;
        return;
    }

    m_newCardsQueued = false;
    newCards();
}

bool DealerScene::newCards()
{
    return false;
}

void DealerScene::setSolver(SolverInterface *s)
{
    delete m_solver;
    delete m_solverThread;
    m_solver = s;
    m_solverThread = nullptr;
}

bool DealerScene::isGameWon() const
{
    const auto patPiles = this->patPiles();
    for (PatPile *p : patPiles) {
        if (!p->isFoundation() && !p->isEmpty())
            return false;
    }
    return true;
}

void DealerScene::startSolver()
{
    if (m_solverEnabled)
        m_solverUpdateTimer.start();
}

bool DealerScene::isGameLost() const
{
    if (!m_winningMoves.isEmpty()) {
        return false;
    }
    if (solver()) {
        if (m_solverThread && m_solverThread->isRunning())
            m_solverThread->abort();

        solver()->translate_layout();
        return solver()->patsolve(neededFutureMoves()) == SolverInterface::NoSolutionExists;
    }
    return false;
}

void DealerScene::recordGameStatistics()
{
    // Don't record the game if it was never started, if it is unchanged since
    // it was last saved (allowing the user to close KPat after saving without
    // it recording a loss) or if it has already been recorded.//         takeState(); // copying it again
    if (m_dealStarted && !m_dealWasJustSaved && !m_statisticsRecorded) {
        int id = oldId();

        QString totalPlayedKey = QStringLiteral("total%1").arg(id);
        QString wonKey = QStringLiteral("won%1").arg(id);
        QString winStreakKey = QStringLiteral("winstreak%1").arg(id);
        QString maxWinStreakKey = QStringLiteral("maxwinstreak%1").arg(id);
        QString loseStreakKey = QStringLiteral("loosestreak%1").arg(id);
        QString maxLoseStreakKey = QStringLiteral("maxloosestreak%1").arg(id);
        QString minMovesKey = QStringLiteral("minmoves%1").arg(id);

        KConfigGroup config(KSharedConfig::openConfig(), scores_group);

        int totalPlayed = config.readEntry(totalPlayedKey, 0);
        int won = config.readEntry(wonKey, 0);
        int winStreak = config.readEntry(winStreakKey, 0);
        int maxWinStreak = config.readEntry(maxWinStreakKey, 0);
        int loseStreak = config.readEntry(loseStreakKey, 0);
        int maxLoseStreak = config.readEntry(maxLoseStreakKey, 0);
        int minMoves = config.readEntry(minMovesKey, -1);

        ++totalPlayed;

        if (m_dealHasBeenWon) {
            ++won;
            ++winStreak;
            maxWinStreak = qMax(winStreak, maxWinStreak);
            loseStreak = 0;
            if (minMoves < 0)
                minMoves = moveCount();
            else
                minMoves = qMin(minMoves, moveCount());
        } else {
            ++loseStreak;
            maxLoseStreak = qMax(loseStreak, maxLoseStreak);
            winStreak = 0;
        }

        config.writeEntry(totalPlayedKey, totalPlayed);
        config.writeEntry(wonKey, won);
        config.writeEntry(winStreakKey, winStreak);
        config.writeEntry(maxWinStreakKey, maxWinStreak);
        config.writeEntry(loseStreakKey, loseStreak);
        config.writeEntry(maxLoseStreakKey, maxLoseStreak);
        config.writeEntry(minMovesKey, minMoves);

        m_statisticsRecorded = true;
    }
}

void DealerScene::relayoutScene()
{
    KCardScene::relayoutScene();

    if (m_wonItem->isVisible())
        updateWonItem();
}

int DealerScene::gameId() const
{
    return m_di->baseId();
}

void DealerScene::setActions(int actions)
{
    m_supportedActions = actions;
}

int DealerScene::actions() const
{
    return m_supportedActions;
}

QList<QAction *> DealerScene::configActions() const
{
    return QList<QAction *>();
}

SolverInterface *DealerScene::solver() const
{
    return m_solver;
}

int DealerScene::neededFutureMoves() const
{
    return m_neededFutureMoves;
}

void DealerScene::setNeededFutureMoves(int i)
{
    m_neededFutureMoves = i;
}

void DealerScene::setDeckContents(int copies, const QList<KCardDeck::Suit> &suits)
{
    Q_ASSERT(copies >= 1);
    Q_ASSERT(!suits.isEmpty());

    // Note that the order in which the cards are created can not be changed
    // without breaking the game numbering. For historical reasons, KPat
    // generates card by rank and then by suit, rather than the more common
    // suit then rank ordering.
    QList<quint32> ids;
    unsigned int number = 0;
    for (int i = 0; i < copies; ++i) {
        const auto ranks = KCardDeck::standardRanks();
        for (const KCardDeck::Rank &r : ranks)
            for (const KCardDeck::Suit &s : suits)
                ids << KCardDeck::getId(s, r, number++);
    }
    deck()->setDeckContents(ids);
}

QImage DealerScene::createDump() const
{
    const QSize previewSize(480, 320);

    const auto cards = deck()->cards();
    for (KCard *c : cards)
        c->completeAnimation();

    QMultiMap<qreal, QGraphicsItem *> itemsByZ;
    const auto items = this->items();
    for (QGraphicsItem *item : items) {
        Q_ASSERT(item->zValue() >= 0);
        itemsByZ.insert(item->zValue(), item);
    }

    QImage img(contentArea().size().toSize(), QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);

    for (QGraphicsItem *item : std::as_const(itemsByZ)) {
        if (item->isVisible()) {
            p.save();
            p.setTransform(item->deviceTransform(p.worldTransform()), false);
            item->paint(&p, nullptr);
            p.restore();
        }
    }

    p.end();

    img = img.scaled(previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QImage img2(previewSize, QImage::Format_ARGB32);
    img2.fill(Qt::transparent);
    QPainter p2(&img2);
    p2.drawImage((img2.width() - img.width()) / 2, (img2.height() - img.height()) / 2, img);
    p2.end();

    return img2;
}

void DealerScene::mapOldId(int id)
{
    Q_UNUSED(id);
}

int DealerScene::oldId() const
{
    return gameId();
}

QString DealerScene::getGameState() const
{
    return QString();
}

void DealerScene::setGameState(const QString &state)
{
    Q_UNUSED(state);
}

QString DealerScene::getGameOptions() const
{
    return QString();
}

void DealerScene::setGameOptions(const QString &options)
{
    Q_UNUSED(options);
}

bool DealerScene::allowedToStartNewGame()
{
    // Check if the user is already running a game, and if she is,
    // then ask if she wants to abort it.
    return !m_dealStarted || m_dealWasJustSaved || m_toldAboutWonGame || m_toldAboutLostGame
        || KMessageBox::warningContinueCancel(QApplication::activeWindow(),
                                              i18n("A new game has been requested, but there is already a game in progress.\n\n"
                                                   "A loss will be recorded in the statistics if the current game is abandoned."),
                                              i18nc("@title:window", "Abandon Current Game?"),
                                              KGuiItem(i18nc("@action:button", "Abandon Current Game")),
                                              KStandardGuiItem::cancel(),
                                              QStringLiteral("careaboutstats"))
        == KMessageBox::Continue;
}

void DealerScene::addCardForDeal(KCardPile *pile, KCard *card, bool faceUp, QPointF startPos)
{
    Q_ASSERT(card);
    Q_ASSERT(pile);

    card->setFaceUp(faceUp);
    pile->add(card);
    m_initDealPositions.insert(card, startPos);
}

void DealerScene::startDealAnimation()
{
    qreal speed = sqrt(width() * width() + height() * height()) / (DURATION_DEAL);
    const auto patPiles = this->patPiles();
    for (PatPile *p : patPiles) {
        updatePileLayout(p, 0);
        const auto cards = p->cards();
        for (KCard *c : cards) {
            if (!m_initDealPositions.contains(c))
                continue;

            QPointF pos2 = c->pos();
            c->setPos(m_initDealPositions.value(c));

            QPointF delta = c->pos() - pos2;
            qreal dist = sqrt(delta.x() * delta.x() + delta.y() * delta.y());
            int duration = qRound(dist / speed);
            c->animate(pos2, c->zValue(), 0, c->isFaceUp(), false, duration);
        }
    }
    m_initDealPositions.clear();
}

void DealerScene::multiStepMove(const QList<KCard *> &cards,
                                KCardPile *pile,
                                const QList<KCardPile *> &freePiles,
                                const QList<KCardPile *> &freeCells,
                                int duration)
{
    Q_ASSERT(cards.size() == 1 || !freePiles.isEmpty() || !freeCells.isEmpty());

    m_multiStepMoves.clear();
    m_multiStepDuration = duration;

    multiStepSubMove(cards, pile, freePiles, freeCells);
    continueMultiStepMove();
}

void DealerScene::multiStepSubMove(QList<KCard *> cards, KCardPile *pile, QList<KCardPile *> freePiles, const QList<KCardPile *> &freeCells)
{
    // Note that cards and freePiles are passed by value, as we need to make a
    // local copy anyway.

    // Using n free cells, we can move a run of n+1 cards. If we want to move
    // more than that, we have to recursively move some of our cards to one of
    // the free piles temporarily.
    const int freeCellsPlusOne = freeCells.size() + 1;
    int cardsToSubMove = cards.size() - freeCellsPlusOne;

    QList<QPair<KCardPile *, QList<KCard *>>> tempMoves;
    while (cardsToSubMove > 0) {
        int tempMoveSize;
        if (cardsToSubMove <= freePiles.size() * freeCellsPlusOne) {
            // If the cards that have to be submoved can be spread across the
            // the free piles without putting more than freeCellsPlusOne cards
            // on each one, we do so. This means that none of our submoves will
            // need further submoves, which keeps the total move count down. We
            // Just to a simple rounding up integer division.
            tempMoveSize = (cardsToSubMove + freePiles.size() - 1) / freePiles.size();
        } else {
            // Otherwise, we use the space optimal method that gets the cards
            // moved using a minimal number of piles, but uses more submoves.
            tempMoveSize = freeCellsPlusOne;
            while (tempMoveSize * 2 < cardsToSubMove)
                tempMoveSize *= 2;
        }

        QList<KCard *> subCards;
        for (int i = 0; i < tempMoveSize; ++i)
            subCards.prepend(cards.takeLast());

        Q_ASSERT(!freePiles.isEmpty());
        KCardPile *nextPile = freePiles.takeFirst();

        tempMoves << qMakePair(nextPile, subCards);
        multiStepSubMove(subCards, nextPile, freePiles, freeCells);

        cardsToSubMove -= tempMoveSize;
    }

    // Move cards to free cells.
    for (int i = 0; i < cards.size() - 1; ++i) {
        KCard *c = cards.at(cards.size() - 1 - i);
        m_multiStepMoves << qMakePair(c, freeCells[i]);
    }

    // Move bottom card to destination pile.
    m_multiStepMoves << qMakePair(cards.first(), pile);

    // Move cards from free cells to destination pile.
    for (int i = 1; i < cards.size(); ++i)
        m_multiStepMoves << qMakePair(cards.at(i), pile);

    // If we just moved the bottomost card of the source pile, it must now be
    // empty and we won't need it any more. So we return it to the list of free
    // piles.
    KCardPile *sourcePile = cards.first()->pile();
    if (sourcePile->at(0) == cards.first())
        freePiles << sourcePile;

    // If we had to do any submoves, we now move those cards from their
    // temporary pile to the destination pile and free up their temporary pile.
    while (!tempMoves.isEmpty()) {
        QPair<KCardPile *, QList<KCard *>> m = tempMoves.takeLast();
        multiStepSubMove(m.second, pile, freePiles, freeCells);
        freePiles << m.first;
    }
}

void DealerScene::continueMultiStepMove()
{
    Q_ASSERT(!m_multiStepMoves.isEmpty());
    Q_ASSERT(!isCardAnimationRunning());

    QPair<KCard *, KCardPile *> m = m_multiStepMoves.takeFirst();
    KCard *card = m.first;
    KCardPile *dest = m.second;
    KCardPile *source = card->pile();

    Q_ASSERT(card == source->topCard());
    Q_ASSERT(allowedToAdd(dest, QList<KCard *>() << card));

    m_multiStepDuration = qMax<int>(m_multiStepDuration * 0.9, 50);

    dest->add(card);
    card->raise();
    updatePileLayout(dest, m_multiStepDuration);
    updatePileLayout(source, m_multiStepDuration);

    if (m_multiStepMoves.isEmpty())
        takeState();
}

#include "dealer.moc"
#include "moc_dealer.cpp"
