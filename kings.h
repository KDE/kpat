#ifndef _KINGS_H_
#define _KINGS_H_

#include "freecell.h"

class Pile;
class Deck;
class KMainWindow;

class Kings : public FreecellBase {
    Q_OBJECT

public:
    Kings( KMainWindow* parent=0, const char* name=0);
    virtual bool isGameLost() const;

public slots:
    virtual void deal();
    virtual void demo();
};

#endif
