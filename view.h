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

class DealerScene;

class KXmlGuiWindow;

#include <QtGui/QGraphicsView>
class QWheelEvent;

class PatienceView: public QGraphicsView
{
    Q_OBJECT

public:

    PatienceView ( KXmlGuiWindow* _window, QWidget* _parent );
    virtual ~PatienceView();

    static PatienceView *instance();

    void setScene( QGraphicsScene *scene);
    DealerScene  *dscene() const;

    KXmlGuiWindow *mainWindow() const;

protected:

    virtual void wheelEvent( QWheelEvent *e );
    virtual void resizeEvent( QResizeEvent *e );

protected:

    static PatienceView *s_instance;

    KXmlGuiWindow * m_window;
};

#endif
