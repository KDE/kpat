#include "dealer.h"

dealer::dealer( QWidget* _parent , const char* _name )
  : CardTable( _parent, _name )
{
}

dealer::~dealer()
{
}

void dealer::stopActivity()
{
  Card::stopMoving();
}

QSize dealer::sizeHint() const
{
  // just a dummy size
  //fprintf( stderr, "kpat: dealer::sizeHint() called!\n" );
  return QSize( 100, 100 );
}

void dealer::undo() {
  Card::undoLastMove();
}
