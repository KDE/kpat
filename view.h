/*
    Copyright (C) 1995  Paul Olav Tvete <paul@troll.no>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef VIEW_H
#define VIEW_H

#include <QGraphicsView>

class DealerScene;
class QWheelEvent;
class KXmlGuiWindow;
class QAction;

class PatienceView: public QGraphicsView
{
    friend class DealerScene;

    Q_OBJECT

public:

    PatienceView ( KXmlGuiWindow* _window, QWidget* _parent );
    virtual ~PatienceView();

    static PatienceView *instance();

    void setViewSize(const QSize &size);

    virtual void setupActions();

    QString anchorName() const;
    void setAnchorName(const QString &name);

    void setSolverEnabled(bool a);
    void setAutoDropEnabled(bool a);

    void setGameNumber(long gmn);
    long gameNumber() const;

    void setScene( QGraphicsScene *scene);
    DealerScene  *dscene() const;

    bool wasShown() const { return m_shown; }
    void setWasShown(bool b) { m_shown = b; }

    KXmlGuiWindow *mainWindow() const;

public slots:

    // restart is pure virtual, so we need something else
    virtual void startNew();
    void hint();
    void slotEnableRedeal(bool);
    void toggleDemo(bool);

protected:

    virtual void wheelEvent( QWheelEvent *e );
    virtual void resizeEvent( QResizeEvent *e );

protected:

    QAction *ademo;
    QAction *ahint, *aredeal, *adrop;

    QString ac;
    static PatienceView *s_instance;

    qreal scaleFactor;
    bool m_shown;

    KXmlGuiWindow * m_window;
};

#endif
