#ifndef _DEALER_H_
#define _DEALER_H_

#include <qvaluelist.h>
#include <qcanvas.h>
#include "card.h"

class KMainWindow;
class Dealer;
class DealerInfo;

class DealerInfoList {
public:
    static DealerInfoList *self();
    void add(DealerInfo *);

    const QValueList<DealerInfo*> games() const { return list; }
private:
    QValueList<DealerInfo*> list;
    static DealerInfoList *_self;
};

class DealerInfo {
public:
    DealerInfo(const char *_name, int _index)
        : name(_name),
          gameindex(_index)
{
    DealerInfoList::self()->add(this);
}
    const char *name;
    uint gameindex;
    virtual Dealer *createGame(KMainWindow *parent) = 0;
};

/***************************************************************

  Dealer -- abstract base class of all varieties of patience



  MORE WORK IS NEEDED -- especially on allocation/deallocation

***************************************************************/
class Dealer: public QCanvasView
{
    Q_OBJECT

public:

    Dealer( QWidget* parent = 0, const char* name = 0 );
    virtual ~Dealer();

    virtual QSize sizeHint() const;

public slots:

    virtual void restart() = 0;
    virtual void undo() {}
    virtual void show() = 0;

protected:

    virtual void contentsMousePressEvent(QMouseEvent* e);
    virtual void contentsMouseMoveEvent( QMouseEvent* );
    virtual void contentsMouseReleaseEvent( QMouseEvent* );
    virtual void contentsMouseDoubleClickEvent( QMouseEvent* );

    void unmarkAll();
    void mark(Card *c);
    virtual void cardClicked(Card *);
    virtual void pileClicked(Pile *);
    virtual void cardDblClicked(Card *);

private:

    bool moved;
    CardList movingCards;
    QCanvasItemList marked;
    QPoint moving_start;
    Dealer( Dealer& );  // don't allow copies or assignments
    void operator = ( Dealer& );  // don't allow copies or assignments
    QCanvas myCanvas;
};

#endif
