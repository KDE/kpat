#ifndef _PILE_H
#define _PILE_H

#include <qlabel.h>
#include "card.h"
#include <kpixmap.h>

class Dealer;

/***************************************

  cardPos -- position on the table which can receive cards

**************************************/
class Pile : public QObject, public QCanvasRectangle
{
    Q_OBJECT

public:

    //  Add- and remove-flags
    static const int Default;
    static const int disallow;
    static const int several; // default: move one card

    // Add-flags
    static const int addSpread;

    // Remove-flags
    static const int autoTurnTop;
    static const int wholeColumn;

    Pile( int _index, Dealer* parent = 0);
    virtual ~Pile();

    CardList cards() const { return myCards; }

    bool legalAdd(const CardList &c ) const;
    bool legalRemove(const Card *c) const;

    void moveCards(CardList &c, Pile *to = 0);
    void moveCardsBack(CardList &c, bool anim = true);

    void setRemoveFlags( int flag ) { removeFlags = flag; }
    void setAddFlags( int flag ) { addFlags = flag; }

    void setLegalMove( const QValueList<int> &moves ) { legalMoves = moves; }

    void setCheckIndex( int index ) { _checkIndex = index; }
    virtual int checkIndex() const { return _checkIndex; }

    void setTarget(bool t) { _target = t; }
    bool target() const { return _target; }

    CardList cardPressed(Card *c);

    Card *top() const;

    void add( Card* c, bool facedown, bool spread); // for initial deal
    void add( Card *c, int index = -1);
    void remove(Card *c);
    void clear();

    int index() const { return myIndex; }
    bool isEmpty() const { return myCards.count() == 0; }

    virtual void drawShape ( QPainter & p );
    static const int RTTI = 1002;

    virtual int rtti() const { return RTTI; }

    Dealer *dealer() const { return _dealer; }

    virtual void setVisible(bool vis);
    virtual void moveBy(double dx, double dy);

    int cardsLeft() const { return myCards.count(); }

    int indexOf(const Card *c) const;
    Card *at(int index) const;

    void hideCards( const CardList & cards );
    void unhideCards( const CardList & cards );

    virtual QSize cardOffset( bool _spread, bool _facedown, const Card *before) const;

    void resetCache();

    static int SPREAD;
    static int DSPREAD;
    static int HSPREAD;

    enum PileType { Custom, KlondikeTarget, KlondikeStore, GypsyStore, FreeCell, FreecellStore};
    void setType( PileType t);
    void setAddType( PileType t);
    void setRemoveType( PileType t);
    PileType addType() const { return _atype; }
    PileType removeType() const { return _rtype; }

    // pile_algorithms
    bool add_klondikeTarget( const CardList& c2 ) const;
    bool add_klondikeStore( const CardList& c2 ) const;
    bool add_gypsyStore( const CardList& c2 ) const;
    bool add_freeCell( const CardList& c2) const;

    bool remove_freecellStore( const Card *c) const;

public slots:
    virtual void cardClicked(Card *c);
    virtual void cardDblClicked(Card *c);

signals:
    void clicked(Card *c);
    void dblClicked(Card *c);

protected:
    int   removeFlags;
    int   addFlags;
    QValueList<int> legalMoves;
    CardList myCards;

private:
    int _checkIndex;
    Dealer *_dealer;
    int myIndex;
    bool _target;
    KPixmap cache, cache_selected;
    PileType _atype, _rtype;
};

typedef QValueList<Pile*> PileList;

#endif
