/*
 *   Copyright 2021 Michael Lang <criticaltemp@protonmail.com>
 *
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
 */

#include "bakersdozen.h"

// own
#include "dealerinfo.h"
#include "patsolve/bakersdozensolver.h"
#include "pileutils.h"
#include "settings.h"
#include "speeds.h"
// KF
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KSelectAction>

BakersDozen::BakersDozen(const DealerInfo *di)
    : DealerScene(di)
{
}

void BakersDozen::initialize()
{
    setDeckContents();

    const qreal dist_x = 1.11;

    for (int i = 0; i < 4; ++i) {
        target[i] = new PatPile(this, i + 1, QStringLiteral("target%1").arg(i));
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setLayoutPos((i * 2 + 3) * dist_x, 0);
        target[i]->setSpread(0, 0);
        target[i]->setKeyboardSelectHint(KCardPile::NeverFocus);
        target[i]->setKeyboardDropHint(KCardPile::AutoFocusTop);
    }

    for (int i = 0; i < 13; ++i) {
        store[i] = new PatPile(this, 0 + i, QStringLiteral("store%1").arg(i));
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setLayoutPos(dist_x * i, 1.2);
        store[i]->setAutoTurnTop(true);
        store[i]->setBottomPadding(2.0);
        store[i]->setHeightPolicy(KCardPile::GrowDown);
        store[i]->setZValue(0.01 * i);
        store[i]->setKeyboardSelectHint(KCardPile::FreeFocus);
        store[i]->setKeyboardDropHint(KCardPile::AutoFocusTop);
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    auto solver = new BakersDozenSolver(this);
    solver->default_max_positions = Settings::bakersDozenSolverIterationsLimit();
    setSolver(solver);
    setNeededFutureMoves(4); // reserve some

    options = new KSelectAction(i18nc("@title:menu", "Popular Variant Presets"), this);
    options->addAction(i18nc("@item:inmenu preset", "Baker's Dozen"));
    options->addAction(i18nc("@item:inmenu preset", "Spanish Patience"));
    options->addAction(i18nc("@item:inmenu preset", "Castles in Spain"));
    options->addAction(i18nc("@item:inmenu preset", "Portuguese Solitaire"));
    options->addAction(i18nc("@item:inmenu preset", "Custom"));

    m_emptyStackFillOption = new KSelectAction(i18nc("@title:menu", "Empty Stack Fill"), this);
    m_emptyStackFillOption->addAction(i18nc("@item:inmenu empty stack fill", "Any (Easy)"));
    m_emptyStackFillOption->addAction(i18nc("@item:inmenu empty stack fill", "Kings only (Medium)"));
    m_emptyStackFillOption->addAction(i18nc("@item:inmenu empty stack fill", "None (Hard)"));

    m_stackFacedownOption = new KSelectAction(i18nc("@title:menu", "Stack Options"), this);
    m_stackFacedownOption->addAction(i18nc("@item:inmenu stack option", "Face Up (Easier)"));
    m_stackFacedownOption->addAction(i18nc("@item:inmenu stack option", "Face Down (Harder)"));

    m_sequenceBuiltByOption = new KSelectAction(i18nc("@title:menu", "Build Sequence"), this);
    m_sequenceBuiltByOption->addAction(i18nc("@item:inmenu build sequence", "Alternating Color"));
    m_sequenceBuiltByOption->addAction(i18nc("@item:inmenu build sequence", "Matching Suit"));
    m_sequenceBuiltByOption->addAction(i18nc("@item:inmenu build sequence", "Rank"));

    connect(options, &KSelectAction::indexTriggered, this, &BakersDozen::gameTypeChanged);
    connect(m_emptyStackFillOption, &KSelectAction::indexTriggered, this, &BakersDozen::gameTypeChanged);
    connect(m_stackFacedownOption, &KSelectAction::indexTriggered, this, &BakersDozen::gameTypeChanged);
    connect(m_sequenceBuiltByOption, &KSelectAction::indexTriggered, this, &BakersDozen::gameTypeChanged);

    getSavedOptions();
}

QList<QAction *> BakersDozen::configActions() const
{
    return QList<QAction *>() << options << m_emptyStackFillOption << m_stackFacedownOption << m_sequenceBuiltByOption;
}

void BakersDozen::gameTypeChanged()
{
    stopDemo();

    if (allowedToStartNewGame()) {
        if (m_variation != options->currentItem()) {
            setOptions(options->currentItem());
        } else {
            // update option selections
            if (m_emptyStackFill != m_emptyStackFillOption->currentItem())
                m_emptyStackFill = m_emptyStackFillOption->currentItem();
            else if (m_stackFacedown != m_stackFacedownOption->currentItem())
                m_stackFacedown = m_stackFacedownOption->currentItem();
            else if (m_sequenceBuiltBy != m_sequenceBuiltByOption->currentItem())
                m_sequenceBuiltBy = m_sequenceBuiltByOption->currentItem();

            matchVariant();
        }

        auto solver = new BakersDozenSolver(this);
        solver->default_max_positions = Settings::bakersDozenSolverIterationsLimit();
        setSolver(solver);
        startNew(gameNumber());
        setSavedOptions();
    } else {
        // If we're not allowed, reset the options
        getSavedOptions();
    }
}

void BakersDozen::restart(const QList<KCard *> &cards)
{
    QList<KCard *> cardList = cards;

    QPointF initPos(0, -deck()->cardHeight());

    for (int row = 0; row < 4; ++row) {
        bool isFaceUp = m_stackFacedown == 1 && row < 3 ? false : true;

        for (int column = 0; column < 13; ++column) {
            addCardForDeal(store[column], cardList.takeLast(), isFaceUp, initPos);
        }
    }

    // Move kings to bottom of pile without changing relative order
    for (int column = 0; column < 13; ++column) {
        int counter = 0;

        const auto cards = store[column]->cards();
        for (KCard *c : cards) {
            if (c->rank() == KCardDeck::King) {
                int index = store[column]->indexOf(c);
                store[column]->swapCards(index, counter);
                counter++;

                if (m_stackFacedown == 1)
                    c->setFaceUp(false);
            }
        }
    }

    startDealAnimation();
}

QString BakersDozen::solverFormat() const
{
    QString output;
    QString tmp;
    for (int i = 0; i < 4; i++) {
        if (target[i]->isEmpty())
            continue;
        tmp += suitToString(target[i]->topCard()->suit()) + QLatin1Char('-') + rankToString(target[i]->topCard()->rank()) + QLatin1Char(' ');
    }
    if (!tmp.isEmpty())
        output += QStringLiteral("Foundations: %1\n").arg(tmp);

    for (int i = 0; i < 13; i++)
        cardsListToLine(output, store[i]->cards());
    return output;
}

bool BakersDozen::checkAdd(const PatPile *pile, const QList<KCard *> &oldCards, const QList<KCard *> &newCards) const
{
    if (pile->pileRole() == PatPile::Tableau) {
        int freeStores = 0;
        if (m_emptyStackFill == 0) {
            for (int i = 0; i < 13; ++i)
                if (store[i]->isEmpty() && store[i] != pile)
                    ++freeStores;
        }

        if (newCards.size() <= 1 << freeStores) {
            if (oldCards.isEmpty())
                return m_emptyStackFill == 0 || (m_emptyStackFill == 1 && newCards.first()->rank() == KCardDeck::King);
            else if (m_sequenceBuiltBy == 1)
                return newCards.first()->suit() == oldCards.last()->suit() && newCards.first()->rank() == oldCards.last()->rank() - 1;
            else if (m_sequenceBuiltBy == 0)
                return checkAddAlternateColorDescending(oldCards, newCards);
            else
                return newCards.first()->rank() == oldCards.last()->rank() - 1;
        } else {
            return false;
        }
    } else {
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    }
}

bool BakersDozen::checkRemove(const PatPile *pile, const QList<KCard *> &cards) const
{
    switch (pile->pileRole()) {
    case PatPile::Tableau:
        if (m_emptyStackFill == 2) {
            return cards.size() == 1;
        } else {
            if (m_sequenceBuiltBy == 1)
                return isSameSuitDescending(cards);
            else if (m_sequenceBuiltBy == 0)
                return isAlternateColorDescending(cards);
            else
                return isRankDescending(cards);
        }
    case PatPile::Cell:
        return cards.first() == pile->topCard();
    case PatPile::Foundation:
        return true;
    default:
        return false;
    }
}

static class BakersDozenDealerInfo : public DealerInfo
{
public:
    BakersDozenDealerInfo()
        : DealerInfo(kli18n("Baker's Dozen"),
                     BakersDozenGeneralId,
                     {
                         {BakersDozenId, kli18n("Baker's Dozen")},
                         {BakersDozenSpanishId, kli18n("Spanish Patience")},
                         {BakersDozenCastlesId, kli18n("Castles in Spain")},
                         {BakersDozenPortugueseId, kli18n("Portuguese Solitaire")},
                         {BakersDozenCustomId, kli18n("Baker's Dozen (Custom)")},
                     })
    {
    }

    DealerScene *createGame() const override
    {
        return new BakersDozen(this);
    }
} BakersDozenDealerInfo;

void BakersDozen::matchVariant()
{
    if (m_emptyStackFill == 2 && m_stackFacedown == 0 && m_sequenceBuiltBy == 2)
        m_variation = 0;
    else if (m_emptyStackFill == 0 && m_stackFacedown == 0 && m_sequenceBuiltBy == 2)
        m_variation = 1;
    else if (m_emptyStackFill == 0 && m_stackFacedown == 1 && m_sequenceBuiltBy == 0)
        m_variation = 2;
    else if (m_emptyStackFill == 1 && m_stackFacedown == 0 && m_sequenceBuiltBy == 2)
        m_variation = 3;
    else
        m_variation = 4;

    options->setCurrentItem(m_variation);
}

void BakersDozen::setSavedOptions()
{
    Settings::setBakersDozenEmptyStackFill(m_emptyStackFill);
    Settings::setBakersDozenStackFacedown(m_stackFacedown);
    Settings::setBakersDozenSequenceBuiltBy(m_sequenceBuiltBy);
}

void BakersDozen::getSavedOptions()
{
    m_emptyStackFill = Settings::bakersDozenEmptyStackFill();
    m_stackFacedown = Settings::bakersDozenStackFacedown();
    m_sequenceBuiltBy = Settings::bakersDozenSequenceBuiltBy();

    matchVariant();

    m_emptyStackFillOption->setCurrentItem(m_emptyStackFill);
    m_stackFacedownOption->setCurrentItem(m_stackFacedown);
    m_sequenceBuiltByOption->setCurrentItem(m_sequenceBuiltBy);
}

void BakersDozen::mapOldId(int id)
{
    switch (id) {
    case DealerInfo::BakersDozenId:
        setOptions(0);
        break;
    case DealerInfo::BakersDozenSpanishId:
        setOptions(1);
        break;
    case DealerInfo::BakersDozenCastlesId:
        setOptions(2);
        break;
    case DealerInfo::BakersDozenPortugueseId:
        setOptions(3);
        break;
    case DealerInfo::BakersDozenCustomId:
        setOptions(4);
        break;
    default:
        // Do nothing.
        break;
    }
}

int BakersDozen::oldId() const
{
    switch (m_variation) {
    case 0:
        return DealerInfo::BakersDozenId;
    case 1:
        return DealerInfo::BakersDozenSpanishId;
    case 2:
        return DealerInfo::BakersDozenCastlesId;
    case 3:
        return DealerInfo::BakersDozenPortugueseId;
    default:
        return DealerInfo::BakersDozenCustomId;
    }
}

void BakersDozen::setOptions(int variation)
{
    if (variation != m_variation) {
        m_variation = variation;

        switch (m_variation) {
        case 0:
            m_emptyStackFill = 2;
            m_stackFacedown = 0;
            m_sequenceBuiltBy = 2;
            break;
        case 1:
            m_emptyStackFill = 0;
            m_stackFacedown = 0;
            m_sequenceBuiltBy = 2;
            break;
        case 2:
            m_emptyStackFill = 0;
            m_stackFacedown = 1;
            m_sequenceBuiltBy = 0;
            break;
        case 3:
            m_emptyStackFill = 1;
            m_stackFacedown = 0;
            m_sequenceBuiltBy = 2;
            break;
        case 4:
            m_emptyStackFill = 0;
            m_stackFacedown = 1;
            m_sequenceBuiltBy = 1;
            break;
        }

        m_emptyStackFillOption->setCurrentItem(m_emptyStackFill);
        m_stackFacedownOption->setCurrentItem(m_stackFacedown);
        m_sequenceBuiltByOption->setCurrentItem(m_sequenceBuiltBy);
    }
}

#include "moc_bakersdozen.cpp"
