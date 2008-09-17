#include "demo.h"

#include <kdebug.h>
#include <QSvgRenderer>
#include <KStandardDirs>
#include <QPainter>
#include <KLocale>

#include "dealer.h"

DemoBubbles::DemoBubbles( QWidget *parent)
    : QWidget( parent )
{
    backPix.load( KStandardDirs::locateLocal( "data", "kpat/back.png" ) );

    games = 0;
    for (QList<DealerInfo*>::ConstIterator it = DealerInfoList::self()->games().begin();
	it != DealerInfoList::self()->games().end(); ++it) {

        games++;
    }
    games++;
}

void DemoBubbles::resizeEvent( QResizeEvent *e )
{
    kDebug() << "resizeEvent" << size();
}

void DemoBubbles::paintEvent ( QPaintEvent * event )
{
    kDebug() << "paintEvent";
    if ( backPix.width() != width() || backPix.height() != height() )
    {
        QSvgRenderer *backren = new QSvgRenderer( KStandardDirs::locate( "data", "kpat/background.svg" ) );

        QImage img( width(), height(), QImage::Format_ARGB32);
        QPainter p2( &img );
        backren->render( &p2 );
        p2.end();
        backPix = QPixmap::fromImage( img );
        backPix.save( KStandardDirs::locateLocal( "data", "kpat/back.png" ), "PNG" );

        delete backren;
    }

    const int margin = 30;
    const int inner_margin = 7;
    int bubble_width = ( width() - ( margin * 5 ) ) / 4;
    int bubble_height = 0;

    kDebug() << "bubble_width" << width() << bubble_width;
    if ( bubblePix.width() != bubble_width )
    {
        QSvgRenderer *backren = new QSvgRenderer( KStandardDirs::locate( "data", "kpat/demo_bubble.svg" ) );
        QRectF rect = backren->boundsOnElement( "bubble" );
        QRectF text = backren->boundsOnElement( "text" );
        kDebug() << "bubble " << rect;
        bubble_height = int( bubble_width / rect.width() * rect.height() + 1 );
        bubble_text_height = int( bubble_width / text.width() * text.height() + 1 );
        QImage img( bubble_width, bubble_height, QImage::Format_ARGB32);
        QPainter p2( &img );
        backren->render( &p2, "bubble" );
        p2.end();
        bubblePix = QPixmap::fromImage( img );

        delete backren;
    } else {
        bubble_height = bubblePix.height();
    }

    QPainter painter( this );
    painter.drawPixmap( 0, 0, backPix );

    QList<DealerInfo*>::ConstIterator it = DealerInfoList::self()->games().begin();

    QFont f( font() );
    int pixels = bubble_text_height - inner_margin;
    f.setPixelSize( pixels );
    painter.setFont( f );

    for ( ; it != DealerInfoList::self()->games().end(); ++it )
    {
        QString text = i18n( ( *it )->name );
        while ( painter.boundingRect( QRectF( 0, 0, width(), height() ), text ).width() > bubble_width * 0.9 )
        {
            pixels--;
            f.setPixelSize( pixels );
            painter.setFont( f );
        }
    }

    // reset
    it = DealerInfoList::self()->games().begin();
    for ( int y = 0; y < games / 4; ++y )
    {
        for ( int x = 0; x < 4; ++x )
        {
            if ( it == DealerInfoList::self()->games().end() )
                break;
            painter.drawPixmap( x * ( bubble_width + margin ) + margin, y * ( bubble_height + margin ) + margin, bubblePix );
            QImage p;
            p.load( KStandardDirs::locate( "data", QString( "kpat/demos/demo_%1.png" ).arg( ( *it )->gameindex ) ) );

            p = p.scaled( bubble_width - inner_margin * 2,
                          bubble_height - inner_margin * 2 - bubble_text_height,
                          Qt::KeepAspectRatio );
            painter.drawImage( x * ( bubble_width + margin ) + margin + inner_margin,
                               y * ( bubble_height + margin ) + margin + inner_margin + bubble_text_height, p );
            QString text = i18n( ( *it )->name );
            text = text.replace( "&&", "&" );
            painter.drawText( QRect( x * ( bubble_width + margin ) + margin,
                                     y * ( bubble_height + margin ) + margin,
                                     bubble_width - inner_margin * 2,
                                     bubble_text_height ),
                              Qt::AlignCenter, text );

            ++it;
        }
    }

}
