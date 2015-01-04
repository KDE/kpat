/*
 * Copyright (C) 2006-2009 Stephan Kulow <coolo@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fortyeightsolver.h"

#include "../fortyeight.h"

#include <QDebug>

#define NUM_PILE 8
#define NUM_DECK 9

#define PRINT 0

class FortyeightSolverState {
public:
  bool empty[8];
  bool linedup[8];
  int  highestbreak[8];
  int  firstempty;
  int  freestores;
  int  fromscore[8];
};

void FortyeightSolver::checkState(FortyeightSolverState &d)
{
    d.firstempty = -1;
    d.freestores = 0;

    for ( int w = 0; w < 8; w++ )
      {
	d.empty[w] = (Wlen[w] == 0);
	d.linedup[w] = true;
	if ( !Wlen[w] ) 
	  {
	    d.freestores++;
	    if (d.firstempty < 0) d.firstempty = w;
	  }
	d.highestbreak[w] = 0;
	for (int i = 1; i < Wlen[w]; i++) 
	  {
	    card_t card1 = W[w][Wlen[w]-i-1];
	    card_t card2 = W[w][Wlen[w]-i];
	    if ( SUIT( card1 ) != SUIT( card2 ) ||
		 RANK( card1 ) != RANK( card2 ) + 1 ) 
	      {
		d.highestbreak[w] = i - 1;
		d.linedup[w] = false;
		break;
	      }
	  }
	d.fromscore[w] = 0;
	for (int i = d.highestbreak[w] + 1; i < Wlen[w]; i++) 
	  {
	    card_t card = W[w][Wlen[w]-i-1];
	    if ( RANK(card) < 5 || RANK(card) == PS_ACE) {
	      d.fromscore[w] += (13 - RANK(card)) * 20;
	    }
	  }
	if (Wlen[w])
	  d.fromscore[w] /= Wlen[w];
      }
}

void FortyeightSolver::make_move(MOVE *m)
{
#if PRINT
    if ( m->totype == O_Type )
      fprintf( stderr, "\nmake move %d from %d to %d out (at %d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    else
        fprintf( stderr, "\nmake move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    print_layout();
#endif

    int from = m->from;
    int to = m->to;

    // move to pile
    if ( from == NUM_DECK && to == NUM_PILE )
    {
        card_t card = *Wp[NUM_DECK];
        Wp[NUM_DECK]--;
        Wlen[NUM_DECK]--;
        card = ( SUIT( card ) << 4 ) + RANK( card );
        Wp[NUM_PILE]++;
        *Wp[NUM_PILE] = card;
        Wlen[NUM_PILE]++;
        hashpile( NUM_DECK );
        hashpile( NUM_PILE );
#if PRINT
        print_layout();
#endif
        return;
    }

    // move to deck
    if ( from == NUM_PILE && to == NUM_DECK )
    {
        while ( Wlen[NUM_PILE] > 1 )
        {
            Wlen[NUM_DECK]++;
            Wp[NUM_DECK]++;
            card_t card = *Wp[NUM_PILE] + ( 1 << 7 );
            *Wp[NUM_DECK] = card;
            Wlen[NUM_PILE]--;
            Wp[NUM_PILE]--;
        }
        hashpile( NUM_DECK );
        hashpile( NUM_PILE );
        lastdeal = true;
#if PRINT
        print_layout();
#endif
        return;
    }

    for ( int i = m->card_index + 1; i > 0; --i )
    {
        card_t card = W[from][Wlen[from]-i];
        Wp[from]--;

	if ( m->totype == O_Type )
	  {
	    O[to]++;
	  }
	else 
	  {
	    Wp[to]++;
	    *Wp[to] = card;
	    Wlen[to]++;
	  }
    }
    Wlen[from] -= m->card_index+1;

    hashpile(from);
    if ( m->totype != O_Type )
      hashpile(to);
#if PRINT
    print_layout();
#endif
}

void FortyeightSolver::undo_move(MOVE *m)
{
#if PRINT
    if ( m->totype == O_Type )
      fprintf( stderr, "\nundo move %d from %d to %d out (at %d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    else
        fprintf( stderr, "\nundo move %d from %d to %d (%d)\n\n", m->card_index, m->from, m->to, m->turn_index );
    print_layout();
#endif

    int from = m->from;
    int to = m->to;

    // move to pile
    if ( from == NUM_DECK && to == NUM_PILE )
    {
        card_t card = *Wp[NUM_PILE];
        Wp[NUM_PILE]--;
        Wlen[NUM_PILE]--;
        card = ( SUIT( card ) << 4 ) + RANK( card ) + ( 1 << 7 );
        Wp[NUM_DECK]++;
        *Wp[NUM_DECK] = card;
        Wlen[NUM_DECK]++;
        hashpile( NUM_DECK );
        hashpile( NUM_PILE );
#if PRINT
        print_layout();
#endif
        return;
    }

    // move to deck
    if ( from == NUM_PILE && to == NUM_DECK )
    {
        while ( Wlen[NUM_DECK] )
        {
            Wlen[NUM_PILE]++;
            Wp[NUM_PILE]++;
            *Wp[NUM_PILE] = ( SUIT( *Wp[NUM_DECK] ) << 4 ) + RANK( *Wp[NUM_DECK] );
            Wlen[NUM_DECK]--;
            Wp[NUM_DECK]--;
        }
        hashpile( NUM_DECK );
        hashpile( NUM_PILE );
        lastdeal = false;
#if PRINT
        print_layout();
#endif
        return;
    }

    for ( int i = m->card_index + 1; i > 0; --i )
    {
      
      card_t card;
      if ( m->totype != O_Type ) 
	{
	  card = W[to][Wlen[to]-i];
	  Wp[to]--;
	} 
      else 
	{
	  card = Osuit[to] + O[to];
	  O[to]--;
	}

      Wp[from]++;
      *Wp[from] = card;
      Wlen[from]++;
    }

    hashpile(from);
    if ( m->totype != O_Type ) {
      Wlen[to] -= m->card_index+1;
      hashpile(to);
    }
#if PRINT
    print_layout();
#endif
}

bool FortyeightSolver::checkMoveOut( int from, MOVE *mp, int *dropped)
{
  if ( !Wlen[from] )
    return false;

  card_t card = *Wp[from];
  int suit = SUIT( card );
  
  int target = suit * 2;

  // aces need to fit on empty
  if ( RANK(card) == PS_ACE )
    {
      if (O[target++] != NONE)
	target--;
      else if ( O[target] != NONE)
	return false;
    }
  else 
    {
      if ( RANK(card) == O[target++] + 1 ) 
	target--;
      else if ( RANK(card) != O[target] + 1 )
	return false;
    }
  
  dropped[target]++;
  mp->card_index = 0;
  mp->from = from;
  mp->to = target;
  mp->totype = O_Type;
  mp->pri = 127;
  mp->turn_index = -1;
  return true;

}

/* Get the possible moves from a position, and store them in Possible[]. */
bool FortyeightSolver::checkMove( int from, int to, MOVE *mp )
{
    if ( !Wlen[from] )
        return false;

    card_t card = *Wp[from];
    Q_ASSERT( to < 8 );

    if ( Wlen[to] )
      {
	card_t top = *Wp[to];
	if ( SUIT( top ) != SUIT( card ) ||
	     RANK( top ) != RANK( card ) + 1 )
	  return false;
      }

    mp->card_index = 0;
    mp->from = from;
    mp->to = to;
    mp->totype = W_Type;
    mp->pri = 70;
    mp->turn_index = -1;
    return true;
}

int FortyeightSolver::get_possible_moves(int *a, int *numout)
{
    int n = 0;
    MOVE *mp = Possible;

    *a = false;

    FortyeightSolverState d;
    checkState(d);

    //print_layout();
    // if a specific target got two possible drops, we don't make it auto drop
    int dropped[8] = { 0, 0, 0, 0, 0, 0, 0, 0};

    // off
    for ( int w = 0; w < 8; w++ )
      {
	if ( checkMoveOut( w, mp, dropped ) )
	  {
	    n++;
	    mp++;
	    break;
	  }
      }
    if ( checkMoveOut( NUM_PILE, mp, dropped ) )
      {
	n++;
	mp++;
      }

    for ( int i = 0; i < 8; i++ )
    {
        if ( dropped[i] > 1 )
        {
            for ( int j = 0; j < n; j++ )
                if ( Possible[j].to == i )
                    Possible[j].pri = qBound( 121, 110 + Wlen[Possible[j].from], 126 );
        }
        if ( dropped[i] == 1 )
        {
            // good automove
            if (n != 1)
            {
                for ( int j = 0; j < n; j++ )
                {
                    if ( Possible[j].to == i )
                    {
		      Possible[0] = Possible[j];
		      Possible[0].pri = 127;
                        break;
                    }
                }
            }
            *a = true;
            *numout = 1;
            return 1;
        }
    }

    //fprintf(stderr, "\n");
    *numout = n;

    for (int w = 0; w < 8; w++)
    {
        for ( int j = 0; j < 8; j++ )
        {
            if ( j == w )
                continue;
            if ( checkMove( w, j, mp ) )
            {
                if ( !Wlen[j] )
                {
		  if (d.linedup[w])
		    continue; // ignore it

		  if ( j != d.firstempty ) // one is enough
		    continue;
		  if ( d.highestbreak[w] >= 1)
		    continue;
		  mp->pri = qMin( 115, 11 + d.fromscore[w]);
                } else {
		  if (d.linedup[w] && Wlen[w] > 1)
		    continue;
		  if (d.linedup[j] && Wlen[j] > 1)
		    mp->pri = 110 + Wlen[j];
		  else {
		    if (d.fromscore[w] < d.fromscore[j])
		      mp->pri = 2;
		    else
		      mp->pri = qMin(115, 20 + d.highestbreak[w] + RANK(*Wp[w]) + d.fromscore[w]);
		  }
		}

                n++;
                mp++;
            }
        }
        if ( checkMove( NUM_PILE, w, mp ) && (!d.empty[w] || w == d.firstempty))
        {
	  if (d.empty[w])
	    mp->pri = 15;
	  else {
	    if (d.linedup[w])
	      mp->pri = 100;
	    else
	      mp->pri = qMax(50 - d.fromscore[w], 0);
	  }
	  n++;
	  mp++;
        }
        if ( Wlen[w] > 1 && d.freestores )
        {
            if ( SUIT( *Wp[w] ) == SUIT( W[w][Wlen[w]-2] ) )
            {
                //print_layout();

                for ( int to = 0; to < 8; to++ )
                {
                    if ( to == w )
                        continue;
                    if ( Wlen[to] && SUIT( *Wp[to] ) != SUIT( *Wp[w] ) )
                        continue;
                    if ( d.empty[to] && to != d.firstempty )
                        continue;
                    int moves = 1 << d.freestores;
                    if ( !Wlen[to] )
                        moves = 1 << ( d.freestores - 1 );

                    if ( moves >= Wlen[w] )
                        moves = Wlen[w];

                    bool switched = false;
                    for ( int i = 2; i < moves; ++i )
                    {
                        /*      printcard( W[w][Wlen[w]-i-1], stderr );
                        printcard( *Wp[w], stderr );
                        fprintf( stderr, " switch? %d\n", i ); */
		      if ( SUIT( W[w][Wlen[w]-i-1] ) != SUIT( *Wp[w] ) ||
			   RANK ( W[w][Wlen[w]-i-1] ) != RANK ( W[w][Wlen[w]-i] ) + 1)
			{
                            switched = true;
                            moves = i;
                            break;
                        }
                    }
                    if ( !Wlen[to] )
		      {
			if ( moves < 2 || moves == Wlen[w] || !switched)
                            continue;
                        //print_layout();
                        mp->card_index = moves-1;
                        mp->from = w;
                        mp->to = to;
                        mp->totype = W_Type;
                        mp->pri = 60;
                        mp->turn_index = -1;
                        n++;
                        mp++;
                        continue;
                    }
		    if (d.linedup[w] && moves < Wlen[w])
		      continue;
                    card_t top = *Wp[to];
                    for ( int i = 2; i <= moves && i <= Wlen[w]; i++ )
                    {
                        card_t cur = W[w][Wlen[w]-i];
                        /* printcard( top, stderr );
                        printcard( cur, stderr );
                        fprintf( stderr, " %d\n", i ); */
                        Q_ASSERT( SUIT( top ) == SUIT( cur ) );
                        if ( RANK( top ) == RANK( cur ) + 1 )
                        {
                            mp->card_index = i-1;
                            mp->from = w;
                            mp->to = to;
                            mp->totype = W_Type;
                            mp->pri = 80;
                            mp->turn_index = -1;
                            n++;
                            mp++;
                        }
                    }
                }
            }
        }
    }
    /* check for deck->pile */
    if ( Wlen[NUM_DECK] ) {
        mp->card_index = 1;
        mp->from = NUM_DECK;
        mp->to = NUM_PILE;
        mp->totype = W_Type;
        mp->pri = 19;
        mp->turn_index = 0;
        n++;
        mp++;
    } else if ( !lastdeal )
    {
        mp->card_index = 1;
        mp->from = NUM_PILE;
        mp->to = NUM_DECK;
        mp->totype = W_Type;
        mp->pri = 19;
        mp->turn_index = 0;
        n++;
        mp++;
    }

    return n;
}

void FortyeightSolver::unpack_cluster( unsigned int k )
{
    O[0] = k & 0xF;
    k >>= 4;
    O[1] = k & 0xF;
    k >>= 4;
    O[2] = k & 0xF;
    k >>= 4;
    O[3] = k & 0xF;
    k >>= 4;
    O[4] = k & 0xF;
    k >>= 4;
    O[5] = k & 0xF;
    k >>= 4;
    O[6] = k & 0xF;
    k >>= 4;
    O[7] = k & 0xF;
}

bool FortyeightSolver::isWon()
{
    for ( int i = 0; i < 8; ++i )
        if ( O[i] != PS_KING )
            return false;
    //qDebug() << "isWon" << getOuts();
    return true;
}

int FortyeightSolver::getOuts()
{
    int total = 0;
    for ( int i = 0; i < 8; ++i )
        total += O[i];
    return total;
}

FortyeightSolver::FortyeightSolver(const Fortyeight *dealer)
    : Solver()
{
    setNumberPiles( 10 );
    deal = dealer;
}

void FortyeightSolver::translate_layout()
{
    /* Read the workspace. */

    int total = 0;
    for ( int w = 0; w < 8; ++w ) {
        int i = translate_pile(deal->stack[w], W[w], 52);
        Wp[w] = &W[w][i - 1];
        Wlen[w] = i;
        total += i;
    }

    /* Output piles, if any. */
    for (int i = 0; i < 8; ++i) {
        O[i] = NONE;
    }
    Osuit[0] = PS_DIAMOND;
    Osuit[1] = PS_DIAMOND;
    Osuit[2] = PS_CLUB;
    Osuit[3] = PS_CLUB;
    Osuit[4] = PS_HEART;
    Osuit[5] = PS_HEART;
    Osuit[6] = PS_SPADE;
    Osuit[7] = PS_SPADE;

    for (int i = 0; i < 8; ++i) {
      KCard *c = deal->target[i]->topCard();
      if (c) {
	int suit = translateSuit( c->suit() ) >> 4;
	if (O[suit * 2] == NONE)
	  O[suit * 2] = c->rank();
	else {
	  if (O[suit * 2] < c->rank()) {
	    O[suit * 2 + 1] = O[suit * 2];
	    O[suit * 2] = c->rank();
	  } else {
	    O[suit * 2 + 1] = c->rank();
	  }
	}
	total += c->rank();
      }
    }
    int i = translate_pile( deal->pile, W[NUM_PILE], 80 );
    Wp[NUM_PILE] = &W[NUM_PILE][i-1];
    Wlen[NUM_PILE] = i;
    total += i;

    i = translate_pile( deal->talon, W[NUM_DECK], 80 );
    Wp[NUM_DECK] = &W[NUM_DECK][i-1];
    Wlen[NUM_DECK] = i;
    total += i;

    lastdeal = deal->lastdeal;

    Q_ASSERT( total == 104 );
}

unsigned int FortyeightSolver::getClusterNumber()
{
    unsigned int i = O[0] + (O[1] << 4);
    unsigned int k = i;
    i = O[2] + (O[3] << 4);
    k |= i << 8;
    i = O[4] + (O[5] << 4);
    k |= i << 16;
    i = O[6] + (O[7] << 4);
    k |= i << 24;
    return k;
}

MoveHint FortyeightSolver::translateMove( const MOVE &m )
{
    if ( m.from == NUM_DECK || m.to == NUM_DECK )
        return MoveHint();
    PatPile *frompile = 0;
    if ( m.from < 8 )
        frompile = deal->stack[m.from];
    else
        frompile = deal->pile;

    Q_ASSERT( frompile );
    KCard *card = frompile->at( frompile->count() - m.card_index - 1);
    Q_ASSERT( card );
    Q_ASSERT( m.to < NUM_PILE );
    if ( m.totype == W_Type)
      return MoveHint( card, deal->stack[m.to], m.pri );

    for (int i = 0; i < 8; i++) {
      KCard *top = deal->target[i]->topCard(); 
      if (top) {
	if (top->suit() == card->suit() && top->rank() + 1 == card->rank())
	  return MoveHint( card, deal->target[i], m.pri );
      } else {
	if (card->rank() == PS_ACE)
	  return MoveHint( card, deal->target[i], m.pri );
      }
    }
    Q_ASSERT( false);
    return MoveHint();
}

void FortyeightSolver::print_layout()
{
    FortyeightSolverState d;
    checkState(d);

    fprintf(stderr, "print-layout-begin\n");
    for (int w = 0; w < 10; w++) {
        if ( w == NUM_DECK )
            fprintf( stderr, "Deck(9): " );
        else if ( w == 8 )
            fprintf( stderr, "Pile(8): " );
        else if ( w < 8 )
            fprintf( stderr, "Play%d: ", w );
        for (int i = 0; i < Wlen[w]; i++) {
            printcard(W[w][i], stderr);
        }
        fputc('\n', stderr);
    }
    fprintf( stderr, "Off: " );
    for (int o = 0; o < 8; ++o) {
      printcard(O[o] + Osuit[o], stderr);
    }
    fprintf( stderr, "\nLast-Deal: %d\n", lastdeal );
#if 1
    fprintf( stderr, "FE: %d FS: %d\n", d.firstempty, d.freestores);
    for (int o = 0; o < 8; ++o) {
      fprintf( stderr, "Pile%d: empty:%d linedup:%d highestbreak:%d fromscore:%d\n", o, d.empty[o], d.linedup[o], d.highestbreak[o], d.fromscore[o]);
    }
#endif
    fprintf(stderr, "print-layout-end\n");
}
