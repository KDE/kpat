/*---------------------------------------------------------------------------

  freecell.cpp  implements a patience card game

     Copyright (C) 1997  Rodolfo Borges

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
#include <kmsgbox.h>
#include "global.h"

#define STACK 1
#define FREECELL 2
#define STORE 3

bool CanPut (const Card *c1, const Card *c2);
bool CanRemove (const Card *c2);
Freecell *freecell_game;
int dont_put_on_free_stack = 0;

//-------------------------------------------------------------------------//

void Freecell::restart()
{
	deck->collectAndShuffle();

	deal();
}

//-------------------------------------------------------------------------//

void Freecell::undo()
{
	Card::undoLastMove();
}

//-------------------------------------------------------------------------//

void Freecell::show()
{
	int i;

	for (i = 0; i < 8; i++)
		stack[i]->show();

	for (i = 0; i < 4; i++)
	{
		freecell[i]->show();
		store[i]->show();
	}
}

//-------------------------------------------------------------------------//

Freecell::Freecell( QWidget* parent, const char* name)
	: dealer(parent,name)
{
	initMetaObject();

	freecell_game = this;

	deck = new Deck (-666, -666, this);

	Card::setLegalMove (STACK, STORE);
	Card::setLegalMove (FREECELL, STORE);
	Card::setLegalMove (STACK, STACK);
	Card::setLegalMove (FREECELL, STACK);
	Card::setLegalMove (STACK, FREECELL);
	Card::setLegalMove (FREECELL, FREECELL);

	Card::setAddFlags (STACK, Card::addSpread | Card::several);
	Card::setRemoveFlags (STACK, Card::several);
	Card::setAddFlags (FREECELL, Card::Default);
	Card::setAddFlags (STORE, Card::Default);
	Card::setRemoveFlags (STORE, Card::disallow);

	Card::setAddFun (STACK, &::CanPut);
	Card::setAddFun (FREECELL, &::CanPut);
	Card::setAddFun (STORE, &::CanPut);
	Card::setRemoveFun (STACK, &::CanRemove);

	for (int r = 0; r < 4; r++)
	{
		int i;

		for (i = 0; i < 8; i++)
			stack[i] = new cardPos (8+80*i, 113, this, STACK);

		for (i = 0; i < 4; i++)
		{
			freecell[i] = new cardPos (8+76*i, 8, this, FREECELL);
			store[i] = new cardPos (338+76*i, 8, this, STORE);
		}
	}

/*
  QPushButton* hb= new QPushButton(i18n("Hint"),this);
  hb->move(10,380);
  hb->adjustSize();
  connect( hb, SIGNAL( clicked()) , SLOT( hint() ) );
  hb->show();
*/

	deal();
}

//-------------------------------------------------------------------------//

Freecell::~Freecell()
{
	delete deck;

	int i;

	for (i = 0; i < 8; i++)
		delete stack[i];

	for (i = 0; i < 4; i++)
	{
		delete freecell[i];
		delete store[i];
	}
}

//-------------------------------------------------------------------------//

int Freecell::CountCards (const Card *c)
{
	int n = 0;

	while (c->next())
	{
		n++;
		c = c->next();
	}

	return n;
}

//-------------------------------------------------------------------------//

int Freecell::CountFreeCells()
{
	int i, n = 0;

	for (i = 0; i < 8; i++)
		if (!stack[i]->next())
			n++;

	for (i = 0; i < 4; i++)
		if (!freecell[i]->next())
			n++;

	return n;
}

//-------------------------------------------------------------------------//

//bool Freecell::CanPut (const Card *c1, const Card *c2)
bool CanPut (const Card *c1, const Card *c2)
{
	if (c1 == c2)
		return 0;

	switch (c1->type())
	{
	case STORE:
		// only aces in empty spaces
		if (c1->Suit() == Card::Empty)
			return (c2->Value() == Card::Ace);

		// ok if in sequence, same suit
		return (c1->Suit() == c2->Suit())
			&& ((c1->Value()+1) == c2->Value());
		break;

	case FREECELL:
		// ok if the target is empty
		return (c1->Suit() == Card::Empty);
		break;

	case STACK:
		// ok if the target is empty
		if (c1->Suit() == Card::Empty)
			// unless this flag is set (explained on CanRemove())
			return (!dont_put_on_free_stack);

		// ok if in sequence, alternate colors
		return ((c1->Value() == (c2->Value()+1))
			&& (c1->Red() != c2->Red()));
	}

	return 0;
}

//-------------------------------------------------------------------------//

bool CanRemove (const Card *c)
{
	dont_put_on_free_stack = 0;

	// ok if just one card
	if (!c->next())
		return 1;

	// Now we're trying to move two or more cards.

	// First, let's check if the column is in valid
	// (that is, in sequence, alternated colors).
	for (const Card *t = c; t->next(); t = t->next())
	{
		if (!((t->Value() == (t->next()->Value()+1))
			&& (t->Red() != t->next()->Red())))
		{
			return 0;
		}
	}

	// Now, let's see if there are enough free cells available.
	int numFreeCells = freecell_game->CountFreeCells();
	int numCards = freecell_game->CountCards (c);

	// If the the destination will be a free stack, the number of
	// free cells needs to be greater. (We couldn't count the
	// destination free stack.)
	if (numFreeCells == numCards)
		dont_put_on_free_stack = 1;

	return (numCards <= numFreeCells);
}

//-------------------------------------------------------------------------//

void Freecell::deal()
{
	int column = 0;
	while (deck->next())
	{
		stack[column]->add (deck->getCard(), FALSE, TRUE);
		column = (column + 1) % 8;
	}
}

//-------------------------------------------------------------------------//

QSize Freecell::sizeHint() const
{
	return QSize (650, 450);
}

//-------------------------------------------------------------------------//

#include"freecell.moc"

//-------------------------------------------------------------------------//

