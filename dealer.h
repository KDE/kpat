#ifndef _DEALER_H_
#define _DEALER_H_

#include "patience.h"
#include <qvaluelist.h>

class dealer;
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
    virtual dealer *createGame(QWidget *parent) = 0;
};

/***************************************************************

  dealer -- abstract base class of all varieties of patience



  MORE WORK IS NEEDED -- especially on allocation/deallocation

***************************************************************/
class dealer: public CardTable
{
  Q_OBJECT

public:

  dealer( QWidget* parent = 0, const char* name = 0 );
  virtual ~dealer();

  virtual QSize sizeHint() const;

  virtual void repaintCards();

public slots:

  virtual void restart() = 0;
  virtual void undo();
  virtual void show() = 0;

protected:

  void stopActivity();

private:

  dealer( dealer& );  // don't allow copies or assignments
  void operator = ( dealer& );  // don't allow copies or assignments
};

#endif
