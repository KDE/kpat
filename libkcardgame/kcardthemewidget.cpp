/*
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

#include "kcardthemewidget.h"
#include "kcardthemewidget_p.h"

#include "kcardcache.h"
#include "carddeckinfo.h"
#include "carddeckinfo_p.h"

#include <KConfigGroup>
#include <KDebug>
#include <KLocale>
#include <KGlobal>
#include <KPixmapCache>
#include <KStandardDirs>

#include <QtGui/QApplication>
#include <QtGui/QFontMetrics>
#include <QtGui/QListView>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QVBoxLayout>


CardThemeModel::CardThemeModel( KCardThemeWidgetPrivate * d, QObject * parent )
  : QAbstractListModel( parent ),
    d( d )
{
    reload();
}


CardThemeModel::~CardThemeModel()
{
    qDeleteAll( m_previews );
}


void CardThemeModel::reload()
{
    reset();

    qDeleteAll( m_previews );
    m_previews.clear();

    m_leftToRender.clear();

    KCardCache2 cardCache;

    foreach( const QString & name, CardDeckInfo::frontNames() )
    {
        QPixmap * pix = new QPixmap();
        if ( d->cache->find( name, *pix ) )
        {
            m_previews.insert( name, pix );
        }
        else
        {
            delete pix;
            m_previews.insert( name, 0 );
            m_leftToRender << name;
        }
    }

    beginInsertRows( QModelIndex(), 0, m_previews.size() );
    endInsertRows();

    if ( !m_leftToRender.isEmpty() )
        QTimer::singleShot( 0, this, SLOT(renderNext()) );
}


void CardThemeModel::renderNext()
{
    QString name = m_leftToRender.takeFirst();

    KCardCache2 cardCache;
    cardCache.setTheme( name );

    QSizeF s = cardCache.naturalSize("back");
    s.scale( 1.5 * d->baseCardSize.width(), d->baseCardSize.height(), Qt::KeepAspectRatio );
    cardCache.setSize( s.toSize() );

    qreal yPos = ( d->previewSize.height() - s.height() ) / 2.0;
    qreal spacingWidth = d->baseCardSize.width()
                         * ( d->previewSize.width() - d->previewFormat.size() * s.width() )
                         / ( d->previewSize.width() - d->previewFormat.size() * d->baseCardSize.width() );

    QPixmap * pix = new QPixmap( d->previewSize );
    pix->fill( Qt::transparent );
    QPainter p( pix );
    qreal xPos = 0;
    foreach ( const QList<QString> & sl, d->previewFormat )
    {
        foreach ( const QString & st, sl )
        {
            p.drawPixmap( xPos, yPos, cardCache.renderCard( st ) );
            xPos += 0.3 * spacingWidth;
        }
        xPos -= 0.3 * spacingWidth;
        xPos += 1 * s.width() + 0.1 * spacingWidth;
    }
    p.end();

    d->cache->insert( name, *pix );

    delete m_previews.value( name, 0 );
    m_previews.insert( name, pix );

    QModelIndex index = indexOf( name );
    emit dataChanged( index, index );

    if ( !m_leftToRender.isEmpty() )
        QTimer::singleShot( 500, this, SLOT(renderNext()) );
}


int CardThemeModel::rowCount(const QModelIndex & parent ) const
{
    Q_UNUSED( parent )
    return m_previews.size();
}


QVariant CardThemeModel::data( const QModelIndex & index, int role ) const
{
    if ( !index.isValid() || index.row() >= m_previews.size())
        return QVariant();

    QMap<QString,QPixmap*>::const_iterator it = m_previews.constBegin();
    for ( int i = 0; i < index.row(); ++i )
        ++it;

    switch ( role )
    {
    case Qt::DisplayRole:
        return it.key();
    case Qt::DecorationRole:
        return qVariantFromValue( (void*)(it.value()) );
    default:
        return QVariant();
    }
}


QModelIndex CardThemeModel::indexOf( const QString & name ) const
{
    QMap<QString,QPixmap*>::const_iterator it = m_previews.constBegin();
    for ( int i = 0; i < m_previews.size(); ++i )
    {
        if ( it.key() == name )
            return index( i, 0 );
        ++it;
    }

    return QModelIndex();
}


CardThemeDelegate::CardThemeDelegate( KCardThemeWidgetPrivate * d, QObject * parent )
  : QAbstractItemDelegate( parent ),
    d( d )
{
}


void CardThemeDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QApplication::style()->drawControl( QStyle::CE_ItemViewItem, &option, painter );

    painter->save();
    QFont font = painter->font();
    font.setWeight( QFont::Bold );
    painter->setFont( font );

    QRect previewRect( option.rect.left() + ( option.rect.width() - d->previewSize.width() ) / 2,
                       option.rect.top() + d->itemMargin,
                       d->previewSize.width(),
                       d->previewSize.height() );

    QVariant var = index.model()->data( index, Qt::DecorationRole );
    QPixmap * pix = static_cast<QPixmap*>( var.value<void*>() );
    if ( pix )
    {
        painter->drawPixmap( previewRect.topLeft(), *pix );
    }
    else
    {
        painter->fillRect( previewRect, QColor( 0, 0, 0, 16 ) );
        painter->drawText( previewRect, Qt::AlignCenter, i18n("Loading...") );
    }

    QRect textRect = option.rect.adjusted( 0, 0, 0, -d->itemMargin );
    QString name = index.model()->data( index, Qt::DisplayRole ).toString();
    painter->drawText( textRect, Qt::AlignHCenter | Qt::AlignBottom, name );

    painter->restore();
}


QSize CardThemeDelegate::sizeHint( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    Q_UNUSED( option )
    Q_UNUSED( index )
    return d->itemSize;
}


KCardThemeWidget::KCardThemeWidget( const QList<QList<QString> > & previewFormat, QWidget * parent )
  : QWidget( parent ),
    d( new KCardThemeWidgetPrivate() )
{
    d->previewFormat = previewFormat;

    d->abstractPreviewWidth = 0;
    for ( int i = 0; i < d->previewFormat.size(); ++i )
    {
        d->abstractPreviewWidth += 1.0;
        d->abstractPreviewWidth += 0.3 * ( d->previewFormat.at( i ).size() - 1 );
        if ( i + 1 < d->previewFormat.size() )
            d->abstractPreviewWidth += 0.1;
    }

    d->baseCardSize = QSize( 80, 100 );
    d->previewSize = QSize( d->baseCardSize.width() * d->abstractPreviewWidth, d->baseCardSize.height() );
    d->itemMargin = 5;
    d->textHeight = fontMetrics().height();
    d->itemSize = QSize( d->previewSize.width() + 2 * d->itemMargin, d->previewSize.height() + d->textHeight + 3 * d->itemMargin );

    d->cache = new KPixmapCache("libkcardgame-previews");

    d->model = new CardThemeModel( d, this );

    d->listView = new QListView( this );
    d->listView->setModel( d->model );
    d->listView->setItemDelegate( new CardThemeDelegate( d, d->model ) );
    d->listView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    d->listView->setAlternatingRowColors( true );

    // FIXME This is just a fudge factor. It should be possible to detemine
    // the actual width necessary including frame and scrollbar somehow.
    d->listView->setMinimumWidth( d->itemSize.width() * 1.1 ); 
    d->listView->setMinimumHeight( d->itemSize.height() * 2.5 );

    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->addWidget( d->listView );
}


KCardThemeWidget::~KCardThemeWidget()
{
    delete d->cache;
    delete d;
}


void KCardThemeWidget::setCurrentSelection( const QString & theme )
{
    QModelIndex index = d->model->indexOf( theme );
    if ( index.isValid() )
        d->listView->setCurrentIndex( index );
}


QString KCardThemeWidget::currentSelection() const
{
    QModelIndex index = d->listView->currentIndex();
    if ( index.isValid() )
        return d->model->data( index, Qt::DisplayRole ).toString();
    else
        return QString();
}

