#ifndef _KINGS_H_
#define _KINGS_H_

#include "dealer.h"
#include "card.h"
#include "freecell.h"

class Pile;
class Deck;
class KMainWindow;

class Kings : public FreecellBase {
    Q_OBJECT

public:
    Kings( KMainWindow* parent=0, const char* name=0);

public slots:
    virtual void deal();
};

#endif
