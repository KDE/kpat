/*
 * Copyright (C) 2008-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2008-2009 Parker Coates <parker.coates@kdemail.net>
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

#ifndef GAMESELECTIONSCENE_H
#define GAMESELECTIONSCENE_H

#include <QtCore/QSignalMapper>
#include <QtGui/QGraphicsScene>


class GameSelectionScene : public QGraphicsScene
{
    Q_OBJECT

public:
    GameSelectionScene( QObject * parent );
    ~GameSelectionScene();

    void resizeScene( const QSize & size );

signals:
    void gameSelected( int i );

private:
    class GameSelectionBox;

    QList<GameSelectionBox*> m_boxes;
    QSignalMapper m_signalMapper;
};

#endif
