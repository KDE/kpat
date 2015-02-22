/*
 *  Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#include "common.h"
#include <KImageCache>
#include <KConfigDialog>
#include <QDebug>
#include <KLineEdit>
#include <KLocalizedString>
#include <KImageCache>
#include <QPushButton>
#include <kns3/downloaddialog.h>

#include <QtCore/QMutexLocker>
#include <QtCore/QScopedPointer>
#include <QApplication>
#include <QFontMetrics>
#include <QListView>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>
#include <QtSvg/QSvgRenderer>


namespace
{
    inline QString timestampKey( const KCardTheme & theme )
    {
        return theme.dirName() + "_timestamp";
    }

    inline QString previewKey( const KCardTheme & theme, const QString & previewString )
    {
        return theme.dirName() + '_' + previewString;
    }
}


PreviewThread::PreviewThread( const KCardThemeWidgetPrivate * d, const QList<KCardTheme> & themes )
  : d( d ),
    m_themes( themes ),
    m_haltFlag( false ),
    m_haltMutex()
{
}


void PreviewThread::halt()
{
    {
        QMutexLocker l( &m_haltMutex );
        m_haltFlag = true;
    }
    wait();
}


void PreviewThread::run()
{
    foreach( const KCardTheme & theme, m_themes )
    {
        {
            QMutexLocker l( &m_haltMutex );
            if ( m_haltFlag )
                return;
        }

        QImage img( d->previewSize, QImage::Format_ARGB32 );
        img.fill( Qt::transparent );
        QPainter p( &img );

        QSvgRenderer renderer( theme.graphicsFilePath() );

        QSizeF size = renderer.boundsOnElement("back").size();
        size.scale( 1.5 * d->baseCardSize.width(), d->baseCardSize.height(), Qt::KeepAspectRatio );

        qreal yPos = ( d->previewSize.height() - size.height() ) / 2;
        qreal spacingWidth = d->baseCardSize.width()
                             * ( d->previewSize.width() - d->previewLayout.size() * size.width() )
                             / ( d->previewSize.width() - d->previewLayout.size() * d->baseCardSize.width() );

        qreal xPos = 0;
        foreach ( const QList<QString> & pile, d->previewLayout )
        {
            foreach ( const QString & card, pile )
            {
                renderer.render( &p, card, QRectF( QPointF( xPos, yPos ), size ) );
                xPos += 0.3 * spacingWidth;
            }
            xPos += 1 * size.width() + ( 0.1 - 0.3 ) * spacingWidth;
        }

        emit previewRendered( theme, img );
    }
}


CardThemeModel::CardThemeModel( KCardThemeWidgetPrivate * d, QObject * parent )
  : QAbstractListModel( parent ),
    d( d ),
    m_thread( 0 )
{
    qRegisterMetaType<KCardTheme>();

    reload();
}


CardThemeModel::~CardThemeModel()
{
    deleteThread();

    qDeleteAll( m_previews );
}


bool lessThanByDisplayName( const KCardTheme & a, const KCardTheme & b )
{
    return a.displayName() < b.displayName();
}


void CardThemeModel::reload()
{
    deleteThread();

    reset();

    m_themes.clear();
    qDeleteAll( m_previews );
    m_previews.clear();

    QList<KCardTheme> previewsNeeded;

    foreach( const KCardTheme & theme, KCardTheme::findAllWithFeatures( d->requiredFeatures ) )
    {
        if ( !theme.isValid() )
            continue;

        QPixmap * pix = new QPixmap();
        QDateTime timestamp;
        if ( cacheFind( d->cache, timestampKey( theme ), &timestamp )
             && timestamp >= theme.lastModified() 
             && d->cache->findPixmap( previewKey( theme, d->previewString ), pix ) )
        {
            m_previews.insert( theme.displayName(), pix );
        }
        else
        {
            delete pix;
            m_previews.insert( theme.displayName(), 0 );
            previewsNeeded << theme;
        }

        m_themes.insert( theme.displayName(), theme );
    }

    beginInsertRows( QModelIndex(), 0, m_themes.size() );
    endInsertRows();

    if ( !previewsNeeded.isEmpty() )
    {
        qSort( previewsNeeded.begin(), previewsNeeded.end(), lessThanByDisplayName ) ;

        m_thread = new PreviewThread( d, previewsNeeded );
        connect(m_thread, &PreviewThread::previewRendered, this, &CardThemeModel::submitPreview, Qt::QueuedConnection );
        m_thread->start();
    }
}


void CardThemeModel::deleteThread()
{
    if ( m_thread && m_thread->isRunning() )
        m_thread->halt();
    delete m_thread;
    m_thread = 0;
}


void CardThemeModel::submitPreview( const KCardTheme & theme, const QImage & image )
{
    d->cache->insertImage( previewKey( theme, d->previewString ), image );
    cacheInsert( d->cache, timestampKey( theme ), theme.lastModified() );

    QPixmap * pix = new QPixmap( QPixmap::fromImage( image ) );
    delete m_previews.value( theme.displayName(), 0 );
    m_previews.insert( theme.displayName(), pix );

    QModelIndex index = indexOf( theme.dirName() );
    emit dataChanged( index, index );
}


int CardThemeModel::rowCount( const QModelIndex & parent ) const
{
    Q_UNUSED( parent )
    return m_themes.size();
}


QVariant CardThemeModel::data( const QModelIndex & index, int role ) const
{
    if ( !index.isValid() || index.row() >= m_themes.size())
        return QVariant();

    if ( role == Qt::UserRole )
    {
        QMap<QString,KCardTheme>::const_iterator it = m_themes.constBegin();
        for ( int i = 0; i < index.row(); ++i )
            ++it;
        return it.value().dirName();
    }

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


QModelIndex CardThemeModel::indexOf( const QString & dirName ) const
{
    QMap<QString,KCardTheme>::const_iterator it = m_themes.constBegin();
    for ( int i = 0; i < m_themes.size(); ++i )
    {
        if ( it.value().dirName() == dirName )
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


KCardThemeWidgetPrivate::KCardThemeWidgetPrivate( KCardThemeWidget * q )
  : QObject( q ),
    q( q )
{
}


void KCardThemeWidgetPrivate::updateLineEdit( const QModelIndex & index )
{
    hiddenLineEdit->setText( model->data( index, Qt::UserRole ).toString() );
}


void KCardThemeWidgetPrivate::updateListView( const QString & dirName )
{
    QModelIndex index = model->indexOf( dirName );
    if ( index.isValid() )
        listView->setCurrentIndex( index );
}


void KCardThemeWidgetPrivate::getNewCardThemes()
{
    QPointer<KNS3::DownloadDialog> dialog = new KNS3::DownloadDialog( "kcardtheme.knsrc", q );
    dialog->exec();
    if ( dialog && !dialog->changedEntries().isEmpty() )
        model->reload();
    delete dialog;
}


KCardThemeWidget::KCardThemeWidget( const QSet<QString> & requiredFeatures, const QString & previewString, QWidget * parent )
  : QWidget( parent ),
    d( new KCardThemeWidgetPrivate( this ) )
{
    d->cache = new KImageCache( "libkcardgame-themes/previews", 1 * 1024 * 1024 );
    d->cache->setPixmapCaching( false );
    d->cache->setEvictionPolicy( KSharedDataCache::EvictOldest );

    d->requiredFeatures = requiredFeatures;
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

    d->hiddenLineEdit = new KLineEdit( this );
    d->hiddenLineEdit->setObjectName( QLatin1String( "kcfg_CardTheme" ) );
    d->hiddenLineEdit->hide();
    connect( d->listView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), d, SLOT(updateLineEdit(QModelIndex)) );
    connect( d->hiddenLineEdit, SIGNAL(textChanged(QString)), d, SLOT(updateListView(QString)) );

    d->newDeckButton = new QPushButton( QIcon::fromTheme( QLatin1String( "get-hot-new-stuff") ), i18n("Get New Card Decks..." ), this );
    connect( d->newDeckButton, SIGNAL(clicked(bool)), d, SLOT(getNewCardThemes()) );

    QHBoxLayout * hLayout = new QHBoxLayout();
    hLayout->addWidget( d->newDeckButton );
    hLayout->addStretch( 1 );

    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->addWidget( d->listView );
    layout->addWidget( d->hiddenLineEdit );
    layout->addLayout( hLayout );
}


KCardThemeWidget::~KCardThemeWidget()
{
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
        return d->model->data( index, Qt::UserRole ).toString();
    else
        return QString();
}


KCardThemeDialog::KCardThemeDialog( QWidget * parent, KConfigSkeleton * config, const QSet<QString> & requiredFeatures, const QString & previewString )
  : KConfigDialog( parent, "KCardThemeDialog", config )
{
    // Leaving the header text and icon empty prevents the header from being shown.
    addPage( new KCardThemeWidget( requiredFeatures, previewString, this ), QString() );

    setFaceType( KPageDialog::Plain );
    setStandardButtons( QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
}


KCardThemeDialog::~KCardThemeDialog()
{
}


bool KCardThemeDialog::showDialog()
{
    return KConfigDialog::showDialog( "KCardThemeDialog" );
}

