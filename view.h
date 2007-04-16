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
#ifndef _VIEW_H_
#define _VIEW_H_

#include <QGraphicsView>

class DealerScene;
class QWheelEvent;
class KXmlGuiWindow;
class QAction;
class KToggleAction;

class PatienceView: public QGraphicsView
{
    friend class DealerScene;

    Q_OBJECT

public:

    PatienceView ( KXmlGuiWindow* parent );
    virtual ~PatienceView();

    static PatienceView *instance();

    void setViewSize(const QSize &size);

    virtual void setupActions();

    QString anchorName() const;
    void setAnchorName(const QString &name);

    void setAutoDropEnabled(bool a);

    void setGameNumber(long gmn);
    long gameNumber() const;

    void setScene( QGraphicsScene *scene);
    DealerScene  *dscene() const;

    bool wasShown() const { return m_shown; }
    void setWasShown(bool b) { m_shown = b; }

public slots:

    // restart is pure virtual, so we need something else
    virtual void startNew();
    void hint();
    void slotEnableRedeal(bool);
    void toggleDemo(bool);

signals:
    void saveGame(); // emergency

public slots:

protected:

    virtual void wheelEvent( QWheelEvent *e );
    virtual void resizeEvent( QResizeEvent *e );

    KXmlGuiWindow *parent() const;

protected:

    QAction *ademo;
    QAction *ahint, *aredeal;

    QString ac;
    static PatienceView *s_instance;

    qreal scaleFactor;
    bool m_shown;
};

#endif
