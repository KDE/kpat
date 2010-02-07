/*
 *  Portions Copyright (C) 2009 by Davide Bettio <davide.bettio@kdemail.net>
 *  Copyright (C) 2010 Parker Coates <parker.coates@kdemail.org>
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

#ifndef KCARDTHEMEWIDGET_P_H
#define KCARDTHEMEWIDGET_P_H

#include "kcardthemewidget.h"

#include "carddeckinfo.h"
#include "carddeckinfo_p.h"

class KPixmapCache;

#include <QtCore/QAbstractItemModel>
#include <QtGui/QAbstractItemDelegate>
class QListView;


class CardThemeModel : public QAbstractListModel
{
public:
    CardThemeModel( KCardThemeWidgetPrivate * d, QObject * parent = 0 );
    virtual ~CardThemeModel();

    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    void reload();
    QModelIndex indexOf( const QString & name ) const;

private:
    const KCardThemeWidgetPrivate * const d;
    QMap<QString,QPixmap*> m_previews;
};


class CardThemeDelegate : public QAbstractItemDelegate
{
public:
    CardThemeDelegate( KCardThemeWidgetPrivate * d, QObject * parent = 0 );

    virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    virtual QSize sizeHint( const QStyleOptionViewItem & option, const QModelIndex & index ) const;

private:
    const KCardThemeWidgetPrivate * const d; 
};


class KCardThemeWidgetPrivate
{
public:
    CardThemeModel * model;
    QListView * listView;
    KPixmapCache * cache;

    QList<QList<QString> > previewFormat;
    QSize baseCardSize;
    QSize previewSize;
    QSize itemSize;
    int itemMargin;
    int textHeight;
    qreal abstractPreviewWidth;
};

#endif