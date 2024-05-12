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

#include "castle.h"

// own
#include "dealerinfo.h"
#include "kpat_debug.h"
#include "patsolve/castlesolver.h"
#include "pileutils.h"
#include "settings.h"
#include "speeds.h"
// KF
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KSelectAction>

Castle::Castle(const DealerInfo *di)
    : DealerScene(di)
{
    configOptions();
    getSavedOptions();
}

void Castle::initialize()
{
    setDeckContents(m_decks);

    const qreal topRowDist = 1.125;
    const qreal bottomRowDist = 1.125;
    const qreal offsetY = m_reserves > 0 ? 1.3 : 0;

    for (int i = 0; i < m_reserves; ++i) {
        freecell[i] = new PatPile(this, 1 + i, QStringLiteral("freecell%1").arg(i));
        freecell[i]->setPileRole(PatPile::Cell);
        freecell[i]->setLayoutPos(3.25 - topRowDist * (m_reserves - 1) / 2 + topRowDist * i, 0);
        freecell[i]->setKeyboardSelectHint(KCardPile::AutoFocusTop);
        freecell[i]->setKeyboardDropHint(KCardPile::AutoFocusTop);
    }

    for (int i = 0; i < m_stacks; ++i) {
        store[i] = new PatPile(this, 1 + i, QStringLiteral("store%1").arg(i));
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setAutoTurnTop(true);
        if (m_layout == 1) {
            store[i]->setLayoutPos(4.5 * (i % 2) + 0, offsetY + bottomRowDist * static_cast<int>((i + 0) / 2));
        } else {
            store[i]->setLayoutPos(2.5 * (i % 2) + 2, offsetY + bottomRowDist * static_cast<int>((i + 0) / 2));
        }

        if (i % 2 || m_layout == 1) {
            store[i]->setSpread(0.21, 0);
            store[i]->setRightPadding(2);
            store[i]->setWidthPolicy(KCardPile::GrowRight);
        } else {
            store[i]->setSpread(-0.21, 0);
            store[i]->setLeftPadding(3);
            store[i]->setWidthPolicy(KCardPile::GrowLeft);
        }

        store[i]->setKeyboardSelectHint(KCardPile::AutoFocusDeepestRemovable);
        store[i]->setKeyboardDropHint(KCardPile::AutoFocusTop);
    }

    for (int i = 0; i < 4 * m_decks; ++i) {
        target[i] = new PatPile(this, 1 + i, QStringLiteral("target%1").arg(i));
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setLayoutPos(3.25, offsetY + bottomRowDist * i);
        target[i]->setKeyboardSelectHint(KCardPile::NeverFocus);
        target[i]->setKeyboardDropHint(KCardPile::ForceFocusTop);
    }

    setActions(DealerScene::Demo | DealerScene::Hint);
    auto solver = new CastleSolver(this);
    solver->default_max_positions = Settings::castleSolverIterationsLimit();
    setSolver(solver);
    setNeededFutureMoves(4); // reserve some
}

QList<QAction *> Castle::configActions() const
{
    return QList<QAction *>() << options << m_emptyStackFillOption << m_sequenceBuiltByOption << m_reservesOption << m_stacksOption << m_stackFaceupOption
                              << m_foundationOption << m_layoutOption;
}

void Castle::gameTypeChanged()
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
            else if (m_stacks - 6 != m_stacksOption->currentItem())
                m_stacks = m_stacksOption->currentItem() + 6;
            else if (m_stackFaceup != m_stackFaceupOption->currentItem())
                m_stackFaceup = m_stackFaceupOption->currentItem();
            else if (m_foundation != m_foundationOption->currentItem())
                m_foundation = m_foundationOption->currentItem();
            else if (m_layout != m_layoutOption->currentItem())
                m_layout = m_layoutOption->currentItem();

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

void Castle::restart(const QList<KCard *> &cards)
{
    QList<KCard *> cardList = cards;

    int column = 0;
    int fdx = 0;

    // Prefill aces to foundation for select game types
    if (m_foundation >= 1) {
        for (int i = 0; i < cardList.size(); ++i) {
            if (cardList.at(i)->rank() == KCardDeck::Ace) {
                addCardForDeal(target[fdx], cardList.takeAt(i), true, target[fdx]->pos());
                --i;
                fdx++;
            }
        }
    }

    bool alternate = false;
    while (!cardList.isEmpty()) {
        // Add matching cards to foundation during deal for select game types
        if (m_foundation == 2 && fdx > 0) {
            KCard *card = cardList.last();
            int i = 0;
            for (i = 0; i < fdx; ++i) {
                if (card->rank() == target[i]->topCard()->rank() + 1 && card->suit() == target[i]->topCard()->suit()) {
                    addCardForDeal(target[i], cardList.takeLast(), true, target[i]->pos());
                    column = (column + 1) % m_stacks;
                    break;
                }
            }

            // Skip if card already added
            if (i < fdx)
                continue;
        }

        bool faceUp = m_stackFaceup == 1 || cardList.length() <= m_stacks || (m_stackFaceup == 2 && alternate);
        addCardForDeal(store[column], cardList.takeLast(), faceUp, store[0]->pos());
        column = (column + 1) % m_stacks;
        if (column % m_stacks == 0)
            alternate = !alternate;
    }

    startDealAnimation();
}

QString Castle::solverFormat() const
{
    QString output;
    QString tmp;
    for (int i = 0; i < 4 * m_decks; i++) {
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

    for (int i = 0; i < m_stacks; i++)
        cardsListToLine(output, store[i]->cards());
    return output;
}

void Castle::cardsDroppedOnPile(const QList<KCard *> &cards, KCardPile *pile)
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
    for (int i = 0; i < m_stacks; ++i)
        if (store[i]->isEmpty() && store[i] != pile)
            freeStores << store[i];

    multiStepMove(cards, pile, freeStores, freeCells, DURATION_MOVE);
}

bool Castle::tryAutomaticMove(KCard *c)
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

bool Castle::canPutStore(const KCardPile *pile, const QList<KCard *> &cards) const
{
    int freeCells = 0;
    for (int i = 0; i < m_reserves; ++i)
        if (freecell[i]->isEmpty())
            ++freeCells;

    int freeStores = 0;
    if (m_emptyStackFill == 0) {
        for (int i = 0; i < m_stacks; ++i)
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

bool Castle::checkAdd(const PatPile *pile, const QList<KCard *> &oldCards, const QList<KCard *> &newCards) const
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

bool Castle::checkRemove(const PatPile *pile, const QList<KCard *> &cards) const
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
        return true;
    default:
        return false;
    }
}

static class CastleDealerInfo : public DealerInfo
{
public:
    CastleDealerInfo()
        : DealerInfo(kli18n("Castle"),
                     CastleGeneralId,
                     {
                         {CastleBeleagueredId, kli18n("Beleaguered Castle")},
                         {CastleCitadelId, kli18n("Citadel")},
                         {CastleExiledKingsId, kli18n("Exiled Kings")},
                         {CastleStreetAlleyId, kli18n("Streets and Alleys")},
                         {CastleSiegecraftId, kli18n("Siegecraft")},
                         {CastleStrongholdId, kli18n("Stronghold")},
                         {CastleCustomId, kli18n("Castle (Custom)")},
                     })
    {
    }

    DealerScene *createGame() const override
    {
        return new Castle(this);
    }
} castleDealerInfo;

void Castle::matchVariant()
{
    if (m_emptyStackFill == 0 && m_sequenceBuiltBy == 2 && m_reserves == 0 && m_stacks == 8 && m_foundation == 1)
        m_variation = 0;
    else if (m_emptyStackFill == 0 && m_sequenceBuiltBy == 2 && m_reserves == 0 && m_stacks == 8 && m_foundation == 2)
        m_variation = 1;
    else if (m_emptyStackFill == 1 && m_sequenceBuiltBy == 2 && m_reserves == 0 && m_stacks == 8 && m_foundation == 2)
        m_variation = 2;
    else if (m_emptyStackFill == 0 && m_sequenceBuiltBy == 2 && m_reserves == 0 && m_stacks == 8 && m_foundation == 0)
        m_variation = 3;
    else if (m_emptyStackFill == 0 && m_sequenceBuiltBy == 2 && m_reserves == 1 && m_stacks == 8 && m_foundation == 1)
        m_variation = 4;
    else if (m_emptyStackFill == 0 && m_sequenceBuiltBy == 2 && m_reserves == 1 && m_stacks == 8 && m_foundation == 0)
        m_variation = 5;
    else
        m_variation = 6;

    options->setCurrentItem(m_variation);
}

void Castle::configOptions()
{
    options = new KSelectAction(i18nc("@title:menu", "Popular Variant Presets"), this);
    options->addAction(i18nc("@item:inmenu preset", "Beleaguered Castle"));
    options->addAction(i18nc("@item:inmenu preset", "Citadel"));
    options->addAction(i18nc("@item:inmenu preset", "Exiled Kings"));
    options->addAction(i18nc("@item:inmenu preset", "Streets and Alleys"));
    options->addAction(i18nc("@item:inmenu preset", "Siegecraft"));
    options->addAction(i18nc("@item:inmenu preset", "Stronghold"));
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

    m_stacksOption = new KSelectAction(i18nc("@title:menu", "Stacks"), this);
    m_stacksOption->addAction(i18n("6"));
    m_stacksOption->addAction(i18n("7"));
    m_stacksOption->addAction(i18n("8"));
    m_stacksOption->addAction(i18n("9"));
    m_stacksOption->addAction(i18n("10"));

    m_stackFaceupOption = new KSelectAction(i18nc("@title:menu", "Stack Options"), this);
    m_stackFaceupOption->addAction(i18nc("@item:inmenu stack option", "Face Down"));
    m_stackFaceupOption->addAction(i18nc("@item:inmenu stack option", "Face Up"));
    m_stackFaceupOption->addAction(i18nc("@item:inmenu stack option", "Alternating Face Up"));

    m_foundationOption = new KSelectAction(i18nc("@title:menu", "Foundation Deal"), this);
    m_foundationOption->addAction(i18nc("@item:inmenu foundation deal", "None"));
    m_foundationOption->addAction(i18nc("@item:inmenu foundation deal", "Aces"));
    m_foundationOption->addAction(i18nc("@item:inmenu foundation deal", "Any"));

    m_layoutOption = new KSelectAction(i18nc("@title:menu", "Layout"), this);
    m_layoutOption->addAction(i18nc("@item:inmenu layout", "Classic"));
    m_layoutOption->addAction(i18nc("@item:inmenu layout", "Modern"));

    connect(options, &KSelectAction::indexTriggered, this, &Castle::gameTypeChanged);
    connect(m_emptyStackFillOption, &KSelectAction::indexTriggered, this, &Castle::gameTypeChanged);
    connect(m_reservesOption, &KSelectAction::indexTriggered, this, &Castle::gameTypeChanged);
    connect(m_sequenceBuiltByOption, &KSelectAction::indexTriggered, this, &Castle::gameTypeChanged);
    connect(m_stacksOption, &KSelectAction::indexTriggered, this, &Castle::gameTypeChanged);
    connect(m_stackFaceupOption, &KSelectAction::indexTriggered, this, &Castle::gameTypeChanged);
    connect(m_foundationOption, &KSelectAction::indexTriggered, this, &Castle::gameTypeChanged);
    connect(m_layoutOption, &KSelectAction::indexTriggered, this, &Castle::gameTypeChanged);
}

void Castle::setSavedOptions()
{
    Settings::setCastleEmptyStackFill(m_emptyStackFill);
    Settings::setCastleSequenceBuiltBy(m_sequenceBuiltBy);
    Settings::setCastleReserves(m_reserves);
    Settings::setCastleStacks(m_stacks);
    Settings::setCastleStackFaceup(m_stackFaceup);
    Settings::setCastleFoundation(m_foundation);
    Settings::setCastleLayout(m_layout);
}

void Castle::getSavedOptions()
{
    m_emptyStackFill = Settings::castleEmptyStackFill();
    m_sequenceBuiltBy = Settings::castleSequenceBuiltBy();
    m_reserves = Settings::castleReserves();
    m_stacks = Settings::castleStacks();
    m_stackFaceup = Settings::castleStackFaceup();
    m_foundation = Settings::castleFoundation();
    m_layout = Settings::castleLayout();
    m_decks = 1;

    if (m_stacks < 6)
        m_stacks = 6;

    matchVariant();

    m_emptyStackFillOption->setCurrentItem(m_emptyStackFill);
    m_sequenceBuiltByOption->setCurrentItem(m_sequenceBuiltBy);
    m_reservesOption->setCurrentItem(m_reserves);
    m_stacksOption->setCurrentItem(m_stacks - 6);
    m_stackFaceupOption->setCurrentItem(m_stackFaceup);
    m_foundationOption->setCurrentItem(m_foundation);
    m_layoutOption->setCurrentItem(m_layout);
}

void Castle::mapOldId(int id)
{
    switch (id) {
    case DealerInfo::CastleBeleagueredId:
        setOptions(0);
        break;
    case DealerInfo::CastleCitadelId:
        setOptions(1);
        break;
    case DealerInfo::CastleExiledKingsId:
        setOptions(2);
        break;
    case DealerInfo::CastleStreetAlleyId:
        setOptions(3);
        break;
    case DealerInfo::CastleSiegecraftId:
        setOptions(4);
        break;
    case DealerInfo::CastleStrongholdId:
        setOptions(5);
        break;
    case DealerInfo::CastleCustomId:
        setOptions(6);
        break;
    default:
        // Do nothing.
        break;
    }
}

int Castle::oldId() const
{
    switch (m_variation) {
    case 0:
        return DealerInfo::CastleBeleagueredId;
    case 1:
        return DealerInfo::CastleCitadelId;
    case 2:
        return DealerInfo::CastleExiledKingsId;
    case 3:
        return DealerInfo::CastleStreetAlleyId;
    case 4:
        return DealerInfo::CastleSiegecraftId;
    case 5:
        return DealerInfo::CastleStrongholdId;
    default:
        return DealerInfo::CastleCustomId;
    }
}

void Castle::setOptions(int variation)
{
    if (variation != m_variation) {
        m_variation = variation;
        m_emptyStackFill = 0;
        m_sequenceBuiltBy = 2;
        m_reserves = 0;
        m_stacks = 8;
        m_stackFaceup = 1;
        m_decks = 1;
        m_foundation = 1;

        switch (m_variation) {
        case 0:
            break;
        case 1:
            m_foundation = 2;
            break;
        case 2:
            m_foundation = 2;
            m_emptyStackFill = 1;
            break;
        case 3:
            m_foundation = 0;
            break;
        case 4:
            m_reserves = 1;
            break;
        case 5:
            m_foundation = 0;
            m_reserves = 1;
            break;
        case 6:
            m_sequenceBuiltBy = 0;
            break;
        }

        m_emptyStackFillOption->setCurrentItem(m_emptyStackFill);
        m_sequenceBuiltByOption->setCurrentItem(m_sequenceBuiltBy);
        m_reservesOption->setCurrentItem(m_reserves);
        m_stacksOption->setCurrentItem(m_stacks - 6);
        m_stackFaceupOption->setCurrentItem(m_stackFaceup);
        m_foundationOption->setCurrentItem(m_foundation);
    }
}

#include "moc_castle.cpp"
