/*
 * Copyright (C) 2008-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2008-2009 Parker Coates <parker.coates@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEMO_H
#define DEMO_H

class GameBubble;

#include <QWidget>


class DemoBubbles : public QWidget
{
    Q_OBJECT

public:
    DemoBubbles( QWidget *parent);
    ~DemoBubbles();
    virtual void paintEvent ( QPaintEvent * event );
    virtual void mouseMoveEvent ( QMouseEvent * event );
    virtual void mousePressEvent ( QMouseEvent * event );
    virtual void resizeEvent ( QResizeEvent * event );

signals:
    void gameSelected( int i );

private:
    int games;
    int bubble_text_height, bubble_width, bubble_height;

    GameBubble *m_bubbles;
};

#endif // DEMO_H
