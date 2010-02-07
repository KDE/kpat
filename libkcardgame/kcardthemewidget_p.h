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

#include "kcardtheme.h"

class KPixmapCache;

#include <QtCore/QAbstractItemModel>
#include <QtCore/QTimer>
#include <QtGui/QAbstractItemDelegate>
class QListView;


class CardThemeModel : public QAbstractListModel
{
    Q_OBJECT

public:
    CardThemeModel( KCardThemeWidgetPrivate * d, QObject * parent = 0 );
    virtual ~CardThemeModel();

    void reload();
    QModelIndex indexOf( const QString & dirName ) const;

    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

private slots:
    void renderNext();

private:
    const KCardThemeWidgetPrivate * const d;
    QMap<QString,KCardTheme> m_themes;
    QMap<QString,QPixmap*> m_previews;
    QList<KCardTheme> m_leftToRender;
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


class KCardThemeWidgetPrivate : public QObject
{
    Q_OBJECT

public slots:
    void updateLineEdit( const QModelIndex & index );
    void updateListView( const QString & dirName );

public:
    CardThemeModel * model;
    QLineEdit * hiddenLineEdit;
    QListView * listView;

    int itemMargin;
    int textHeight;
    qreal abstractPreviewWidth;
    QSize baseCardSize;
    QSize previewSize;
    QSize itemSize;
    QString previewString;
    QList<QList<QString> > previewLayout;
};

#endif