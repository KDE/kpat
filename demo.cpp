#include "demo.h"

#include <cmath>

#include <kdebug.h>
#include <QSvgRenderer>
#include <KStandardDirs>
#include <QPainter>
#include <KLocale>
#include <QMouseEvent>

#include "dealer.h"

class GameBubble
{
public:
    GameBubble() {
        active = false;
        width = height = 0;
    }
    int x, y;
    int width, height;
    int gameindex;
    bool active;
    QImage pix;
    QString text;
};


static const qreal inner_margin_ratio = 0.035;
static const qreal spacing_ratio = 0.10;
static const int minimum_font_size = 8;

DemoBubbles::DemoBubbles( QWidget *parent)
    : QWidget( parent )
{
    backPix.load( KStandardDirs::locateLocal( "data", "kpat/back.png" ) );

    games = DealerInfoList::self()->games().size();

    m_bubbles = new GameBubble[games];
    bubblerenderer = new QSvgRenderer( KStandardDirs::locate( "data", "kpat/demo_bubble.svg" ) );

    setMouseTracking( true );
}

DemoBubbles::~DemoBubbles()
{
    delete [] m_bubbles;
    delete bubblerenderer;
}

void DemoBubbles::resizeEvent ( QResizeEvent * )
{
    QRectF bubble_rect = bubblerenderer->boundsOnElement( "bubble" );
    QRectF text_rect = bubblerenderer->boundsOnElement( "text" );

    qreal bubble_aspect = bubble_rect.width() / bubble_rect.height();
    qreal my_aspect = qreal(width()) / height();

    int rows, cols, best_rows;
    qreal lowest_error = 1000000.0;
    for ( rows = 1; rows <= games; ++rows )
    {
        cols = ceil( qreal( games ) / rows );

        // Skip combinations that would lead an empty last row
        if ( cols * rows - games >= cols )
            continue;

        qreal grid_aspect = bubble_aspect * cols / rows;
        qreal error = grid_aspect > my_aspect ? grid_aspect / my_aspect :  my_aspect / grid_aspect;
        if ( error < lowest_error )
        {
            lowest_error = error;
            best_rows = rows;
        }
    }
    rows = best_rows;
    cols = ceil( qreal( games ) / best_rows );

    int my_height, outer_margin, inner_margin;
    int my_width = ( width() / 10 ) * 10;
    while ( true )
    {
        bubble_width = qRound( my_width / ( cols + ( cols + 1 ) * spacing_ratio ) );
        outer_margin = qRound( spacing_ratio * bubble_width );
        inner_margin = qRound( inner_margin_ratio * bubble_width );

        bubble_height = int( bubble_width / bubble_rect.width() * bubble_rect.height() + 1 );
        bubble_text_height = int( bubble_width / text_rect.width() * text_rect.height() + 1 );

        my_height = bubble_height * rows + outer_margin * ( rows + 1 );

        if ( my_height > height() && bubble_text_height > inner_margin )
            my_width -= 10;
        else
            break;
    }

    int x_offset = ( width() - my_width ) / 2;
    int y_offset = ( height() - my_height ) / 2;

    QStringList list;
    QList<DealerInfo*>::ConstIterator it;

    for (it = DealerInfoList::self()->games().begin();
         it != DealerInfoList::self()->games().end(); ++it)
    {
        list.append( i18n((*it)->name) );
    }
    list.sort();

    int index = 0;
    // reset
    for ( int y = 0; y < rows; ++y )
    {
        for ( int x = 0; x < cols; ++x )
        {
            if ( index == games )
                break;
            it = DealerInfoList::self()->games().begin();
            while ( index != list.indexOf( i18n((*it)->name ) ) )
                ++it;
            m_bubbles[index].x = x_offset + x * ( bubble_width + outer_margin ) + outer_margin;
            m_bubbles[index].y = y_offset + y * ( bubble_height + outer_margin ) + outer_margin;
            m_bubbles[index].width = bubble_width;
            m_bubbles[index].height = bubble_height;
            m_bubbles[index].gameindex = ( *it )->gameindex;
            m_bubbles[index].active = false;
            m_bubbles[index].text = i18n( ( *it )->name ).replace( "&&", "&" );
            if ( m_bubbles[index].pix.isNull() )
                m_bubbles[index].pix.load( KStandardDirs::locate( "data", QString( "kpat/demos/demo_%1.png" ).arg( ( *it )->gameindex ) ) );
            ++index;
        }
    }
}

void DemoBubbles::mouseMoveEvent ( QMouseEvent *event )
{
    for ( int i = 0; i < games; ++i )
    {
        //kDebug() << m_bubbles[i].x << m_bubbles[i].y << m_bubbles[i].width << m_bubbles[i].height << event->pos();
        if ( m_bubbles[i].x < event->x() && m_bubbles[i].y < event->y() &&
             m_bubbles[i].width + m_bubbles[i].x > event->x() &&
             m_bubbles[i].height + m_bubbles[i].y > event->y() )
        {
            if ( !m_bubbles[i].active )
            {
                update( m_bubbles[i].x,
                        m_bubbles[i].y,
                        m_bubbles[i].width,
                        m_bubbles[i].height );
            }
            m_bubbles[i].active = true;
        } else {
            if ( m_bubbles[i].active )
            {
                update( m_bubbles[i].x,
                        m_bubbles[i].y,
                        m_bubbles[i].width,
                        m_bubbles[i].height );
            }
            m_bubbles[i].active = false;
        }
    }
}

void DemoBubbles::mousePressEvent ( QMouseEvent * event )
{
    mouseMoveEvent( event );
    for ( int i = 0; i < games; ++i )
    {
        if ( m_bubbles[i].active )
        {
            emit gameSelected( m_bubbles[i].gameindex );
            return;
        }
    }
}

void DemoBubbles::paintEvent ( QPaintEvent * )
{
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

    if ( bubblePix.width() != bubble_width )
    {
        QImage img( bubble_width, bubble_height, QImage::Format_ARGB32);
        img.fill( qRgba( 0, 0, 255, 0 ) );
        QPainter p2( &img );
        bubblerenderer->render( &p2, "bubble" );
        p2.end();
        bubblePix = QPixmap::fromImage( img );
    }

    QPainter painter( this );
    painter.drawPixmap( 0, 0, backPix );

    int inner_margin = qRound( inner_margin_ratio * bubble_width );
    int pixels = bubble_text_height - inner_margin;
    QFont f( font() );
    f.setPixelSize( pixels );
    painter.setFont( f );

    for ( int i = 0; i < games; ++i )
    {
        while ( pixels > minimum_font_size &&
                painter.boundingRect( QRectF( 0, 0, width(), height() ), m_bubbles[i].text ).width() > bubble_width * 0.9 )
        {
            pixels--;
            f.setPixelSize( pixels );
            painter.setFont( f );
        }
    }

    for ( int index = 0; index < games; ++index )
    {
        painter.drawPixmap( m_bubbles[index].x,
                            m_bubbles[index].y, bubblePix );

        QImage p = m_bubbles[index].pix.scaled( bubble_width - inner_margin * 2,
                                                bubble_height - inner_margin * 2 - bubble_text_height,
                                                Qt::KeepAspectRatio );
        int off = ( m_bubbles[index].width - p.width() ) / 2;
        painter.drawImage( m_bubbles[index].x + off,
                           m_bubbles[index].y + inner_margin + bubble_text_height, p );

        if ( pixels >= minimum_font_size )
        {
            if ( m_bubbles[index].active )
                painter.setPen( Qt::white );
            else
                painter.setPen( Qt::black );
            painter.drawText( QRect( m_bubbles[index].x,
                                    m_bubbles[index].y,
                                    bubble_width,
                                    bubble_text_height ),
                            Qt::AlignCenter, m_bubbles[index].text );
        }

    }
}
