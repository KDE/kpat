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

// own
#include "dealer.h"
#include "hint.h"

class KSelectAction;

class Freecell : public DealerScene
{
    Q_OBJECT

public:
    explicit Freecell(const DealerInfo *di);
    void initialize() override;
    void mapOldId(int id) override;
    int oldId() const override;
    QList<QAction *> configActions() const override;

protected:
    bool checkAdd(const PatPile *pile, const QList<KCard *> &oldCards, const QList<KCard *> &newCards) const override;
    bool checkRemove(const PatPile *pile, const QList<KCard *> &cards) const override;
    void cardsDroppedOnPile(const QList<KCard *> &cards, KCardPile *pile) override;
    void restart(const QList<KCard *> &cards) override;

private Q_SLOTS:
    void gameTypeChanged();

protected Q_SLOTS:
    bool tryAutomaticMove(KCard *c) override;

private:
    bool canPutStore(const KCardPile *pile, const QList<KCard *> &cards) const;

    void configOptions();
    void setOptions(int v);
    void getSavedOptions();
    void setSavedOptions();
    void matchVariant();

    virtual QString solverFormat() const;
    PatPile *store[12];
    PatPile *freecell[8];
    PatPile *target[12];

    KSelectAction *options;
    int m_variation;

    KSelectAction *m_emptyStackFillOption;
    int m_emptyStackFill;

    KSelectAction *m_sequenceBuiltByOption;
    int m_sequenceBuiltBy;

    KSelectAction *m_reservesOption;
    int m_reserves;

    KSelectAction *m_stacksOption;
    int m_stacks;

    KSelectAction *m_decksOption;
    int m_decks;

    friend class FreecellSolver;
};

#endif
