/*---------------------------------------------------------------------------

  freecell.cpp  implements a patience card game

     Copyright (C) 1997  Rodolfo Borges
               (C) 2000  Stephan Kulow

 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.

---------------------------------------------------------------------------*/

#include "freecell.h"
#include <klocale.h>
#include "deck.h"
#include <assert.h>
#include <kdebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <qtimer.h>
#include "cardmaps.h"

#include "freecell-solver/fcs_user.h"
#include "freecell-solver/fcs_cl.h"

const int CHUNKSIZE = 100;

void FreecellPile::moveCards(CardList &c, Pile *to)
{
    if (c.count() == 1) {
        Pile::moveCards(c, to);
        return;
    }
    FreecellBase *b = dynamic_cast<FreecellBase*>(dealer());
    if (b) {
        b->moveCards(c, this, to);
    }
}

//-------------------------------------------------------------------------//

FreecellBase::FreecellBase( int decks, int stores, int freecells, int fill, bool unlimit,
                            KMainWindow* parent, const char* name)
    : Dealer(parent,name),
solver_instance(0), es_filling(fill), solver_ret(FCS_STATE_NOT_BEGAN_YET),
unlimited_move(unlimit)
{
    deck = Deck::new_deck(this, decks);
    deck->hide();

    kdDebug(11111) << "cards " << deck->cards().count() << endl;
    Pile *t;
    for (int i = 0; i < stores; i++) {
        FreecellPile *p = new FreecellPile(1 + i, this);
        store.append(p);
        p->setAddFlags(Pile::addSpread | Pile::several);
        p->setRemoveFlags(Pile::several);
        p->setCheckIndex(0);
    }

    for (int i = 0; i < freecells; i++)
    {
        t = new Pile (1 + stores +i, this);
        freecell.append(t);
        t->setType(Pile::FreeCell);
    }

    for (int i = 0; i < decks * 4; i++)
    {
        t = new Pile(1 + stores + freecells +i, this);
        target.append(t);
        t->setType(Pile::KlondikeTarget);
        // COOLO: I'm still not too sure about that t->setRemoveFlags(Pile::Default);
    }

    setActions(Dealer::Demo | Dealer::Hint);
}

FreecellBase::~FreecellBase()
{
    if (solver_instance)
    {
        freecell_solver_user_free(solver_instance);
        solver_instance = NULL;
    }
}
//-------------------------------------------------------------------------//

void FreecellBase::restart()
{
    freeSolution();
    deck->collectAndShuffle();
    deal();
}

QString suitToString(Card::Suit s) {
    switch (s) {
        case Card::Clubs:
            return "C";
        case Card::Hearts:
            return "H";
        case Card::Diamonds:
            return "D";
        case Card::Spades:
            return "S";
    }
    return QString::null;
}

QString rankToString(Card::Rank r)
{
    switch (r) {
        case Card::King:
            return "K";
        case Card::Ace:
            return "A";
        case Card::Jack:
            return "J";
        case Card::Queen:
            return "Q";
        default:
            return QString::number(r);
    }
}

int getDeck(Card::Suit suit)
{
    switch (suit) {
        case Card::Hearts:
            return 0;
        case Card::Spades:
            return 1;
        case Card::Diamonds:
            return 2;
        case Card::Clubs:
            return 3;
    }
    return 0;
}

static const char * freecell_solver_cmd_line_args[280] =
{
"--method", "soft-dfs", "-to", "0123456789", "-step",
"500", "--st-name", "1", "-nst", "--method",
"soft-dfs", "-to", "0123467", "-step", "500",
"--st-name", "2", "-nst", "--method", "random-dfs",
"-seed", "2", "-to", "0[01][23456789]", "-step",
"500", "--st-name", "3", "-nst", "--method",
"random-dfs", "-seed", "1", "-to", "0[0123456789]",
"-step", "500", "--st-name", "4", "-nst", "--method",
"random-dfs", "-seed", "3", "-to", "0[01][23467]",
"-step", "500", "--st-name", "5", "-nst", "--method",
"random-dfs", "-seed", "4", "-to", "0[0123467]",
"-step", "500", "--st-name", "9", "-nst", "--method",
"random-dfs", "-to", "[01][23456789]", "-seed", "8",
"-step", "500", "--st-name", "10", "-nst",
"--method", "random-dfs", "-to", "[01][23456789]",
"-seed", "268", "-step", "500", "--st-name", "12",
"-nst", "--method", "a-star", "-asw",
"0.2,0.3,0.5,0,0", "-step", "500", "--st-name", "16",
"-nst", "--method", "a-star", "-to", "0123467",
"-asw", "0.5,0,0.3,0,0", "-step", "500", "--st-name",
"18", "-nst", "--method", "soft-dfs", "-to",
"0126394875", "-step", "500", "--st-name", "19",
"--prelude",
"350@2,350@5,350@9,350@12,350@2,350@10,350@3,350@9,350@5,350@18,350@2,350@5,350@4,350@10,350@4,350@12,1050@9,700@18,350@10,350@5,350@2,350@10,1050@16,350@2,700@4,350@10,1050@2,1400@3,350@18,1750@5,350@16,350@18,700@4,1050@12,2450@5,1400@18,1050@2,1400@10,6300@1,4900@12,8050@18",
"-ni", "--method", "soft-dfs", "-to", "01ABCDE",
"-step", "500", "--st-name", "0", "-nst", "--method",
"random-dfs", "-to", "[01][ABCDE]", "-seed", "1",
"-step", "500", "--st-name", "1", "-nst", "--method",
"random-dfs", "-to", "[01][ABCDE]", "-seed", "2",
"-step", "500", "--st-name", "2", "-nst", "--method",
"random-dfs", "-to", "[01][ABCDE]", "-seed", "3",
"-step", "500", "--st-name", "3", "-nst", "--method",
"random-dfs", "-to", "01[ABCDE]", "-seed", "268",
"-step", "500", "--st-name", "4", "-nst", "--method",
"a-star", "-to", "01ABCDE", "-step", "500",
"--st-name", "5", "-nst", "--method", "a-star",
"-to", "01ABCDE", "-asw", "0.2,0.3,0.5,0,0", "-step",
"500", "--st-name", "6", "-nst", "--method",
"a-star", "-to", "01ABCDE", "-asw", "0.5,0,0.5,0,0",
"-step", "500", "--st-name", "7", "-nst", "--method",
"random-dfs", "-to", "[01][ABD][CE]", "-seed", "1900",
"-step", "500", "--st-name", "8", "-nst", "--method",
"random-dfs", "-to", "[01][ABCDE]", "-seed", "192",
"-step", "500", "--st-name", "9", "-nst", "--method",
"random-dfs", "-to", "[01ABCDE]", "-seed", "1977",
"-step", "500", "--st-name", "10", "-nst",
"--method", "random-dfs", "-to", "[01ABCDE]", "-seed",
"24", "-step", "500", "--st-name", "11", "-nst",
"--method", "soft-dfs", "-to", "01ABDCE", "-step",
"500", "--st-name", "12", "-nst", "--method",
"soft-dfs", "-to", "ABC01DE", "-step", "500",
"--st-name", "13", "-nst", "--method", "soft-dfs",
"-to", "01EABCD", "-step", "500", "--st-name", "14",
"-nst", "--method", "soft-dfs", "-to", "01BDAEC",
"-step", "500", "--st-name", "15", "--prelude",
"1000@0,1000@3,1000@0,1000@9,1000@4,1000@9,1000@3,1000@4,2000@2,1000@0,2000@1,1000@14,2000@11,1000@14,1000@3,1000@11,1000@2,1000@0,2000@4,2000@10,1000@0,1000@2,2000@10,1000@0,2000@11,2000@1,1000@10,1000@2,1000@10,2000@0,1000@9,1000@1,1000@2,1000@14,3000@8,1000@2,1000@14,1000@1,1000@10,3000@6,2000@4,1000@2,2000@0,1000@2,1000@11,2000@6,1000@0,5000@1,1000@0,2000@1,1000@2,3000@3,1000@10,1000@14,2000@6,1000@0,1000@2,2000@11,6000@8,8000@9,3000@1,2000@10,2000@14,3000@15,4000@0,1000@8,1000@10,1000@14,7000@0,14000@2,6000@3,7000@4,1000@8,4000@9,2000@15,2000@6,4000@3,2000@4,3000@15,2000@0,6000@1,2000@4,4000@6,4000@9,4000@14,7000@8,3000@0,3000@1,5000@2,3000@3,4000@9,8000@10,9000@3,5000@8,7000@11,11000@12,12000@0,8000@3,11000@9,9000@15,7000@2,12000@8,16000@5,8000@13,18000@0,9000@15,12000@10,16000@0,14000@3,16000@9,26000@4,23000@3,42000@6,22000@8,27000@10,38000@7,41000@0,42000@3,84000@13,61000@15,159000@5,90000@9"
};

void FreecellBase::findSolution()
{
    kdDebug(11111) << "findSolution\n";

    QString output = solverFormat();
    kdDebug(11111) << output << endl;

    int ret;

    /* If solver_instance was not initialized yet - initialize it */
    if (! solver_instance)
    {
        solver_instance = freecell_solver_user_alloc();

        char * error_string;
        int error_arg;
        char * known_parameters[1] = {NULL};


        ret = freecell_solver_user_cmd_line_parse_args(
            solver_instance,
            sizeof(freecell_solver_cmd_line_args)/sizeof(freecell_solver_cmd_line_args[0]),
            freecell_solver_cmd_line_args,
            0,
            known_parameters,
            NULL,
            NULL,
            &error_string,
            &error_arg
            );


        assert(!ret);
    }
    /*
     * I'm using the more standard interface instead of the depracated
     * user_set_game one. I'd like that each function will have its
     * own dedicated purpose.
     *
     *     Shlomi Fish
     * */
#if 0
    ret = freecell_solver_user_set_game(solver_instance,
                                            freecell.count(),
                                            store.count(),
                                            deck->decksNum(),
                                            FCS_SEQ_BUILT_BY_ALTERNATE_COLOR,
                                            unlimited_move,
                                            es_filling);
    assert(!ret);
#else
    freecell_solver_user_set_num_freecells(solver_instance,freecell.count());
    freecell_solver_user_set_num_stacks(solver_instance,store.count());
    freecell_solver_user_set_num_decks(solver_instance,deck->decksNum());
    freecell_solver_user_set_sequences_are_built_by_type(solver_instance, FCS_SEQ_BUILT_BY_ALTERNATE_COLOR);
    freecell_solver_user_set_sequence_move(solver_instance, unlimited_move);
    freecell_solver_user_set_empty_stacks_filled_by(solver_instance, es_filling);

#endif

    freecell_solver_user_limit_iterations(solver_instance, CHUNKSIZE);

    solver_ret = freecell_solver_user_solve_board(solver_instance,
                                                  output.latin1());
    resumeSolution();
}

void FreecellBase::resumeSolution()
{
    if (!solver_instance)
        return;

    emit gameInfo(i18n("%1 tries - depth %2")
                  .arg(freecell_solver_user_get_num_times(solver_instance))
                  .arg(freecell_solver_user_get_current_depth(solver_instance)));

    if (solver_ret == FCS_STATE_WAS_SOLVED)
    {
        emit gameInfo(i18n("solved after %1 tries").
                      arg(freecell_solver_user_get_num_times(
                          solver_instance)));
        kdDebug(11111) << "solved\n";
        Dealer::demo();
        return;
    }
    if (solver_ret == FCS_STATE_IS_NOT_SOLVEABLE) {
	int moves = freecell_solver_user_get_num_times(solver_instance);
        freeSolution();
        emit gameInfo(i18n("unsolved after %1 moves")
                      .arg(moves));
        stopDemo();
        return;
    }

    unsigned int max_iters = freecell_solver_user_get_limit_iterations(
        solver_instance) + CHUNKSIZE;
    freecell_solver_user_limit_iterations(solver_instance,
                                          max_iters);

    if (max_iters > 120000) {
        solver_ret = FCS_STATE_IS_NOT_SOLVEABLE;
        resumeSolution();
        return;
    }

    solver_ret = freecell_solver_user_resume_solution(solver_instance);
    QTimer::singleShot(0, this, SLOT(resumeSolution()));

}
MoveHint *FreecellBase::translateMove(void *m) {
    fcs_move_t move = *(static_cast<fcs_move_t *>(m));
    uint cards = fcs_move_get_num_cards_in_seq(move);
    Pile *from = 0;
    Pile *to = 0;

    switch(fcs_move_get_type(move))
    {
        case FCS_MOVE_TYPE_STACK_TO_STACK:
            from = store[fcs_move_get_src_stack(move)];
            to = store[fcs_move_get_dest_stack(move)];
            break;

        case FCS_MOVE_TYPE_FREECELL_TO_STACK:
            from = freecell[fcs_move_get_src_freecell(move)];
            to = store[fcs_move_get_dest_stack(move)];
            cards = 1;
            break;

        case FCS_MOVE_TYPE_FREECELL_TO_FREECELL:
            from = freecell[fcs_move_get_src_freecell(move)];
            to = freecell[fcs_move_get_dest_freecell(move)];
            cards = 1;
            break;

        case FCS_MOVE_TYPE_STACK_TO_FREECELL:
            from = store[fcs_move_get_src_stack(move)];
            to = freecell[fcs_move_get_dest_freecell(move)];
            cards = 1;
            break;

        case FCS_MOVE_TYPE_STACK_TO_FOUNDATION:
            from = store[fcs_move_get_src_stack(move)];
            cards = 1;
            to = 0;
            break;

        case FCS_MOVE_TYPE_FREECELL_TO_FOUNDATION:
            from = freecell[fcs_move_get_src_freecell(move)];
            cards = 1;
            to = 0;
    }
    assert(from);
    assert(cards <= from->cards().count());
    assert(to || cards == 1);
    Card *c = from->cards()[from->cards().count() - cards];

    if (!to)
        to = findTarget(c);
    assert(to);
    return new MoveHint(c, to);
}

QString FreecellBase::solverFormat() const
{
    QString output;
    QString tmp;
    for (uint i = 0; i < target.count(); i++) {
        if (target[i]->isEmpty())
            continue;
        tmp += suitToString(target[i]->top()->suit()) + "-" + rankToString(target[i]->top()->rank()) + " ";
    }
    if (!tmp.isEmpty())
        output += QString::fromLatin1("Foundations: %1\n").arg(tmp);

    tmp.truncate(0);
    for (uint i = 0; i < freecell.count(); i++) {
        if (freecell[i]->isEmpty())
            tmp += "- ";
        else
            tmp += rankToString(freecell[i]->top()->rank()) + suitToString(freecell[i]->top()->suit()) + " ";
    }
    if (!tmp.isEmpty())
        output += QString::fromLatin1("Freecells: %1\n").arg(tmp);

    for (uint i = 0; i < store.count(); i++)
    {
        CardList cards = store[i]->cards();
        for (CardList::ConstIterator it = cards.begin(); it != cards.end(); ++it)
            output += rankToString((*it)->rank()) + suitToString((*it)->suit()) + " ";
        output += "\n";
    }
    return output;
}

//  Idea stolen from klondike.cpp
//
//  This function returns true when it is certain that the card t is no longer
//  needed on any of the play piles.
//
//  To determine wether a card is no longer needed on any of the play piles we
//  obviously must know what a card can be used for there. According to the
//  rules a card can be used to store another card with 1 less unit of value
//  and opposite color. This is the only thing that a card can be used for
//  there. Therefore the cards with lowest value (1) are useless there (base
//  case). The other cards each have 2 cards that can be stored on them, let us
//  call those 2 cards *depending cards*.
//
//  The object of the game is to put all cards on the target piles. Therefore
//  cards that are no longer needed on any of the play piles should be put on
//  the target piles if possible. Cards on the target piles can not be moved
//  and they can not store any of its depending cards. Let us call this that
//  the cards on the target piles are *out of play*.
//
//  The simple and obvious rule is:
//    A card is no longer needed when both of its depending cards are out of
//    play.
//
//  More complex:
//    Assume card t is red.  Now, if the lowest unplayed black card is
//    t.value()-2, then t may be needed to hold that black t.value()-1 card.
//    If the lowest unplayed black card is t.value()-1, it will be playable
//    to the target, unless it is needed for a red card of value t.value()-2.
//
//  So, t is not needed if the lowest unplayed red card is t.value()-2 and the
//  lowest unplayed black card is t.value()-1, OR if the lowest unplayed black
//  card is t.value().  So, no recursion needed - we did it ahead of time.

bool FreecellBase::noLongerNeeded(const Card & t)
{

    if (t.rank() <= Card::Two) return true; //  Base case.

    bool cardIsRed = t.isRed();

    uint numSame = 0, numDiff = 0;
    Card::Rank lowSame = Card::King, lowDiff = Card::King;
    for (PileList::Iterator it = target.begin(); it != target.end(); ++it)
    {
        if ((*it)->isEmpty())
            continue;
        if ((*it)->top()->isRed() == cardIsRed) {
            numSame++;
            if ((*it)->top()->rank() < lowSame)
                lowSame = static_cast<Card::Rank>((*it)->top()->rank()+1);
        } else {
            numDiff++;
            if ((*it)->top()->rank() < lowDiff)
                lowDiff = static_cast<Card::Rank>((*it)->top()->rank()+1);
        }
    }
    if (numSame < target.count()/2) lowSame = Card::Ace;
    if (numDiff < target.count()/2) lowDiff = Card::Ace;

    return (lowDiff >= t.rank() ||
        (lowDiff >= t.rank()-1 && lowSame >= t.rank()-2));
}

//  This is the getHints() from dealer.cpp with one line changed
//  to use noLongerNeeded() to decide if the card should be
//  dropped or not.
//
//  I would recommend adding a virtual bool noLongerNeeded(const Card &t)
//  to the base class (Dealer) that just returns true, and then calling
//  it like is done here.  That would preserve current functionality
//  but eliminate this code duplication
void FreecellBase::getHints()
{
    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        if (!takeTargetForHints() && (*it)->target())
            continue;

        Pile *store = *it;
        if (store->isEmpty())
            continue;
//        kdDebug(11111) << "trying " << store->top()->name() << endl;

        CardList cards = store->cards();
        while (cards.count() && !cards.first()->realFace()) cards.remove(cards.begin());

        CardList::Iterator iti = cards.begin();
        while (iti != cards.end())
        {
            if (store->legalRemove(*iti)) {
//                kdDebug(11111) << "could remove " << (*iti)->name() << endl;
                for (PileList::Iterator pit = piles.begin(); pit != piles.end(); ++pit)
                {
                    Pile *dest = *pit;
                    if (dest == store)
                        continue;
                    if (store->indexOf(*iti) == 0 && dest->isEmpty() && !dest->target())
                        continue;
                    if (!dest->legalAdd(cards))
                        continue;

                    bool old_prefer = checkPrefering( dest->checkIndex(), dest, cards );
                    if (!takeTargetForHints() && dest->target())
                        newHint(new MoveHint(*iti, dest, noLongerNeeded(*(*iti))));
                    else {
                        store->hideCards(cards);
                        // if it could be here as well, then it's no use
                        if ((store->isEmpty() && !dest->isEmpty()) || !store->legalAdd(cards))
                            newHint(new MoveHint(*iti, dest));
                        else {
                            if (old_prefer && !checkPrefering( store->checkIndex(),
                                                               store, cards ))
                            { // if checkPrefers says so, we add it nonetheless
                                newHint(new MoveHint(*iti, dest));
                            }
                        }
                        store->unhideCards(cards);
                    }
                }
            }
            cards.remove(iti);
            iti = cards.begin();
        }
    }
}

void FreecellBase::demo()
{
    if (solver_instance && solver_ret == FCS_STATE_WAS_SOLVED) {
        Dealer::demo();
        return;
    }
    towait = (Card*)-1;
    unmarkAll();
    kdDebug(11111) << "demo " << (solver_ret != FCS_STATE_IS_NOT_SOLVEABLE) << endl;
    if (solver_ret != FCS_STATE_IS_NOT_SOLVEABLE)
        findSolution();
}

MoveHint *FreecellBase::chooseHint()
{
    if (solver_instance && freecell_solver_user_get_moves_left(solver_instance)) {

        emit gameInfo(i18n("%1 moves before finish").arg(freecell_solver_user_get_moves_left(solver_instance)));

        fcs_move_t move;
        if (!freecell_solver_user_get_next_move(solver_instance, &move)) {
            MoveHint *mh = translateMove(&move);
            oldmoves.append(mh);
            return mh;
        } else
            return 0;
    } else
        return Dealer::chooseHint();
}

void FreecellBase::countFreeCells(int &free_cells, int &free_stores) const
{
    free_cells = 0;
    free_stores = 0;

    for (uint i = 0; i < freecell.count(); i++)
        if (freecell[i]->isEmpty()) free_cells++;
    if (es_filling == FCS_ES_FILLED_BY_ANY_CARD)
        for (uint i = 0; i < store.count(); i++)
            if (store[i]->isEmpty()) free_stores++;
}

void FreecellBase::freeSolution()
{
    for (HintList::Iterator it = oldmoves.begin(); it != oldmoves.end(); ++it)
        delete *it;
    oldmoves.clear();

    if (!solver_instance)
        return;
    freecell_solver_user_recycle(solver_instance);
    solver_ret = FCS_STATE_NOT_BEGAN_YET;
}

void FreecellBase::stopDemo()
{
    Dealer::stopDemo();
    freeSolution();
}

void FreecellBase::moveCards(CardList &c, FreecellPile *from, Pile *to)
{
    if (!demoActive() && solver_instance) {
        freeSolution();
    }

    assert(c.count() > 1);
    if (unlimited_move) {
        from->Pile::moveCards(c, to);
        return;
    }
    setWaiting(true);

    from->moveCardsBack(c);
    waitfor = c.first();
    connect(waitfor, SIGNAL(stoped(Card*)), SLOT(waitForMoving(Card*)));

    PileList fcs;

    for (uint i = 0; i < freecell.count(); i++)
        if (freecell[i]->isEmpty()) fcs.append(freecell[i]);

    PileList fss;

    if (es_filling == FCS_ES_FILLED_BY_ANY_CARD)
        for (uint i = 0; i < store.count(); i++)
            if (store[i]->isEmpty() && to != store[i]) fss.append(store[i]);

    if (fcs.count() == 0) {
        assert(fss.count());
        fcs.append(fss.last());
        fss.remove(fss.fromLast());
    }
    while (moves.count()) { delete moves.first(); moves.remove(moves.begin()); }

    movePileToPile(c, to, fss, fcs, 0, c.count(), 0);

    if (!waitfor->animated())
        QTimer::singleShot(0, this, SLOT(startMoving()));
}

struct MoveAway {
    Pile *firstfree;
    int start;
    int count;
};

void FreecellBase::movePileToPile(CardList &c, Pile *to, PileList fss, PileList &fcs, uint start, uint count, int debug_level)
{
    kdDebug(11111) << debug_level << " movePileToPile" << c.count() << " " << start  << " " << count << endl;
    uint moveaway = 0;
    if (count > fcs.count() + 1) {
        moveaway = (fcs.count() + 1);
        while (moveaway * 2 < count)
            moveaway <<= 1;
    }
    kdDebug(11111) << debug_level << " moveaway " << moveaway << endl;

    QValueList<MoveAway> moves_away;

    if (count - moveaway < (fcs.count() + 1) && (count <= 2 * (fcs.count() + 1))) {
        moveaway = count - (fcs.count() + 1);
    }
    while (count > fcs.count() + 1) {
        assert(fss.count());
        MoveAway ma;
        ma.firstfree = fss[0];
        ma.start = start;
        ma.count = moveaway;
        moves_away.append(ma);
        fss.remove(fss.begin());
        movePileToPile(c, ma.firstfree, fss, fcs, start, moveaway, debug_level + 1);
        start += moveaway;
        count -= moveaway;
        moveaway >>= 1;
        if ((count > (fcs.count() + 1)) && (count <= 2 * (fcs.count() + 1)))
            moveaway = count - (fcs.count() + 1);
    }
    uint moving = QMIN(count, QMIN(c.count() - start, fcs.count() + 1));
    assert(moving);

    for (uint i = 0; i < moving - 1; i++) {
        moves.append(new MoveHint(c[c.count() - i - 1 - start], fcs[i]));
    }
    moves.append(new MoveHint(c[c.count() - start - 1 - (moving - 1)], to));

    for (int i = moving - 2; i >= 0; --i)
        moves.append(new MoveHint(c[c.count() - i - 1 - start], to));

    while (moves_away.count())
    {
        MoveAway ma = moves_away.last();
        moves_away.remove(moves_away.fromLast());
        movePileToPile(c, to, fss, fcs, ma.start, ma.count, debug_level + 1);
        fss.append(ma.firstfree);
    }
}

void FreecellBase::startMoving()
{
    kdDebug(11111) << "startMoving\n";
    if (moves.isEmpty()) {
        if (demoActive() && towait) {
            waitForDemo(towait);
        }
        setWaiting(false);
        takeState();
        return;
    }

    MoveHint *mh = moves.first();
    moves.remove(moves.begin());
    CardList empty;
    empty.append(mh->card());
    assert(mh->card() == mh->card()->source()->top());
    assert(mh->pile()->legalAdd(empty));
    mh->pile()->add(mh->card());
    mh->pile()->moveCardsBack(empty, true);
    waitfor = mh->card();
    kdDebug(11111) << "wait for moving end " << mh->card()->name() << endl;
    connect(mh->card(), SIGNAL(stoped(Card*)), SLOT(waitForMoving(Card*)));
    delete mh;
}

void FreecellBase::newDemoMove(Card *m)
{
    Dealer::newDemoMove(m);
    if (m != m->source()->top())
        m->disconnect();
}

void FreecellBase::waitForMoving(Card *c)
{
    if (waitfor != c)
        return;
    c->disconnect();
    startMoving();
}

bool FreecellBase::cardDblClicked(Card *c)
{
    // target move
    if (Dealer::cardDblClicked(c))
        return true;

    if (c->animated())
        return false;

    if (c == c->source()->top() && c->realFace())
        for (uint i = 0; i < freecell.count(); i++)
            if (freecell[i]->isEmpty()) {
                CardList empty;
                empty.append(c);
                c->source()->moveCards(empty, freecell[i]);
                canvas()->update();
                return true;
            }
    return false;
}

bool FreecellBase::CanPutStore(const Pile *c1, const CardList &c2) const
{
    int fcs, fss;
    countFreeCells(fcs, fss);

    if (c1->isEmpty()) // destination is empty
        fss--;

    if (!unlimited_move && int(c2.count()) > ((fcs)+1)<<fss)
        return false;

    // ok if the target is empty
    if (c1->isEmpty())
        return true;

    Card *c = c2.first(); // we assume there are only valid sequences

    // ok if in sequence, alternate colors
    return ((c1->top()->rank() == (c->rank()+1))
            && (c1->top()->isRed() != c->isRed()));
}

bool FreecellBase::checkAdd(int, const Pile *c1, const CardList &c2) const
{
    return CanPutStore(c1, c2);
}

//-------------------------------------------------------------------------//

bool FreecellBase::checkRemove(int checkIndex, const Pile *p, const Card *c) const
{
    if (checkIndex != 0)
        return false;

    // ok if just one card
    if (c == p->top())
        return true;

    // Now we're trying to move two or more cards.

    // First, let's check if the column is in valid
    // (that is, in sequence, alternated colors).
    int index = p->indexOf(c) + 1;
    const Card *before = c;
    while (true)
    {
        c = p->at(index++);

        if (!((c->rank() == (before->rank()-1))
              && (c->isRed() != before->isRed())))
        {
            return false;
        }
        if (c == p->top())
            return true;
        before = c;
    }

    return true;
}

//-------------------------------------------------------------------------//

class Freecell : public FreecellBase
{
public:
    Freecell( KMainWindow* parent=0, const char* name=0);
    virtual void deal();
};

Freecell::Freecell( KMainWindow* parent, const char* name)
    : FreecellBase(1, 8, 4, FCS_ES_FILLED_BY_ANY_CARD, false, parent, name)
{
    for (int i = 0; i < 8; i++)
        store[i]->move(8 + ( cardMap::CARDX() * 11 / 10 + 1 ) * i, 8 + cardMap::CARDY() * 11 / 10);

    const int right = 8 + ( cardMap::CARDX() * 11 / 10 + 1 ) * 7 + cardMap::CARDX();

    for (int i = 0; i < 4; i++)
        freecell[i]->move(8 + ( cardMap::CARDX() * 13 / 12 ) * i, 8);

    for (int i = 0; i < 4; i++)
        target[i]->move(right - (3-i) * ( cardMap::CARDX() * 13 / 12 ) -cardMap::CARDX()  , 8);
}

void Freecell::deal()
{
    int column = 0;
    while (!deck->isEmpty())
    {
        store[column]->add (deck->nextCard(), false, true);
        column = (column + 1) % 8;
    }
}

static class LocalDealerInfo3 : public DealerInfo
{
public:
    LocalDealerInfo3() : DealerInfo(I18N_NOOP("&Freecell"), 3) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Freecell(parent); }
} ldi8;

//-------------------------------------------------------------------------//

#include"freecell.moc"
