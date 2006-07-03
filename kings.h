/*
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
*/
#ifndef _KINGS_H_
#define _KINGS_H_

#include "freecell.h"

class Pile;
class Deck;
class KMainWindow;

class Kings : public FreecellBase {
    Q_OBJECT

public:
    Kings( KMainWindow* parent=0 );
    virtual bool isGameLost() const;

public slots:
    virtual void deal();
    virtual void demo();
};

#endif
