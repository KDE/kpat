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

#include <qdialog.h>
#include "freecell.h"
#include <klocale.h>
#include "deck.h"
#include "pile.h"
#include <assert.h>
#include <kdebug.h>
#include <stdio.h>
#include <ktempfile.h>
#include <kprocess.h>
#include <qtextstream.h>
#include <qfile.h>
#include <stdlib.h>
#include <qtimer.h>

#include <freecell-solver/fcs.h>
#include <freecell-solver/preset.h>

const int CHUNKSIZE = 100;

void FreecellPile::moveCards(CardList &c, Pile *to)
{
    if (c.count() == 1) {
        Pile::moveCards(c, to);
        return;
    }
    dynamic_cast<FreecellBase*>(dealer())->moveCards(c, this, to);
}

//-------------------------------------------------------------------------//

FreecellBase::FreecellBase( int decks, int stores, int freecells, int fill,
                            KMainWindow* parent, const char* name)
    : Dealer(parent,name),
solver_instance(0), solver_state(0), es_filling(fill), solver_ret(FCS_STATE_NOT_BEGAN_YET)
{
    deck = new Deck(0, this, decks);
    deck->hide();

    kdDebug() << "cards " << deck->cards().count() << endl;
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
        t->setRemoveFlags(Pile::Default);
    }

    setActions(Dealer::Demo | Dealer::Hint);
}

//-------------------------------------------------------------------------//

void FreecellBase::restart()
{
    freeSolution();
    deck->collectAndShuffle();
    deal();
}

QString suitToString(Card::Suits v) {
    switch (v) {
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

QString valueToString(Card::Values v)
{
    switch (v) {
        case Card::King:
            return "K";
        case Card::Ace:
            return "A";
        case Card::Jack:
            return "J";
        case Card::Queen:
            return "Q";
        default:
            return QString::number(v);
    }
}

int getDeck(Card::Suits suit)
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

fcs_card_t getCard(Card *c)
{
    if (!c)
        return fcs_empty_card;

    fcs_card_t card = 0;
    fcs_card_set_num(card, static_cast<int>(c->value()));
    fcs_card_set_deck(card, getDeck(c->suit()));
    return card;
}

void FreecellBase::findSolution()
{
    kdDebug() << "findSolution\n";

    QString output = solverFormat();
    fprintf(stderr, "%s\n", output.latin1());

    freecell_solver_instance_t *instance = 0;
    instance = freecell_solver_alloc_instance();
    freecell_solver_init_instance(instance);
    instance->freecells_num = freecell.count();
    instance->stacks_num = store.count();
    instance->decks_num = deck->decksNum();
    instance->sequences_are_built_by = FCS_SEQ_BUILT_BY_ALTERNATE_COLOR;
    instance->unlimited_sequence_move = 0;
    instance->empty_stacks_fill = es_filling;

    instance->max_num_times = CHUNKSIZE;
    instance->max_depth = 250;
    instance->solution_moves = 0;

    fcs_state_with_locations_t *state = new fcs_state_with_locations_t;
    fcs_state_init(state, instance->stacks_num);
    for(int c=0;c<instance->freecells_num;c++) {
        fcs_empty_freecell(state->s, c);
        fcs_put_card_in_freecell(state->s, c, getCard(freecell[c]->top()));
    }
    for(int d=0; d < 4*instance->decks_num ;d++)
    {
        fcs_set_deck(state->s, d, 0);
    }
    int deck_index[4] = {0, 0, 0, 0};

    for (uint d = 0; d < target.count(); d++)
    {
        Card *c = target[d]->top();
        if (c) {
            fcs_set_deck(state->s, deck_index[getDeck(c->suit())]*4 + getDeck(c->suit()), static_cast<int>(c->value()));
            deck_index[getDeck(c->suit())]++;
        }
    }
    for (int s = 0; s < instance->stacks_num; s++) {
        CardList cards = store[s]->cards();
        for (CardList::ConstIterator it = cards.begin(); it != cards.end(); ++it)
        {
            fcs_push_card_into_stack(state->s, s, getCard(*it));
        }
    }

    fcs_card_t card;
    assert(fcs_check_state_validity(state, instance->freecells_num, instance->stacks_num, instance->decks_num, &card) == 0);

    fcs_canonize_state(state, instance->freecells_num, instance->stacks_num);
    solver_ret = freecell_solver_solve_instance(instance, state);
    solver_instance = instance;
    solver_state = state;
    resumeSolution();
}

void FreecellBase::resumeSolution()
{
    freecell_solver_instance_t *instance = static_cast<freecell_solver_instance_t *>(solver_instance);
    fcs_state_with_locations_t *state = static_cast<fcs_state_with_locations_t *>(solver_state);

    if (!instance)
        return;

    emit gameInfo(i18n("%1 tries - depth %2").arg(instance->max_num_times).arg(instance->num_solution_states));

    kdDebug() << "resumeSolution " << instance->max_num_times
              << " " << solver_ret << endl;

    if (solver_ret == FCS_STATE_WAS_SOLVED)
    {
        int a;

        for(a=0;a<instance->num_solution_states;a++)
        {
            free((void*)instance->solution_states[a]);
        }
        free((void*)instance->solution_states);

        fcs_move_stack_normalize(
            instance->solution_moves,
            state,
            instance->freecells_num,
            instance->stacks_num,
            instance->decks_num
            );
        emit gameInfo(i18n("solved after %1 tries").arg(instance->num_times));
        kdDebug() << "solved\n";
        Dealer::demo();
        return;
    }
    if (solver_ret == FCS_STATE_IS_NOT_SOLVEABLE) {
        freeSolution();
        emit gameInfo(i18n("unsolved after %1 moves").arg(instance->num_times));
        stopDemo();
        return;
    }

    instance->max_num_times += CHUNKSIZE;

    solver_ret = freecell_solver_resume_instance(instance);

    kdDebug() << "resumeSolution2 " << instance->max_num_times
              << " " << solver_ret << endl;

    QTimer::singleShot(0, this, SLOT(resumeSolution()));

}
MoveHint *FreecellBase::translateMove(void *m) {
    fcs_move_t *move = static_cast<fcs_move_t *>(m);
    uint cards = move->num_cards_in_sequence;
    Pile *from = 0;
    Pile *to = 0;

    switch(move->type)
    {
        case FCS_MOVE_TYPE_STACK_TO_STACK:
            from = store[move->src_stack];
            to = store[move->dest_stack];
            break;

        case FCS_MOVE_TYPE_FREECELL_TO_STACK:
            from = freecell[move->src_freecell];
            to = store[move->dest_stack];
            cards = 1;
            break;

        case FCS_MOVE_TYPE_FREECELL_TO_FREECELL:
            from = freecell[move->src_freecell];
            to = freecell[move->dest_freecell];
            cards = 1;
            break;

        case FCS_MOVE_TYPE_STACK_TO_FREECELL:
            from = store[move->src_stack];
            to = freecell[move->dest_freecell];
            cards = 1;
            break;

        case FCS_MOVE_TYPE_STACK_TO_FOUNDATION:
            from = store[move->src_stack];
            cards = 1;
            to = 0;
            break;

        case FCS_MOVE_TYPE_FREECELL_TO_FOUNDATION:
            from = freecell[move->src_freecell];
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
        tmp += suitToString(target[i]->top()->suit()) + "-" + valueToString(target[i]->top()->value()) + " ";
    }
    if (!tmp.isEmpty())
        output += QString::fromLatin1("Foundations: %1\n").arg(tmp);

    tmp.truncate(0);
    for (uint i = 0; i < freecell.count(); i++) {
        if (freecell[i]->isEmpty())
            tmp += "- ";
        else
            tmp += valueToString(freecell[i]->top()->value()) + suitToString(freecell[i]->top()->suit()) + " ";
    }
    if (!tmp.isEmpty())
        output += QString::fromLatin1("Freecells: %1\n").arg(tmp);

    for (uint i = 0; i < store.count(); i++)
    {
        CardList cards = store[i]->cards();
        for (CardList::ConstIterator it = cards.begin(); it != cards.end(); ++it)
            output += valueToString((*it)->value()) + suitToString((*it)->suit()) + " ";
        output += "\n";
    }
    return output;
}

void FreecellBase::getHints()
{
    Dealer::getHints();
}

void FreecellBase::demo()
{
    if (solver_instance && solver_ret == FCS_STATE_WAS_SOLVED) {
        Dealer::demo();
        return;
    }
    towait = (Card*)-1;
    unmarkAll();
    kdDebug() << "demo\n";
    if (!solver_instance && solver_ret != FCS_STATE_IS_NOT_SOLVEABLE)
        findSolution();
}

MoveHint *FreecellBase::chooseHint()
{
    freecell_solver_instance_t *instance = static_cast<freecell_solver_instance_t *>(solver_instance);
    kdDebug() << "choosing " << instance << " " << (instance ? instance->solution_moves->num_moves : 0) << " " << solver_ret  << endl;

    fcs_move_t move;
    if (instance && instance->solution_moves->num_moves > 0) {
        emit gameInfo(i18n("%1 moves before finish").arg(instance->solution_moves->num_moves));
        move = instance->solution_moves->moves[--instance->solution_moves->num_moves];
        kdDebug() << "move " << fcs_move_to_string(move) << endl;
        return translateMove(&move); // TODO: memory leak because of abuse!!!
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
    solver_ret = FCS_STATE_NOT_BEGAN_YET;
    freecell_solver_instance_t *instance = static_cast<freecell_solver_instance_t *>(solver_instance);
    if (!instance)
        return;
    if (instance->solution_moves)
        fcs_move_stack_destroy(instance->solution_moves);
    freecell_solver_finish_instance(instance);
    freecell_solver_free_instance(instance);
    solver_instance = 0;
}

void FreecellBase::stopDemo()
{
    Dealer::stopDemo();
    solver_ret = FCS_STATE_IS_NOT_SOLVEABLE;
    freeSolution();
}

void FreecellBase::moveCards(CardList &c, FreecellPile *from, Pile *to)
{
    if (!demoActive() && solver_instance) {
        freeSolution();
    }

    assert(c.count() > 1);
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
    kdDebug() << debug_level << " movePileToPile" << c.count() << " " << start  << " " << count << endl;
    uint moveaway = 0;
    if (count > fcs.count() + 1) {
        moveaway = (fcs.count() + 1);
        while (moveaway * 2 < count)
            moveaway <<= 1;
    }
    kdDebug() << debug_level << " moveaway " << moveaway << endl;

    QValueList<MoveAway> moves_away;

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
    kdDebug() << "startMoving\n";
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
    kdDebug() << "wait for moving end " << mh->card()->name() << endl;
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

bool FreecellBase::CanPutStore(const Pile *c1, const CardList &c2) const
{
    int fcs, fss;
    countFreeCells(fcs, fss);

    if (c1->isEmpty()) // destination is empty
        fss--;

    if (int(c2.count()) > ((fcs)+1)<<fss)
        return false;

    // ok if the target is empty
    if (c1->isEmpty())
        return true;

    Card *c = c2.first(); // we assume there are only valid sequences

    // ok if in sequence, alternate colors
    return ((c1->top()->value() == (c->value()+1))
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

        if (!((c->value() == (before->value()-1))
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
    : FreecellBase(1, 8, 4, FCS_ES_FILLED_BY_ANY_CARD, parent, name)
{
    for (int i = 0; i < 8; i++)
        store[i]->move(8+80*i, 113);

    for (int i = 0; i < 4; i++)
        freecell[i]->move(8+76*i, 8);

    for (int i = 0; i < 4; i++)
        target[i]->move(338+76*i, 8);
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

//-------------------------------------------------------------------------//

#if 0

Pile *FreecellBase::pileForName(QString line) const
{
    if (line.left(5) == "stack") {
        line = line.mid(6);
        bool ok;
        int stack = line.left(line.find(' ')).toInt(&ok);
        assert(ok);
        assert(stack >= 0 && stack <= 7);
        return store[stack];
    }
    if (line.left(8) == "freecell") {
        line = line.mid(9);
        bool ok;
        int stack = line.left(line.find(' ')).toInt(&ok);
        assert(ok);
        assert(stack >= 0 && stack <= 3);
        return freecell[stack];
    }
    return 0;
}

    KTempFile outf;
    outf.setAutoDelete(true);
    KTempFile inf;
    // inf.setAutoDelete(true);
    KShellProcess proc;
    outf.close();
    // << "-to" << "016543297"
    proc << "fc-solve" << "-m"  << inf.name() << ">" << outf.name() << "2>&1";
    QString output = solverFormat();

    fprintf(inf.fstream(), output.utf8().data());
    inf.close();
    if (!proc.start(KProcess::Block))
        kdError() << "can't run fc-solve\n";
    kdDebug() << "exit " << proc.exitStatus() << endl;

    QFile f(outf.name());
    f.open(IO_ReadOnly);
    QTextStream is(&f);
    QString line;
    while (!is.eof()) {
        line = is.readLine();
        printf("%s\n", line.latin1());
        if (foundhint)
            continue;

        if (line.left(17) == "I could not solve") {
            Dealer::getHints();
            return;
        }
        if (line.left(5) == "Move ") {
            // "Move a card from stack 6 to the foundations"
            line = line.mid(5);
            QString num = line.left(line.find(' '));
            kdDebug() << "num \"" << num << "\"\n";
            uint cards = 0;
            if (num == QString::fromLatin1("a"))
                cards = 1;
            else
                cards = num.toUInt();
            kdDebug() << "cards " << cards << endl;
            line = line.mid(line.find("from ") + 5);

            Pile *from = pileForName(line);
            if (cards > from->cards().count()) {
                Dealer::getHints();
                return;
            }

            assert(cards <= from->cards().count());
            Card *c = from->cards()[from->cards().count() - cards];

            line = line.mid(line.find("to ") + 3);
            kdDebug() << "should move " << c->name() << " to " << line << endl;
            Pile *to = pileForName(line);
            if (!to)
                to = findTarget(c);
            assert(to);
            newHint(new MoveHint(c, to));
            foundhint = true;
        }
    }
#endif
