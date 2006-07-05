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
#ifndef _DEALER_H_
#define _DEALER_H_

#include "pile.h"
#include "hint.h"
#include <krandomsequence.h>

#include <QWheelEvent>
#include <QPixmap>
#include <Q3PtrList>
#include <QList>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QGraphicsScene>
#include <QGraphicsView>

class QPixmap;
class QGraphicsRectItem;
class QDomDocument;
class KMainWindow;
class Dealer;
class DealerInfo;
class KAction;
class KSelectAction;
class KToggleAction;

class DealerInfoList {
public:
    static DealerInfoList *self();
    void add(DealerInfo *);

    const QList<DealerInfo*> games() const { return list; }
private:
    QList<DealerInfo*> list;
    static DealerInfoList *_self;
};

class DealerInfo 
{
public:
    DealerInfo(const char *_name, int _index)
        : name(_name),
	gameindex(_index)
	{
	    DealerInfoList::self()->add(this);
	}
	virtual ~DealerInfo(){}
	const char *name;
	int gameindex;
	virtual DealerScene *createGame() = 0;
};

class CardState;

typedef QList<CardState> CardStateList;

struct State
{
    CardStateList cards;
    QString gameData;
};

class DealerScene : public QGraphicsScene
{
    Q_OBJECT

public:
    DealerScene();
    ~DealerScene();
    void unmarkAll();
    void mark(Card *c);

protected:
    virtual void mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mouseMoveEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * mouseEvent );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * mouseEvent );

public:
    Pile *findTarget(Card *c);
    virtual bool cardClicked(Card *);
    virtual void pileClicked(Pile *);
    virtual bool cardDblClicked(Card *);

    bool isMoving(Card *c) const;

    virtual void getHints();
    void newHint(MoveHint *mh);
    void clearHints();
    // it's not const because it changes the random seed
    virtual MoveHint *chooseHint();

    void setAutoDropEnabled(bool a);
    bool autoDrop() const { return _autodrop; }

    void setGameNumber(long gmn);
    long gameNumber() const;

    bool waiting() const { return _waiting != 0; }
    void setWaiting(bool w);

    QColor midColor() const { return _midcolor; }
    void setBackgroundPixmap(const QPixmap &background, const QColor &midcolor);

    void addPile(Pile *p);
    void removePile(Pile *p);

    virtual bool isGameWon() const;
    int freeCells() const;

    void startNew();

    void drawPile(QPixmap &, Pile *p, bool selected);

    void enlargeCanvas(QGraphicsRectItem *c);

    virtual void restart() = 0;

public slots:
    virtual bool startAutoDrop();
    void hint();

signals:
    void undoPossible(bool poss);

private slots:
    void waitForAutoDrop(Card *);

private:
    QList<QGraphicsItem *> marked;

    bool moved;
    CardList movingCards;

    QPointF moving_start;
    bool _autodrop;
    QList<MoveHint*> hints;

    PileList piles;
    int _waiting;
    QColor _midcolor;

    KRandomSequence randseq;
    long gamenumber;
};


/***************************************************************

  Dealer -- abstract base class of all varieties of patience

***************************************************************/
class Dealer: public QGraphicsView
{
    friend class DealerScene;

    Q_OBJECT

public:

    enum UserTypes { CardTypeId = 1, PileTypeId = 2 };

    Dealer( KMainWindow* parent );
    virtual ~Dealer();

    static Dealer *instance();

    virtual bool isGameLost() const;
    virtual bool isGameWon() const;

    void setViewSize(const QSize &size);

    virtual bool checkRemove( int checkIndex, const Pile *c1, const Card *c) const;
    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const;

    virtual Card *demoNewCards();

    virtual void setupActions();

    bool demoActive() const;

    void saveGame(QDomDocument &doc);
    void openGame(QDomDocument &doc);

    void setGameId(int id) { _id = id; }
    int gameId() const { return _id; }

    void setTakeTargetForHints(bool e) { takeTargets = e; }
    bool takeTargetForHints() const { return takeTargets; }

    virtual QSize minimumCardSize() const;
    virtual void resizeEvent(QResizeEvent *);

    QString anchorName() const;
    void setAnchorName(const QString &name);

    int getMoves() const { return undoList.count(); }
    void setBackgroundPixmap(const QPixmap &background, const QColor &midcolor);

    void setAutoDropEnabled(bool a);

    void setGameNumber(long gmn);
    long gameNumber() const;

    void setScene( QGraphicsScene *scene);

public slots:

    // restart is pure virtual, so we need something else
    virtual void startNew();
    void undo();
    virtual void takeState();
    void hint();
    void slotTakeState(Card *c);

signals:
    void undoPossible(bool poss);
    void gameWon(bool withhelp);
    void gameLost();
    void saveGame(); // emergency
    void gameInfo(const QString &info);
    void updateMoves();

public slots:
    virtual void demo();
    void waitForDemo(Card *);
    void toggleDemo();
    virtual void stopDemo();

protected:

    enum { None = 0, Hint = 1, Demo = 2, Redeal = 4 } Actions;

    void setActions(int actions) { myActions = actions; }
    int actions() const { return myActions; }

    virtual void wheelEvent( QWheelEvent *e );

    void won();

    KMainWindow *parent() const;
  
protected:

    State *getState();
    void setState(State *);

    // reimplement this to add game-specific information in the state structure
    virtual QString getGameState() const { return QString::null; }
    // reimplement this to use the game-specific information from the state structure
    virtual void setGameState( const QString & ) {}

    virtual void newDemoMove(Card *m);

    Dealer( Dealer& );  // don't allow copies or assignments
    void operator = ( Dealer& );  // don't allow copies or assignments
    DealerScene  *dscene() const;

    QSize minsize;
    QSize viewsize;
    Q3PtrList<State> undoList;
    Card *towait;
    QTimer *demotimer;
    int myActions;
    bool toldAboutLostGame;

    KToggleAction *ademo;
    KAction *ahint, *aredeal;

    quint32 _id;
    bool takeTargets;
    bool _won;
    bool stop_demo_next;
    QString ac;
    static Dealer *s_instance;

    bool _gameRecorded;

private:
    void countLoss();
    void countGame();
};

#endif
