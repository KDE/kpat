/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
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
// Based on main.cpp
#include <QTest>
#include "dealer.h"
#include "dealerinfo.h"
#include "golf.h"
#include "../kpat_debug.h"

#include <cassert>

class TestSolverFormat: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void solverFormat_deal1();
};

static DealerScene *getDealer( int wanted_game )
{
    fprintf(stderr, "diuuuuu=%d\n", wanted_game);
    const auto games = DealerInfoList::self()->games();
    for (DealerInfo * di : games) {
        fprintf(stderr, "di=%p\n", (void *)di);
        if ( di->providesId( wanted_game ) )
        {
            DealerScene * d = di->createGame();
            Q_ASSERT( d );
            d->setDeck( new KCardDeck( KCardTheme(), d ) );
            d->initialize();

            if ( !d->solver() )
            {
                qCCritical(KPAT_LOG) << "There is no solver for" << di->nameForId( wanted_game );;
                return nullptr;
            }

            return d;
        }
    }
    return nullptr;
}

void TestSolverFormat::solverFormat_deal1()
{
    DealerScene *dealer = getDealer(DealerInfo::GolfId);
    assert(dealer);
    dealer->deck()->stopAnimations();
    dealer->startNew(1);
    QString have = static_cast<Golf *>(dealer)->solverFormat();
    QString want(
        "Foundations: TH\n"
        "Talon: 8H 2C JH 7D 6D 8S 8D QS 6C 3D 8C TC 6S 9C 2H 6H\n"
        "JD 5H KH AS 4H\n"
        "2D KD 3H AH AC\n"
        "9H KC 2S 3C 4D\n"
        "JC 9S KS 4C 7S\n"
        "5D 5S 9D 5C 3S\n"
        "7H AD QD TS TD\n"
        "7C QC JS QH 4S\n"
    );
    QCOMPARE(have, want);
    delete dealer;
}

QTEST_MAIN(TestSolverFormat)
#include "solver_format.moc"
