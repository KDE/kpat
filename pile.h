#ifndef _PILE_H
#define _PILE_H

#include <qlabel.h>
#include "card.h"
class Dealer;

/***************************************

  cardPos -- position on the table which can receive cards

**************************************/
class Pile : public QCanvasRectangle
{

public:

    //  Add- and remove-flags
    static const int Default;
    static const int disallow;
    static const int several; // default: move one card
    static const int faceDown; //  move/add cards facedown

    // Add-flags
    static const int addSpread;
    static const int addRotated; // Note: cannot have Spread && Rotate
    static const int minus45;
    static const int plus45;
    static const int plus90;

    // Remove-flags
    static const int alsoFaceDown;
    static const int autoTurnTop;
    static const int wholeColumn;

    Pile( int _index, Dealer* parent = 0);
    virtual ~Pile();

    int   removeFlags;
    int   addFlags;
    QValueList<int> legalMoves;
    CardList myCards;

    CardList cards() const { return myCards; }
    typedef bool (*addCheckPtr)(const Pile*, const CardList& );
    addCheckPtr addCheckFun;

    typedef bool (*removeCheckPtr)(const Pile*);
    removeCheckPtr removeCheckFun;

    bool legalAdd(const CardList &c ) const;
    bool legalRemove(const Card *c) const;

    void moveCards(CardList &c, Pile *to = 0);
    void moveCardsBack(CardList &c);

    void clear();

    void setRemoveFlags( int flag ) { removeFlags = flag; }
    void setAddFlags( int flag ) { addFlags = flag; }
    void setLegalMove( const QValueList<int> &moves ) { legalMoves = moves; }
    void setAddFun( addCheckPtr f) { addCheckFun = f; }

    void setRemoveFun( removeCheckPtr f) { removeCheckFun = f; }

    CardList cardPressed(Card *c);

    Card *top() const;

    void add( Card* c, bool facedown, bool spread); // for initial deal
    void remove(Card *c);

    int index() const { return myIndex; }
    bool isEmpty() const { return myCards.count() == 0; }

    virtual void drawShape ( QPainter & p );
    static const int RTTI = 1002;

    virtual int rtti() const { return RTTI; }

    Dealer *dealer() const { return _dealer; }

    virtual void setVisible(bool vis);
    virtual void moveBy(double dx, double dy);

    int cardsLeft() const { return myCards.count(); }

private:
    Dealer *_dealer;
    int myIndex;
    int direction;
};

typedef QValueList<Pile*> PileList;

#endif
