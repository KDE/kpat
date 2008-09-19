#include "demo.h"

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


static const int inner_margin = 7;

DemoBubbles::DemoBubbles( QWidget *parent)
    : QWidget( parent )
{
    backPix.load( KStandardDirs::locateLocal( "data", "kpat/back.png" ) );

    games = 0;
    for (QList<DealerInfo*>::ConstIterator it = DealerInfoList::self()->games().begin();
	it != DealerInfoList::self()->games().end(); ++it) {

        games++;

    }
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
    const int margin = 30;

    int mywidth = width();

    int rows = ( games + 1 ) / 4;

    while ( true )
    {

        bubble_width = ( mywidth - ( margin * 5 ) ) / 4;

        QRectF rect = bubblerenderer->boundsOnElement( "bubble" );
        QRectF text = bubblerenderer->boundsOnElement( "text" );
        bubble_height = int( bubble_width / rect.width() * rect.height() + 1 );
        bubble_text_height = int( bubble_width / text.width() * text.height() + 1 );

        kDebug() << mywidth << bubble_width << bubble_height << games << height();

        if ( ( bubble_height * rows + margin * ( rows + 1 ) ) > height() && bubble_text_height > inner_margin )
        {
            mywidth *= 0.9;
        } else
            break;

    }

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
    for ( int y = 0; y < games / 4 + 1; ++y )
    {
        for ( int x = 0; x < 4; ++x )
        {
            if ( index == games )
                break;
            it = DealerInfoList::self()->games().begin();
            while ( index != list.indexOf( i18n((*it)->name ) ) )
                ++it;
            m_bubbles[index].x = x * ( bubble_width + margin ) + margin;
            m_bubbles[index].y = y * ( bubble_height + margin ) + margin;
            m_bubbles[index].width = bubble_width;
            m_bubbles[index].height = bubble_height;
            m_bubbles[index].gameindex = ( *it )->gameindex;
            m_bubbles[index].active = false;
            m_bubbles[index].text = i18n( ( *it )->name ).replace( "&&", "&" );
            kDebug() << index <<  m_bubbles[index].text;
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

    QFont f( font() );
    int pixels = bubble_text_height - inner_margin;
    f.setPixelSize( pixels );
    painter.setFont( f );

    for ( int i = 0; i < games; ++i )
    {
        while ( painter.boundingRect( QRectF( 0, 0, width(), height() ), m_bubbles[i].text ).width() > bubble_width * 0.9 )
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
        painter.drawImage( m_bubbles[index].x + inner_margin,
                           m_bubbles[index].y + inner_margin + bubble_text_height, p );

        if ( m_bubbles[index].active )
            painter.setPen( Qt::white );
        else
            painter.setPen( Qt::black );
        painter.drawText( QRect( m_bubbles[index].x,
                                 m_bubbles[index].y,
                                 bubble_width - inner_margin * 2,
                                 bubble_text_height ),
                          Qt::AlignCenter, m_bubbles[index].text );

    }
}
