/*
 *  Copyright (C) 2010 Parker Coates <coates@kde.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "pileutils.h"

// KCardGame
#include <KCard>
#include <KCardDeck>

namespace
{
inline int alternateColor(int color)
{
    return color == KCardDeck::Red ? KCardDeck::Black : KCardDeck::Red;
}
}

bool isSameSuitAscending(const QList<KCard *> &cards)
{
    if (cards.size() <= 1)
        return true;

    int suit = cards.first()->suit();
    int lastRank = cards.first()->rank();

    for (int i = 1; i < cards.size(); ++i) {
        ++lastRank;
        if (cards[i]->suit() != suit || cards[i]->rank() != lastRank)
            return false;
    }
    return true;
}

int countSameSuitDescendingSequences(const QList<KCard *> &cards)
{
    if (cards.size() <= 1)
        return 0;

    int suit = cards.first()->suit();
    int lastRank = cards.first()->rank();

    int count = 1;

    for (int i = 1; i < cards.size(); ++i) {
        --lastRank;

        if (cards[i]->rank() != lastRank)
            return -1;

        if (cards[i]->suit() != suit) {
            count++;
            suit = cards[i]->suit();
        }
    }
    return count;
}

bool isSameSuitDescending(const QList<KCard *> &cards)
{
    if (cards.size() <= 1)
        return true;

    int suit = cards.first()->suit();
    int lastRank = cards.first()->rank();

    for (int i = 1; i < cards.size(); ++i) {
        --lastRank;
        if (cards[i]->suit() != suit || cards[i]->rank() != lastRank)
            return false;
    }
    return true;
}

bool isAlternateColorDescending(const QList<KCard *> &cards)
{
    if (cards.size() <= 1)
        return true;

    int lastColor = cards.first()->color();
    int lastRank = cards.first()->rank();

    for (int i = 1; i < cards.size(); ++i) {
        lastColor = alternateColor(lastColor);
        --lastRank;

        if (cards[i]->color() != lastColor || cards[i]->rank() != lastRank)
            return false;
    }
    return true;
}

bool isRankDescending(const QList<KCard *> &cards)
{
    if (cards.size() <= 1)
        return true;

    int lastRank = cards.first()->rank();

    for (int i = 1; i < cards.size(); ++i) {
        --lastRank;
        if (cards[i]->rank() != lastRank)
            return false;
    }
    return true;
}

bool checkAddSameSuitAscendingFromAce(const QList<KCard *> &oldCards, const QList<KCard *> &newCards)
{
    if (!isSameSuitAscending(newCards))
        return false;

    if (oldCards.isEmpty())
        return newCards.first()->rank() == KCardDeck::Ace;
    else
        return newCards.first()->suit() == oldCards.last()->suit() && newCards.first()->rank() == oldCards.last()->rank() + 1;
}

bool checkAddAlternateColorDescending(const QList<KCard *> &oldCards, const QList<KCard *> &newCards)
{
    return isAlternateColorDescending(newCards)
        && (oldCards.isEmpty()
            || (newCards.first()->color() == alternateColor(oldCards.last()->color()) && newCards.first()->rank() == oldCards.last()->rank() - 1));
}

bool checkAddAlternateColorDescendingFromKing(const QList<KCard *> &oldCards, const QList<KCard *> &newCards)
{
    if (!isAlternateColorDescending(newCards))
        return false;

    if (oldCards.isEmpty())
        return newCards.first()->rank() == KCardDeck::King;
    else
        return newCards.first()->color() == alternateColor(oldCards.last()->color()) && newCards.first()->rank() == oldCards.last()->rank() - 1;
}

QString suitToString(int s)
{
    switch (s) {
    case KCardDeck::Clubs:
        return QStringLiteral("C");
    case KCardDeck::Hearts:
        return QStringLiteral("H");
    case KCardDeck::Diamonds:
        return QStringLiteral("D");
    case KCardDeck::Spades:
        return QStringLiteral("S");
    default:
        exit(-1);
    }
    return QString();
}

QString rankToString(int r)
{
    switch (r) {
    case KCardDeck::King:
        return QStringLiteral("K");
    case KCardDeck::Ace:
        return QStringLiteral("A");
    case KCardDeck::Jack:
        return QStringLiteral("J");
    case KCardDeck::Queen:
        return QStringLiteral("Q");
    case KCardDeck::Ten:
        return QStringLiteral("T");
    default:
        return QString::number(r);
    }
}

QString cardToRankSuitString(const KCard *const card)
{
    return rankToString(card->rank()) + suitToString(card->suit());
}

void cardsListToLine(QString &output, const QList<KCard *> &cards)
{
    bool first = true;
    for (QList<KCard *>::ConstIterator it = cards.constBegin(); it != cards.constEnd(); ++it) {
        if (!first) {
            output += QLatin1Char(' ');
        }
        first = false;
        output += cardToRankSuitString(*it);
    }
    output += QLatin1Char('\n');
}
