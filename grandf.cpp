/***********************-*-C++-*-********

  grandf.cpp  implements a patience card game

     Copyright (C) 1995  Paul Olav Tvete

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

****************************************/

#include <kapp.h>
#include <qdialog.h>
#include "grandf.h"
#include <kmsgbox.h>
#include "global.h"

class CardBox : public QDialog {
public:
  CardBox( const QPixmap &, QWidget *parent=0 );
};

CardBox::CardBox( const QPixmap &pix, QWidget *parent )
  : QDialog( parent, 0, TRUE )
{
  QLabel *label = new QLabel( i18n("Try to move card:"), this );
  label->setAlignment( AlignCenter );
  QSize s = label->sizeHint();
  label->resize( s.width() + 4, s.height() + 4 );
  int w = label->width();    

  QLabel *p = new QLabel( this, "label" );
  p->setPixmap( pix );
  p->resize( pix.size() );
  w = QMAX( w, p->width() );

  QPushButton *button = new QPushButton( i18n("OK"), this, "button" );
  connect( button, SIGNAL(clicked()), SLOT(accept()) );
  QFont font( "Helvetica", 12 );
  font.setWeight( QFont::Bold );
  button->setFont( font );
  w = QMAX( w, button->width() );

  label->move( QMAX( (w - label->width())/2, 0), 0 );
  p->move( QMAX( (w - p->width())/2, 0), label->height() );
  button->move( QMAX( (w - button->width())/2, 0), 
		p->height() + label->height() );
}

void displayCard( Card* c ) {
  CardBox cb( c->pixmap() ); 
  cb.exec();
}

void Grandf::restart()  {
  deck->collectAndShuffle();

  deal();
  numberOfDeals = 1;
}

void Grandf::hint() {
  bool found = FALSE;

  Card* t[7];
  Card* c=0;
  for(int i=0; i<7;i++) 
    t[i] = store[i]->top();

  for(int i=0; i<7&&!found; i++) {
    c=store[i];
    while (!found && (c=c->next()) ) {
      if (!c->FaceUp()) 
	continue;

      int j=0;
      while (j<7 && !found) {
	found = (i!=j) && Sstep1(t[j],c);
	if ((c->Value() == Card::King) && (c == store[i]->next()))
	    found = FALSE;
	j++;
      }
    }
  }

  if (found) 
    displayCard(c);
}

void Grandf::undo() {
  Card::undoLastMove();
}

void Grandf::show()  {
  for(int i=0; i<4; i++)
    target[i]->show();

  for(int i=0; i<7; i++)
    store[i]->show();

  rb.show();
}

Grandf::Grandf( QWidget* parent, const char* name)
  : dealer(parent,name), rb(i18n("Redeal"), this) 
{
  initMetaObject();

  const int Store = 1;
  const int Target = 2;

  //    Card::setSafeMove();

  Card::setAddFlags(Store, Card::addSpread | Card::several);
  Card::setRemoveFlags(Store, Card::several | Card::autoTurnTop);
  Card::setAddFun(Store, &Sstep1);

  //    Card::setAddFlags(Target, Card::default);
  Card::setRemoveFlags(Target, Card::disallow);
  Card::setAddFun(Target, &step1);

  Card::setLegalMove(Store, Target);
  Card::setLegalMove(Store, Store);

  deck = new Deck (-100, -100, this);

  for(int i=0; i<4; i++)
    target[i] = new cardPos(110+i*100, 10, this, Target);

  for(int i=0; i<7; i++)
    store[i] = new cardPos(10+100*i, 150, this, Store);

  //    resize(1000,600);

  rb.move(10,40);
  rb.adjustSize();
  connect( &rb, SIGNAL( clicked()) , SLOT( redeal() ) );
    
  QPushButton* hb= new QPushButton(i18n("Hint"),this);
  hb->move(10,80);
  hb->adjustSize();
  connect( hb, SIGNAL( clicked()) , SLOT( hint() ) );
  hb->show();

  deal();
  numberOfDeals = 1;
}

Grandf::~Grandf() {
  delete deck;

  for(int i=0; i<4; i++)
    delete target[i];

  for(int i=0; i<7; i++)
    delete store[i];
}

void Grandf::redeal() {
  if (numberOfDeals < 3) {
    collect();
    deal(); 
    numberOfDeals++;
  } else
    KMsgBox::message(this, i18n("Information"), 
		     i18n("Only 3 deals allowed"));
}

void Grandf::deal() {
  int start = 0;
  int stop = 7-1;
  int dir = 1;
  for(int round=0; round < 7; round++) {
    int i = start;
    do {
      if (deck->next())
	store[i]->add(deck->getCard(), i != start, TRUE);
      i += dir;
    } while ( i != stop + dir);
    int t = start;
    start = stop;
    stop = t+dir;
    dir = -dir;
  }

  int i = 0;
  while (deck->next()){
    store[i+1]->add(deck->getCard(), FALSE , TRUE);
    i = (i+1)%6;
  }
}

/*****************************

  Does the collecting step of the game

  NOTE: this is not quite correct -- the piles should be turned
  facedown (ie partially reversed) during collection.

******************************/
void Grandf::collect() {
  Card* p;
  for (int pos = 6; pos >= 0; pos--) {
    p = store[pos]->next();
    if (p) {
      p->remove();
      deck->add(p,TRUE,FALSE);
    }
  }
}

/*
bool step11( const Card* c1, const Card* c2 )  {
    
    return !c2 || (c2->Value() == c1->Value() + 1)
	&& (c1->Suit() != Card::Empty ? c1->Suit() == c2->Suit() : TRUE)
	&& step11( c2, c2->prev()) ;

}
*/

bool Grandf::step1( const Card* c1, const Card* c2 )  {
  return  (c2->Value() == c1->Value() + 1)
    && (c1->Suit() != Card::Empty ? c1->Suit() == c2->Suit() : TRUE);
    
  //    return step11( c1, c2->top() );
}

bool Grandf::Sstep1( const Card* c1, const Card* c2)  {
  if (c1->Suit() == Card::Empty) 
    return c2->Value() == Card::King;
  else
    return (c2->Value() == c1->Value() - 1) && c1->Suit() == c2->Suit();
}

QSize Grandf::sizeHint() const {
  return QSize(700, 476);
}

#include "grandf.moc"
