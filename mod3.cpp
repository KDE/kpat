/*---------------------------------------------------------------------------

  mod3.cpp  implements a patience card game

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
#include <kapp.h>
#include "mod3.h"
#include <kmsgbox.h>
#include "global.h"

//-------------------------------------------------------------------------//

void Mod3::restart()
{
	deck->collectAndShuffle();

	deal();
}

//-------------------------------------------------------------------------//

void Mod3::undo()
{
	Card::undoLastMove();
}

//-------------------------------------------------------------------------//

void Mod3::show()
{
	for (int r = 0; r < 4; r++)
		for (int c = 0; c < 8; c++)
			stack[r][c]->show();

	rb.show();
}

//-------------------------------------------------------------------------//

Mod3::Mod3( QWidget* parent, const char* name)
	: dealer(parent,name), rb(i18n("Redeal"), this) 
{
	initMetaObject();

	deck = new Deck (-666, -666, this, 2);

	for (int r = 0; r < 4; r++)
	{
		for (int i = 1; i <= 3; i++)
			Card::setLegalMove (r+1, i);

		if (r < 3)
		{
			Card::setAddFlags (r+1, Card::Default);
			Card::setAddFun (r+1, &CanPut);
		}
		else
			Card::setAddFlags (r+1, Card::addSpread);

		for (int c = 0; c < 8; c++)
			stack[r][c] = new cardPos (
				8+80*c, 8+105*r + 32*(r == 3),
				this, r+1);
	}

	rb.move (8, 322);
	rb.adjustSize();
	connect (&rb, SIGNAL (clicked()) , SLOT (redeal()));

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

Mod3::~Mod3()
{
	delete deck;

	for (int r = 0; r < 4; r++)
		for (int c = 0; c < 8; c++)
			delete stack[r][c];
}

//-------------------------------------------------------------------------//

bool Mod3::CanPut (const Card *c1, const Card *c2)
{
	if (c1 == c2)
		return 0;

	if (c1->Suit() == Card::Empty)
		return (c2->Value() == (c1->type()+1));

	if (c1->Suit() != c2->Suit())
		return 0;

	if (c2->Value() != (c1->Value()+3))
		return 0;

	if (c1->prev()->Suit() == Card::Empty)
		return (c1->Value() == (c1->type()+1));

	return 1;
}

//-------------------------------------------------------------------------//

void Mod3::redeal()
{
	if (!deck->next())
	{
		KMsgBox::message (this, i18n ("Information"), 
			i18n ("No more cards"));
		return;
	}

	for (int c = 0; c < 8; c++)
	{
		Card *card;

		do
			card = deck->getCard();
		while ((card->Value() == Card::Ace) && deck->next());

		stack[3][c]->add (card, FALSE, TRUE);
	}
}

//-------------------------------------------------------------------------//

void Mod3::deal()
{
	for (int r = 0; r < 4; r++)
		for (int c = 0; c < 8; c++)
		{
			Card *card;

			do
				card = deck->getCard();
			while (card->Value() == Card::Ace);

			stack[r][c]->add (card, FALSE, TRUE);
		}
}

//-------------------------------------------------------------------------//

QSize Mod3::sizeHint() const
{
	return QSize (650, 550);
}

//-------------------------------------------------------------------------//

#include"mod3.moc"

//-------------------------------------------------------------------------//

