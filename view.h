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

#include <QtGui/QGraphicsView>
class QWheelEvent;

class PatienceView: public QGraphicsView
{
    Q_OBJECT

public:
    PatienceView ( QWidget* _parent );
    virtual ~PatienceView();

    void setScene( QGraphicsScene *scene );

protected:
    virtual void wheelEvent( QWheelEvent *e );
    virtual void resizeEvent( QResizeEvent *e );

};

#endif
