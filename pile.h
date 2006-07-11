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
#ifndef _PILE_H
#define _PILE_H


#include "card.h"
#include <QPixmap>
//Added by qt3to4:
#include <QList>
#include <QGraphicsPixmapItem>

class DealerScene;


/***************************************

  Pile -- A pile on the board that can hold cards.

**************************************/

class Pile : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

public:

    enum PileType { Custom, 
		    KlondikeTarget,
		    KlondikeStore, 
		    GypsyStore, 
		    FreeCell, 
		    FreecellStore};

    //  Add- and remove-flags
    static const int Default;
    static const int disallow;
    static const int several; // default: move one card

    // Add-flags
    static const int addSpread;

    // Remove-flags
    static const int autoTurnTop;
    static const int wholeColumn;

    Pile( int _index, DealerScene* parent = 0);
    virtual ~Pile();

    DealerScene  *dealer() const { return m_dealer; }
    CardList      cards()  const { return m_cards; }

    bool legalAdd(const CardList &c ) const;
    bool legalRemove(const Card *c) const;

    virtual void moveCards(CardList &c, Pile *to = 0);
    void moveCardsBack(CardList &c, bool anim = true);

    void setRemoveFlags( int flag ) { removeFlags = flag; }
    void setAddFlags( int flag ) { addFlags = flag; }

    void setCheckIndex( int index ) { _checkIndex = index; }
    virtual int checkIndex() const { return _checkIndex; }

    void setTarget(bool t) { _target = t; }
    bool target() const { return _target; }

    void setHighlighted( bool flag );
    bool isHighlighted() const { return m_highlighted; }

    CardList cardPressed(Card *c);

    Card *top() const;

    void add( Card *c, bool facedown, bool spread); // for initial deal
    void add( Card *c, int index = -1);
    void remove(Card *c);
    void clear();

    int  index()   const { return myIndex; }
    bool isEmpty() const { return m_cards.count() == 0; }

    void updateBrush();
    static const int my_type;

    virtual int  type() const       { return UserType + my_type; }

    virtual void setVisible(bool vis);
    virtual void moveBy(double dx, double dy);

    int cardsLeft() const { return m_cards.count(); }

    int indexOf(const Card *c) const;
    Card *at(int index) const;

    void hideCards( const CardList & cards );
    void unhideCards( const CardList & cards );

    virtual QSize cardOffset( bool _spread, bool _facedown, const Card *before) const;

    void resetCache();
    virtual void initSizes();

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

    // The spread properties.
    int  spread() const    { return _spread; }
    void setSpread(int s)  { _spread = s; }
    int  dspread() const   { return _dspread; }
    void setDSpread(int s) { _dspread = s; }
    int  hspread() const   { return _hspread; }
    void setHSpread(int s) { _hspread = s; }

public slots:
    virtual bool cardClicked(Card *c);
    virtual bool cardDblClicked(Card *c);

signals:
    void clicked(Card *c);
    void dblClicked(Card *c);

protected:
    int       removeFlags;
    int       addFlags;
    CardList  m_cards;

private:
    DealerScene  *m_dealer;

    PileType  _atype;
    PileType  _rtype;
    int       _spread;
    int       _hspread;
    int       _dspread;

    int _checkIndex;
    int myIndex;
    bool _target;

    QPixmap cache;
    QPixmap cache_selected;
    bool m_highlighted;

};

typedef QList<Pile*> PileList;

#endif
