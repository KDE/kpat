#ifndef _DEALER_H_
#define _DEALER_H_

#include "patience.h"

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
