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

    m_themes.clear();
    qDeleteAll( m_previews );
    m_previews.clear();
    m_leftToRender.clear();

    foreach( const KCardTheme & theme, KCardTheme::findAll() )
    {
        if ( !theme.isValid() )
            continue;

        QString mapKey = theme.dirName();

        KCardCache2 cache( theme );

        QPixmap * pix = new QPixmap();
        if ( cache.timestamp() >= theme.lastModified()
             && cache.findOther( "preview_" + d->previewString, *pix ) )
        {
            m_previews.insert( mapKey, pix );
        }
        else
        {
            delete pix;
            m_previews.insert( mapKey, 0 );
            m_leftToRender << theme;
        }
        m_themes.insert( mapKey, theme );
    }

    beginInsertRows( QModelIndex(), 0, m_themes.size() );
    endInsertRows();

    if ( !m_leftToRender.isEmpty() )
        QTimer::singleShot( 0, this, SLOT(renderNext()) );
}


void CardThemeModel::renderNext()
{
    KCardTheme theme = m_leftToRender.takeFirst();

    KCardCache2 cache( theme );

    QSizeF s = cache.naturalSize("back");
    s.scale( 1.5 * d->baseCardSize.width(), d->baseCardSize.height(), Qt::KeepAspectRatio );
    kDebug() << s;
    cache.setSize( s.toSize() );

    qreal yPos = ( d->previewSize.height() - s.height() ) / 2.0;
    qreal spacingWidth = d->baseCardSize.width()
                         * ( d->previewSize.width() - d->previewLayout.size() * s.width() )
                         / ( d->previewSize.width() - d->previewLayout.size() * d->baseCardSize.width() );

    QPixmap * pix = new QPixmap( d->previewSize );
    pix->fill( Qt::transparent );
    QPainter p( pix );
    qreal xPos = 0;
    foreach ( const QList<QString> & sl, d->previewLayout )
    {
        foreach ( const QString & st, sl )
        {
            p.drawPixmap( xPos, yPos, cache.renderCard( st ) );
            xPos += 0.3 * spacingWidth;
        }
        xPos -= 0.3 * spacingWidth;
        xPos += 1 * s.width() + 0.1 * spacingWidth;
    }
    p.end();

    QString dirName = theme.dirName();

    cache.insertOther( "preview_" + d->previewString, *pix );

    delete m_previews.value( dirName, 0 );
    m_previews.insert( dirName, pix );

    QModelIndex index = indexOf( dirName );
    emit dataChanged( index, index );

    if ( !m_leftToRender.isEmpty() )
        QTimer::singleShot( 500, this, SLOT(renderNext()) );
}


int CardThemeModel::rowCount(const QModelIndex & parent ) const
{
    Q_UNUSED( parent )
    return m_themes.size();
}


QVariant CardThemeModel::data( const QModelIndex & index, int role ) const
{
    if ( !index.isValid() || index.row() >= m_themes.size())
        return QVariant();

    if ( role == Qt::DisplayRole )
    {
        QMap<QString,KCardTheme>::const_iterator it = m_themes.constBegin();
        for ( int i = 0; i < index.row(); ++i )
            ++it;
        return it.value().displayName();
    }

    if ( role == Qt::DecorationRole )
    {
        QMap<QString,QPixmap*>::const_iterator it = m_previews.constBegin();
        for ( int i = 0; i < index.row(); ++i )
            ++it;
        return qVariantFromValue( (void*)(it.value()) );
    }

    return QVariant();
}


QModelIndex CardThemeModel::indexOf( const QString & name ) const
{
    QMap<QString,KCardTheme>::const_iterator it = m_themes.constBegin();
    for ( int i = 0; i < m_themes.size(); ++i )
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


KCardThemeWidget::KCardThemeWidget( const QString & previewString, QWidget * parent )
  : QWidget( parent ),
    d( new KCardThemeWidgetPrivate() )
{
    d->previewString = previewString;

    d->previewLayout.clear();
    foreach ( const QString & pile, previewString.split(';') )
        d->previewLayout << pile.split(',');

    d->abstractPreviewWidth = 0;
    for ( int i = 0; i < d->previewLayout.size(); ++i )
    {
        d->abstractPreviewWidth += 1.0;
        d->abstractPreviewWidth += 0.3 * ( d->previewLayout.at( i ).size() - 1 );
        if ( i + 1 < d->previewLayout.size() )
            d->abstractPreviewWidth += 0.1;
    }

    d->baseCardSize = QSize( 80, 100 );
    d->previewSize = QSize( d->baseCardSize.width() * d->abstractPreviewWidth, d->baseCardSize.height() );
    d->itemMargin = 5;
    d->textHeight = fontMetrics().height();
    d->itemSize = QSize( d->previewSize.width() + 2 * d->itemMargin, d->previewSize.height() + d->textHeight + 3 * d->itemMargin );

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
    delete d;
}


void KCardThemeWidget::setCurrentSelection( const QString & dirName )
{
    QModelIndex index = d->model->indexOf( dirName );
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

