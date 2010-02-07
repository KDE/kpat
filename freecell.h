/*
 * Copyright (C) 1997 Rodolfo Borges <barrett@labma.ufrj.br>
 * Copyright (C) 1998-2009 Stephan Kulow <coolo@kde.org>
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

#ifndef FREECELL_H
#define FREECELL_H

#include "dealer.h"
#include "hint.h"


class FreecellPile : public PatPile
{
public:
    explicit FreecellPile(int _index, const QString & objectName = QString()) : PatPile(_index, objectName) {}
    virtual void moveCards(QList<KCard*> &c, Pile *to);
};

class Freecell : public DealerScene
{
    friend class FreecellSolver;

    Q_OBJECT

public:
    Freecell();
    void moveCards(QList<KCard*> &c, FreecellPile *from, PatPile *to);
    virtual ~Freecell();

public slots:
    virtual void restart();
    void waitForMoving();
    void startMoving();

protected:
    virtual bool checkAdd(const PatPile * pile, const QList<StandardCard*> & oldCards, const QList<StandardCard*> & newCards) const;
    virtual bool checkRemove(const PatPile * pile, const QList<StandardCard*> & cards) const;

    bool canPutStore(const PatPile *c1, const QList<StandardCard*>& c2) const;

    void countFreeCells(int &free_cells, int &free_stores) const;

    void movePileToPile(QList<StandardCard*> &c, PatPile *to, QList<PatPile*> & fss, QList<PatPile*> & fcs,
                        int start, int count, int debug_level);

    Pile *pileForName(QString line) const;

    MoveHint *translateMove(void *m);

    virtual void newDemoMove(KCard *m);
    virtual bool cardDoubleClicked(KCard *c);
    virtual void deal();
    virtual void getHints();

protected:
    FreecellPile* store[8];
    PatPile* freecell[4];
    PatPile* target[4];

private:
    QList<MoveHint*> moves;
    int sum_moves;
    int current_weight;
    bool noLongerNeeded(const KCard &);
};

#endif

//-------------------------------------------------------------------------//
