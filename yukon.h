#ifndef YUKON_H
#define YUKON_H

#include "dealer.h"

class Yukon : public Dealer {
    Q_OBJECT

public:
    Yukon( KMainWindow* parent=0, const char* name=0);
	virtual bool isGameLost() const;

public slots:
    void deal();
    virtual void restart();

private:
    Pile* store[7];
    Pile* target[4];
    Deck *deck;
};

#endif
