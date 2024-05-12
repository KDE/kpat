/*
 * Copyright (C) 1997 Rodolfo Borges <barrett@labma.ufrj.br>
 * Copyright (C) 1998-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#include "freecell.h"

// own
#include "dealerinfo.h"
#include "kpat_debug.h"
#include "patsolve/freecellsolver.h"
#include "pileutils.h"
#include "settings.h"
#include "speeds.h"
// KF
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KSelectAction>

Freecell::Freecell(const DealerInfo *di)
    : DealerScene(di)
{
    configOptions();
    getSavedOptions();
}

void Freecell::initialize()
{
    setDeckContents(m_decks + 1);

    const bool isRightFoundation = m_reserves + 4 * (m_decks + 1) > (m_stacks + 6);
    const qreal topRowDist = isRightFoundation ? 1.13 : 1.08;
    const qreal bottomRowDist = 1.13;
    const qreal targetOffsetDist = ((m_stacks + 5) * bottomRowDist + 1) - (3 * topRowDist + 1) * (m_decks + 1);

    for (int i = 0; i < m_reserves; ++i) {
        freecell[i] = new PatPile(this, 1 + i, QStringLiteral("freecell%1").arg(i));
        freecell[i]->setPileRole(PatPile::Cell);
        freecell[i]->setLayoutPos(topRowDist * i, 0);
        freecell[i]->setKeyboardSelectHint(KCardPile::AutoFocusTop);
        freecell[i]->setKeyboardDropHint(KCardPile::AutoFocusTop);
    }

    for (int i = 0; i < (m_stacks + 6); ++i) {
        store[i] = new PatPile(this, 1 + i, QStringLiteral("store%1").arg(i));
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setLayoutPos(bottomRowDist * i, 1.3);
        store[i]->setBottomPadding(2.5);
        store[i]->setHeightPolicy(KCardPile::GrowDown);
        store[i]->setKeyboardSelectHint(KCardPile::AutoFocusDeepestRemovable);
        store[i]->setKeyboardDropHint(KCardPile::AutoFocusTop);
    }

    if (isRightFoundation) {
        const int columns = std::max(m_reserves, m_stacks + 6);
        for (int i = 0; i < 4 * (m_decks + 1); ++i) {
            const qreal offsetX = 0.25 + columns * bottomRowDist + i / 4 * bottomRowDist;
            const qreal offsetY = bottomRowDist * i - i / 4 * bottomRowDist * 4;
            target[i] = new PatPile(this, 1 + i, QStringLiteral("target%1").arg(i));
            target[i]->setPileRole(PatPile::Foundation);
            target[i]->setLayoutPos(offsetX, offsetY);
            target[i]->setKeyboardSelectHint(KCardPile::NeverFocus);
            target[i]->setKeyboardDropHint(KCardPile::ForceFocusTop);
        }
    } else {
        for (int i = 0; i < 4 * (m_decks + 1); ++i) {
            target[i] = new PatPile(this, 1 + i, QStringLiteral("target%1").arg(i));
            target[i]->setPileRole(PatPile::Foundation);
            target[i]->setLayoutPos(targetOffsetDist + topRowDist * i, 0);
            target[i]->setSpread(0, 0);
            target[i]->setKeyboardSelectHint(KCardPile::NeverFocus);
            target[i]->setKeyboardDropHint(KCardPile::ForceFocusTop);
        }
    }

    setActions(DealerScene::Demo | DealerScene::Hint);
    auto solver = new FreecellSolver(this);
    solver->default_max_positions = Settings::freecellSolverIterationsLimit();
    setSolver(solver);
    setNeededFutureMoves(4); // reserve some
}

QList<QAction *> Freecell::configActions() const
{
    return QList<QAction *>() << options << m_emptyStackFillOption << m_sequenceBuiltByOption << m_reservesOption << m_stacksOption;
}

void Freecell::gameTypeChanged()
{
    stopDemo();

    if (allowedToStartNewGame()) {
        if (m_variation != options->currentItem()) {
            setOptions(options->currentItem());
        } else {
            // update option selections
            if (m_emptyStackFill != m_emptyStackFillOption->currentItem())
                m_emptyStackFill = m_emptyStackFillOption->currentItem();
            else if (m_sequenceBuiltBy != m_sequenceBuiltByOption->currentItem())
                m_sequenceBuiltBy = m_sequenceBuiltByOption->currentItem();
            else if (m_reserves != m_reservesOption->currentItem())
                m_reserves = m_reservesOption->currentItem();
            else if (m_stacks != m_stacksOption->currentItem())
                m_stacks = m_stacksOption->currentItem();
            else if (m_decks != m_decksOption->currentItem())
                m_decks = m_decksOption->currentItem();

            matchVariant();
        }

        // remove existing piles
        clearPatPiles();

        initialize();
        relayoutScene();
        startNew(gameNumber());
        setSavedOptions();
    } else {
        // If we're not allowed, reset the options
        getSavedOptions();
    }
}

void Freecell::restart(const QList<KCard *> &cards)
{
    QList<KCard *> cardList = cards;

    // Prefill reserves for select game types
    if (m_variation == 4)
        for (int i = 0; i < 2; ++i)
            addCardForDeal(freecell[i], cardList.takeLast(), true, freecell[0]->pos());
    else if (m_variation == 1 || m_variation == 2)
        for (int i = 0; i < 4; ++i)
            addCardForDeal(freecell[i], cardList.takeLast(), true, freecell[0]->pos());

    int column = 0;
    while (!cardList.isEmpty()) {
        addCardForDeal(store[column], cardList.takeLast(), true, store[0]->pos());
        column = (column + 1) % (m_stacks + 6);
    }

    startDealAnimation();
}

QString Freecell::solverFormat() const
{
    QString output;
    QString tmp;
    for (int i = 0; i < 4 * (m_decks + 1); i++) {
        if (target[i]->isEmpty())
            continue;
        tmp += suitToString(target[i]->topCard()->suit()) + QLatin1Char('-') + rankToString(target[i]->topCard()->rank()) + QLatin1Char(' ');
    }
    if (!tmp.isEmpty())
        output += QStringLiteral("Foundations: %1\n").arg(tmp);

    tmp.truncate(0);
    for (int i = 0; i < m_reserves; i++) {
        const auto fc = freecell[i];
        tmp += (fc->isEmpty() ? QStringLiteral("-") : cardToRankSuitString(fc->topCard())) + QLatin1Char(' ');
    }
    if (!tmp.isEmpty()) {
        QString a = QStringLiteral("Freecells: %1\n");
        output += a.arg(tmp);
    }

    for (int i = 0; i < (m_stacks + 6); i++)
        cardsListToLine(output, store[i]->cards());
    return output;
}

void Freecell::cardsDroppedOnPile(const QList<KCard *> &cards, KCardPile *pile)
{
    if (cards.size() <= 1) {
        DealerScene::moveCardsToPile(cards, pile, DURATION_MOVE);
        return;
    }

    QList<KCardPile *> freeCells;
    for (int i = 0; i < m_reserves; ++i)
        if (freecell[i]->isEmpty())
            freeCells << freecell[i];

    QList<KCardPile *> freeStores;
    for (int i = 0; i < (m_stacks + 6); ++i)
        if (store[i]->isEmpty() && store[i] != pile)
            freeStores << store[i];

    multiStepMove(cards, pile, freeStores, freeCells, DURATION_MOVE);
}

bool Freecell::tryAutomaticMove(KCard *c)
{
    // target move
    if (DealerScene::tryAutomaticMove(c))
        return true;

    if (c->isAnimated())
        return false;

    if (allowedToRemove(c->pile(), c) && c == c->pile()->topCard()) {
        for (int i = 0; i < m_reserves; i++) {
            if (allowedToAdd(freecell[i], {c})) {
                moveCardToPile(c, freecell[i], DURATION_MOVE);
                return true;
            }
        }
    }
    return false;
}

bool Freecell::canPutStore(const KCardPile *pile, const QList<KCard *> &cards) const
{
    int freeCells = 0;
    for (int i = 0; i < m_reserves; ++i)
        if (freecell[i]->isEmpty())
            ++freeCells;

    int freeStores = 0;
    if (m_emptyStackFill == 0) {
        for (int i = 0; i < (m_stacks + 6); ++i)
            if (store[i]->isEmpty() && store[i] != pile)
                ++freeStores;
    }

    if (cards.size() <= (freeCells + 1) << freeStores) {
        if (pile->isEmpty())
            return m_emptyStackFill == 0 || (m_emptyStackFill == 1 && cards.first()->rank() == KCardDeck::King);
        else if (m_sequenceBuiltBy == 1)
            return cards.first()->rank() == pile->topCard()->rank() - 1 && cards.first()->suit() == pile->topCard()->suit();
        else if (m_sequenceBuiltBy == 0)
            return cards.first()->rank() == pile->topCard()->rank() - 1 && pile->topCard()->color() != cards.first()->color();
        else
            return cards.first()->rank() == pile->topCard()->rank() - 1;
    } else {
        return false;
    }
}

bool Freecell::checkAdd(const PatPile *pile, const QList<KCard *> &oldCards, const QList<KCard *> &newCards) const
{
    switch (pile->pileRole()) {
    case PatPile::Tableau:
        return canPutStore(pile, newCards);
    case PatPile::Cell:
        return oldCards.isEmpty() && newCards.size() == 1;
    case PatPile::Foundation:
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    default:
        return false;
    }
}

bool Freecell::checkRemove(const PatPile *pile, const QList<KCard *> &cards) const
{
    switch (pile->pileRole()) {
    case PatPile::Tableau:
        if (m_sequenceBuiltBy == 1)
            return isSameSuitDescending(cards);
        else if (m_sequenceBuiltBy == 0)
            return isAlternateColorDescending(cards);
        else
            return isRankDescending(cards);
    case PatPile::Cell:
        return cards.first() == pile->topCard();
    case PatPile::Foundation:
    default:
        return false;
    }
}

static class FreecellDealerInfo : public DealerInfo
{
public:
    FreecellDealerInfo()
        : DealerInfo(kli18n("Freecell"),
                     FreecellGeneralId,
                     {
                         {FreecellBakersId, kli18n("Baker's Game")},
                         {FreecellEightOffId, kli18n("Eight Off")},
                         {FreecellForeId, kli18n("Forecell")},
                         {FreecellId, kli18n("Freecell")},
                         {FreecellSeahavenId, kli18n("Seahaven Towers")},
                         {FreecellCustomId, kli18n("Freecell (Custom)")},
                     })
    {
    }

    DealerScene *createGame() const override
    {
        return new Freecell(this);
    }
} freecellDealerInfo;

void Freecell::matchVariant()
{
    if (m_emptyStackFill == 0 && m_sequenceBuiltBy == 1 && m_reserves == 4 && m_stacks == 2)
        m_variation = 0;
    else if (m_emptyStackFill == 1 && m_sequenceBuiltBy == 1 && m_reserves == 8 && m_stacks == 2)
        m_variation = 1;
    else if (m_emptyStackFill == 1 && m_sequenceBuiltBy == 0 && m_reserves == 4 && m_stacks == 2)
        m_variation = 2;
    else if (m_emptyStackFill == 0 && m_sequenceBuiltBy == 0 && m_reserves == 4 && m_stacks == 2)
        m_variation = 3;
    else if (m_emptyStackFill == 1 && m_sequenceBuiltBy == 1 && m_reserves == 4 && m_stacks == 4)
        m_variation = 4;
    else
        m_variation = 5;

    options->setCurrentItem(m_variation);
}

void Freecell::configOptions()
{
    options = new KSelectAction(i18nc("@title:menu", "Popular Variant Presets"), this);
    options->addAction(i18nc("@item:inmenu preset", "Baker's Game"));
    options->addAction(i18nc("@item:inmenu preset", "Eight Off"));
    options->addAction(i18nc("@item:inmenu preset", "Forecell"));
    options->addAction(i18nc("@item:inmenu preset", "Freecell"));
    options->addAction(i18nc("@item:inmenu preset", "Seahaven Towers"));
    options->addAction(i18nc("@item:inmenu preset", "Custom"));

    m_emptyStackFillOption = new KSelectAction(i18nc("@title:menu", "Empty Stack Fill"), this);
    m_emptyStackFillOption->addAction(i18nc("@item:inmenu empty stack fill", "Any (Easy)"));
    m_emptyStackFillOption->addAction(i18nc("@item:inmenu empty stack fill", "Kings only (Medium)"));
    m_emptyStackFillOption->addAction(i18nc("@item:inmenu empty stack fill", "None (Hard)"));

    m_sequenceBuiltByOption = new KSelectAction(i18nc("@title:menu", "Build Sequence"), this);
    m_sequenceBuiltByOption->addAction(i18nc("@item:inmenu build sequence", "Alternating Color"));
    m_sequenceBuiltByOption->addAction(i18nc("@item:inmenu build sequence", "Matching Suit"));
    m_sequenceBuiltByOption->addAction(i18nc("@item:inmenu build sequence", "Rank"));

    m_reservesOption = new KSelectAction(i18nc("@title:menu", "Free Cells"), this);
    m_reservesOption->addAction(i18n("0"));
    m_reservesOption->addAction(i18n("1"));
    m_reservesOption->addAction(i18n("2"));
    m_reservesOption->addAction(i18n("3"));
    m_reservesOption->addAction(i18n("4"));
    m_reservesOption->addAction(i18n("5"));
    m_reservesOption->addAction(i18n("6"));
    m_reservesOption->addAction(i18n("7"));
    m_reservesOption->addAction(i18n("8"));

    m_stacksOption = new KSelectAction(i18nc("@title:menu", "Stacks"), this);
    m_stacksOption->addAction(i18n("6"));
    m_stacksOption->addAction(i18n("7"));
    m_stacksOption->addAction(i18n("8"));
    m_stacksOption->addAction(i18n("9"));
    m_stacksOption->addAction(i18n("10"));
    m_stacksOption->addAction(i18n("11"));
    m_stacksOption->addAction(i18n("12"));

    m_decksOption = new KSelectAction(i18nc("@title:menu", "Decks"), this);
    m_decksOption->addAction(i18n("1"));
    m_decksOption->addAction(i18n("2"));
    m_decksOption->addAction(i18n("3"));

    connect(options, &KSelectAction::indexTriggered, this, &Freecell::gameTypeChanged);
    connect(m_emptyStackFillOption, &KSelectAction::indexTriggered, this, &Freecell::gameTypeChanged);
    connect(m_reservesOption, &KSelectAction::indexTriggered, this, &Freecell::gameTypeChanged);
    connect(m_sequenceBuiltByOption, &KSelectAction::indexTriggered, this, &Freecell::gameTypeChanged);
    connect(m_stacksOption, &KSelectAction::indexTriggered, this, &Freecell::gameTypeChanged);
    connect(m_decksOption, &KSelectAction::indexTriggered, this, &Freecell::gameTypeChanged);
}

void Freecell::setSavedOptions()
{
    Settings::setFreecellEmptyStackFill(m_emptyStackFill);
    Settings::setFreecellSequenceBuiltBy(m_sequenceBuiltBy);
    Settings::setFreecellReserves(m_reserves);
    Settings::setFreecellStacks(m_stacks);
    Settings::setFreecellDecks(m_decks);
}

void Freecell::getSavedOptions()
{
    m_emptyStackFill = Settings::freecellEmptyStackFill();
    m_sequenceBuiltBy = Settings::freecellSequenceBuiltBy();
    m_reserves = Settings::freecellReserves();
    m_stacks = Settings::freecellStacks();
    m_decks = Settings::freecellDecks();

    matchVariant();

    m_emptyStackFillOption->setCurrentItem(m_emptyStackFill);
    m_sequenceBuiltByOption->setCurrentItem(m_sequenceBuiltBy);
    m_reservesOption->setCurrentItem(m_reserves);
    m_stacksOption->setCurrentItem(m_stacks);
    m_decksOption->setCurrentItem(m_decks);
}

void Freecell::mapOldId(int id)
{
    switch (id) {
    case DealerInfo::FreecellBakersId:
        setOptions(0);
        break;
    case DealerInfo::FreecellEightOffId:
        setOptions(1);
        break;
    case DealerInfo::FreecellForeId:
        setOptions(2);
        break;
    case DealerInfo::FreecellId:
        setOptions(3);
        break;
    case DealerInfo::FreecellSeahavenId:
        setOptions(4);
        break;
    case DealerInfo::FreecellCustomId:
        setOptions(5);
        break;
    default:
        // Do nothing.
        break;
    }
}

int Freecell::oldId() const
{
    switch (m_variation) {
    case 0:
        return DealerInfo::FreecellBakersId;
    case 1:
        return DealerInfo::FreecellEightOffId;
    case 2:
        return DealerInfo::FreecellForeId;
    case 3:
        return DealerInfo::FreecellId;
    case 4:
        return DealerInfo::FreecellSeahavenId;
    default:
        return DealerInfo::FreecellCustomId;
    }
}

void Freecell::setOptions(int variation)
{
    if (variation != m_variation) {
        m_variation = variation;
        m_emptyStackFill = 0;
        m_sequenceBuiltBy = 0;
        m_reserves = 4;
        m_stacks = 2;
        m_decks = 0;

        switch (m_variation) {
        case 0:
            m_sequenceBuiltBy = 1;
            break;
        case 1:
            m_emptyStackFill = 1;
            m_sequenceBuiltBy = 1;
            m_reserves = 8;
            break;
        case 2:
            m_emptyStackFill = 1;
            break;
        case 3:
            break;
        case 4:
            m_emptyStackFill = 1;
            m_sequenceBuiltBy = 1;
            m_stacks = 4;
            break;
        case 5:
            m_emptyStackFill = 2;
            m_sequenceBuiltBy = 2;
            break;
        }

        m_emptyStackFillOption->setCurrentItem(m_emptyStackFill);
        m_sequenceBuiltByOption->setCurrentItem(m_sequenceBuiltBy);
        m_reservesOption->setCurrentItem(m_reserves);
        m_stacksOption->setCurrentItem(m_stacks);
        m_decksOption->setCurrentItem(m_decks);
    }
}

#include "moc_freecell.cpp"
