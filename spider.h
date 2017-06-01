/*
 * Copyright (C) 2003 Josh Metzler <joshdeb@metzlers.org>
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

#ifndef SPIDER_H
#define SPIDER_H

#include "dealer.h"

class KSelectAction;


class Spider : public DealerScene
{
    Q_OBJECT

public:
    explicit Spider( const DealerInfo * di );
    void initialize() Q_DECL_OVERRIDE;
    void mapOldId(int id) Q_DECL_OVERRIDE;
    int oldId() const Q_DECL_OVERRIDE;
    QList<QAction*> configActions() const Q_DECL_OVERRIDE;

protected:
    QString getGameState() const Q_DECL_OVERRIDE;
    void setGameState( const QString & state ) Q_DECL_OVERRIDE;
    QString getGameOptions() const Q_DECL_OVERRIDE;
    void setGameOptions( const QString & options ) Q_DECL_OVERRIDE;
    bool checkAdd(const PatPile * pile, const QList<KCard*> & oldCards, const QList<KCard*> & newCards) const Q_DECL_OVERRIDE;
    bool checkRemove(const PatPile * pile, const QList<KCard*> & cards) const Q_DECL_OVERRIDE;
    void cardsMoved( const QList<KCard*> & cards, KCardPile * oldPile, KCardPile * newPile ) Q_DECL_OVERRIDE;
    void restart( const QList<KCard*> & cards ) Q_DECL_OVERRIDE;

protected slots:
    bool newCards() Q_DECL_OVERRIDE;
    void animationDone() Q_DECL_OVERRIDE;

private slots:
    void gameTypeChanged();

private:
    bool pileHasFullRun( KCardPile * pile );
    void moveFullRunToLeg( KCardPile * pile );
    void setSuits(int s);
    void createDeck();
    QPointF randomPos();

    PatPile *stack[10];
    PatPile *legs[8];
    int m_leg;
    PatPile *redeals[5];
    int m_redeal;
    int m_suits;
    QList<KCardPile*> m_pilesWithRuns;

    KSelectAction *options;

    KSelectAction *m_stackFaceupOption;
    int m_stackFaceup;

    friend class SpiderSolver;
};

#endif
