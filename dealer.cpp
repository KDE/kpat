#include "dealer.h"
#include <qobjcoll.h>
#include <kstaticdeleter.h>

DealerInfoList *DealerInfoList::_self = 0;
static KStaticDeleter<DealerInfoList> dl;

DealerInfoList *DealerInfoList::self()
{
    if (!_self)
        _self = dl.setObject(new DealerInfoList());
    return _self;
}

void DealerInfoList::add(DealerInfo *dealer)
{
    list.append(dealer);
}

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

void dealer::repaintCards()
{
    // this hack is needed, update() is not enough
    QObjectList *ol = queryList("basicCard");
    QObjectListIt it( *ol );
    while (it.current()) {
        QWidget *w = (QWidget *)it.current();
        ++it;
        w->repaint(true);
    }
    delete ol;
}

#include "dealer.moc"
