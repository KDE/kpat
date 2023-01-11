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

#ifndef CASTLE_H
#define CASTLE_H

// own
#include "dealer.h"
#include "hint.h"

class KSelectAction;

class Castle : public DealerScene
{
    Q_OBJECT

public:
    explicit Castle(const DealerInfo *di);
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
    PatPile *store[10];
    PatPile *freecell[4];
    PatPile *target[8];

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

    KSelectAction *m_stackFaceupOption;
    int m_stackFaceup;

    KSelectAction *m_decksOption;
    int m_decks;

    KSelectAction *m_foundationOption;
    int m_foundation;

    KSelectAction *m_layoutOption;
    int m_layout;

    friend class CastleSolver;
};

#endif
