#include "demo.h"
#include "dealer.h"
#include "render.h"

#include <cmath>

#include <QPainter>
#include <QMouseEvent>

#include <kdebug.h>
#include <klocale.h>

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
    QString text;
};


static const qreal inner_margin_ratio = 0.035;
static const qreal spacing_ratio = 0.10;
static const int minimum_font_size = 8;

DemoBubbles::DemoBubbles( QWidget *parent)
    : QWidget( parent )
{
    games = DealerInfoList::self()->games().size();

    m_bubbles = new GameBubble[games];

    setMouseTracking( true );
}

DemoBubbles::~DemoBubbles()
{
    delete [] m_bubbles;
}

void DemoBubbles::resizeEvent ( QResizeEvent * )
{
    QRectF bubble_rect = Render::boundsOnElement( "bubble" );
    QRectF text_rect = Render::boundsOnElement( "bubble_text_area" );

    qreal bubble_aspect = bubble_rect.width() / bubble_rect.height();
    qreal my_aspect = qreal(width()) / height();

    int rows, cols, best_rows = 1;
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

            GameBubble & bubble = m_bubbles[index];
            bubble.x = x_offset + x * ( bubble_width + outer_margin ) + outer_margin;
            bubble.y = y_offset + y * ( bubble_height + outer_margin ) + outer_margin;
            bubble.width = bubble_width;
            bubble.height = bubble_height;
            bubble.gameindex = ( *it )->ids.first();
            bubble.active = false;
            bubble.text = i18n( ( *it )->name );
            ++index;
        }
    }
}

void DemoBubbles::mouseMoveEvent ( QMouseEvent *event )
{
    for ( int i = 0; i < games; ++i )
    {
        GameBubble & bubble = m_bubbles[i];

        if ( bubble.x < event->x() && bubble.y < event->y() &&
             bubble.width + bubble.x > event->x() &&
             bubble.height + bubble.y > event->y() )
        {
            if ( !bubble.active )
            {
                update( bubble.x,
                        bubble.y,
                        bubble.width,
                        bubble.height );
            }
            bubble.active = true;
        } else {
            if ( bubble.active )
            {
                update( bubble.x,
                        bubble.y,
                        bubble.width,
                        bubble.height );
            }
            bubble.active = false;
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
    QPainter painter( this );
    painter.drawPixmap( 0, 0, Render::renderElement( "background", size() ) );

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
        const GameBubble & bubble = m_bubbles[index];

        painter.drawPixmap( bubble.x, bubble.y,
                            Render::renderElement( bubble.active ? "bubble_hover" : "bubble", 
                                                   QSize(bubble_width, bubble_height) ) );

        QSize size( bubble_width - inner_margin * 2, bubble_height - inner_margin * 2 - bubble_text_height );
        QPixmap pix = Render::renderGamePreview( bubble.gameindex, size );

        int off = ( bubble.width - pix.width() ) / 2;
        painter.drawPixmap( bubble.x + off,
                            bubble.y + inner_margin + bubble_text_height, pix );

        if ( pixels >= minimum_font_size )
        {
            QRect rect( bubble.x, bubble.y, bubble_width, bubble_text_height );
            painter.drawText( rect, Qt::AlignCenter, bubble.text );
        }
    }
}
