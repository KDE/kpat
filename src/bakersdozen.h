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

#ifndef BAKERSDOZEN_H
#define BAKERSDOZEN_H

// own
#include "dealer.h"
#include "hint.h"

class KSelectAction;

class BakersDozen : public DealerScene
{
    Q_OBJECT

public:
    explicit BakersDozen(const DealerInfo *di);
    void initialize() override;
    void mapOldId(int id) override;
    int oldId() const override;
    QList<QAction *> configActions() const override;

protected:
    bool checkAdd(const PatPile *pile, const QList<KCard *> &oldCards, const QList<KCard *> &newCards) const override;
    bool checkRemove(const PatPile *pile, const QList<KCard *> &cards) const override;
    void restart(const QList<KCard *> &cards) override;
    // QList<MoveHint> getHints() override;

private Q_SLOTS:
    void gameTypeChanged();

private:
    void setOptions(int v);
    void getSavedOptions();
    void setSavedOptions();
    void matchVariant();

    virtual QString solverFormat() const;
    PatPile *store[13];
    PatPile *target[4];

    KSelectAction *options;
    int m_variation;

    KSelectAction *m_emptyStackFillOption;
    int m_emptyStackFill;

    KSelectAction *m_stackFacedownOption;
    int m_stackFacedown;

    KSelectAction *m_sequenceBuiltByOption;
    int m_sequenceBuiltBy;

    friend class BakersDozenSolver;
};

#endif
