/*
 *  Copyright (C) 2008 Andreas Pakulat <apaku@gmx.de>
 *  Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of 
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef KCARDCACHE_P_H
#define KCARDCACHE_P_H

class LoadThread;

class QMutex;
#include <QtCore/QSize>
#include <QtCore/QStringList>
#include <QtCore/QThread>
class QImage;
class QPixmap;
class QSvgRenderer;

class KPixmapCache;


class KCardCache2Private : public QObject
{
    Q_OBJECT

public:
    KPixmapCache * cache;
    LoadThread * loadThread;
    QSize size;
    QString theme;
    QMutex * rendererMutex;
    QSvgRenderer * svgRenderer;
    QSvgRenderer * renderer();

public slots:
    void submitRendering( const QString & key, const QImage & image );
};


class LoadThread : public QThread
{
    Q_OBJECT

public:
    LoadThread( KCardCache2Private * d, QSize size, const QString & theme, const QStringList & elements );
    void run();
    void kill();

signals:
    void renderingDone( const QString & key, const QImage & image );

private:
    KCardCache2Private * const d;
    const QSize size;
    const QString theme;
    const QStringList elementsToRender;
    bool doKill;
    QMutex * killMutex;
};

#endif
