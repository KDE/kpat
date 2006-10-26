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
#include <QList>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QGraphicsScene>
#include <QGraphicsView>

class QPixmap;
class QGraphicsItem;
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
    void countLoss();
    void countGame();

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

    void setBackgroundPixmap(const QPixmap &background, const QColor &midcolor);

    void addPile(Pile *p);
    void removePile(Pile *p);

    virtual bool isGameLost() const;
    virtual bool isGameWon() const;

    void setGameId(int id) { _id = id; }
    int gameId() const { return _id; }

    void startNew();

    virtual void restart() = 0;
    virtual void stopDemo();

    virtual bool checkRemove( int checkIndex, const Pile *c1, const Card *c) const;
    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const;

    // reimplement this to add game-specific information in the state structure
    virtual QString getGameState() const { return QString(); }
    // reimplement this to use the game-specific information from the state structure
    virtual void setGameState( const QString & ) {}

    QPointF maxPilePos() const;

    void setSceneSize( const QSize &s );

    void won();
    bool demoActive() const;

public slots:
    virtual bool startAutoDrop();
    void hint();
    void rescale(bool onlypiles);

    State *getState();
    void setState(State *);
    void relayoutPiles();
    void slotShowGame(bool);

protected slots:
    virtual void demo();
    void waitForDemo(Card *);
    void waitForWonAnim(Card *c);
    void toggleDemo(bool);

signals:
    void undoPossible(bool poss);
    void enableRedeal(bool enable);
    void gameInfo(const QString &info);
    void gameWon(bool withhelp);
    void demoActive(bool en);

private slots:
    void waitForAutoDrop(Card *);

protected:
    PileList piles;
    QList<MoveHint*> hints;
    KRandomSequence randseq;
    Card *towait;

    virtual Card *demoNewCards();
    virtual void newDemoMove(Card *m);

private:
    QList<QGraphicsItem *> marked;

    bool moved;
    CardList movingCards;

    QPointF moving_start;
    bool _autodrop;
    int _waiting;
    long gamenumber;

    QTimer *demotimer;
    bool stop_demo_next;
    qreal m_autoDropFactor;

    bool _won;
    quint32 _id;
    bool _gameRecorded;
    QGraphicsItem *wonItem;
    bool gothelp;
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

    void setViewSize(const QSize &size);

    virtual void setupActions();

    void saveGame(QDomDocument &doc);
    void openGame(QDomDocument &doc);

    QString anchorName() const;
    void setAnchorName(const QString &name);

    int getMoves() const { return undoList.count(); }

    void setAutoDropEnabled(bool a);

    void setGameNumber(long gmn);
    long gameNumber() const;

    void setScene( QGraphicsScene *scene);
    DealerScene  *dscene() const;

    enum { None = 0, Hint = 1, Demo = 2, Redeal = 4 } Actions;

    void setActions(int actions) { myActions = actions; }
    int actions() const { return myActions; }

    virtual void takeState();

    bool wasShown() const { return m_shown; }
    void setWasShown(bool b) { m_shown = b; }

public slots:

    // restart is pure virtual, so we need something else
    virtual void startNew();
    void undo();
    void hint();
    void slotTakeState(Card *c);
    void slotEnableRedeal(bool);

signals:
    void undoPossible(bool poss);
    void gameLost();
    void saveGame(); // emergency
    void updateMoves();

public slots:

protected:

    virtual void wheelEvent( QWheelEvent *e );
    virtual void resizeEvent( QResizeEvent *e );

    KMainWindow *parent() const;

protected:

    Dealer( Dealer& );  // don't allow copies or assignments
    void operator = ( Dealer& );  // don't allow copies or assignments


    QSize minsize;
    QSize viewsize;
    QList<State*> undoList;

    int myActions;
    bool toldAboutLostGame;

    KToggleAction *ademo;
    KAction *ahint, *aredeal;

    QString ac;
    static Dealer *s_instance;

    qreal scaleFactor;
    bool m_shown;
};

#endif
