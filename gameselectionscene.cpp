/*
 * Copyright (C) 2008-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2008-2009 Parker Coates <parker.coates@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gameselectionscene.h"

#include "dealerinfo.h"
#include "render.h"
#include "speeds.h"

#include <KColorUtils>
#include <KLocale>

#include <QtCore/QBasicTimer>
#include <QtGui/QGraphicsItem>
#include <QtGui/QPainter>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QtCore/QPropertyAnimation>

#include <cmath>


static const qreal boxPaddingRatio = 0.035;
static const qreal spacingRatio = 0.10;
static const qreal textToTotalHeightRatio = 1 / 6.0;

#include <KDebug>
class GameSelectionScene::GameSelectionBox : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY( qreal fade READ hoverFadeAmount WRITE setHoverFadeAmount )

public:
    GameSelectionBox( const QString & name, int id )
      : m_label( name ),
        m_gameId( id ),
        m_anim( new QPropertyAnimation( this, "fade", this ) ),
        m_highlightFadeAmount( 0 )
    {
        setAcceptHoverEvents( true );
        m_anim->setDuration( DURATION_HOVERFADE );
        m_anim->setStartValue( qreal( 0.0 ) );
        m_anim->setEndValue( qreal( 1.0 ) );
        m_anim->setEasingCurve( QEasingCurve::InOutSine );
    }

    void setSize( const QSize & size )
    {
        m_size = size;
    }

    virtual QRectF boundingRect() const
    {
        return QRectF( QPointF(), m_size );
    }

    QString label() const
    {
        return m_label;
    }

    static bool lessThan( const GameSelectionBox * a, const GameSelectionBox * b )
    {
        return a->m_label < b->m_label;
    }

signals:
    void clicked();

protected:
    virtual void mousePressEvent( QGraphicsSceneMouseEvent * event )
    {
        Q_UNUSED( event )
        emit clicked();
    }

    virtual void hoverEnterEvent( QGraphicsSceneHoverEvent * event )
    {
        Q_UNUSED( event )
        m_anim->setDirection( QAbstractAnimation::Forward );
        if ( m_anim->state() != QAbstractAnimation::Running )
        {
            m_anim->start();
        }
    }

    virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent * event )
    {
        Q_UNUSED( event )
        m_anim->setDirection( QAbstractAnimation::Backward );
        if ( m_anim->state() != QAbstractAnimation::Running )
        {
            m_anim->start();
        }
    }

    qreal hoverFadeAmount() const
    {
        return m_highlightFadeAmount;
    }

    void setHoverFadeAmount( qreal amount )
    {
        m_highlightFadeAmount = amount;
        update();
    }

    virtual void paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 )
    {
        Q_UNUSED( option )
        Q_UNUSED( widget )
        int textAreaHeight = m_size.height() * textToTotalHeightRatio;
        int padding = boxPaddingRatio * m_size.width();
        QSize previewSize( m_size.height() - padding * 2, m_size.height() - padding * 2 - textAreaHeight );
        QRect textRect( 0, 0, m_size.width(), textAreaHeight );

        // Draw background/frame
        if ( m_anim->state() == QAbstractAnimation::Running )
        {
            qreal opacity = painter->opacity();
            painter->setOpacity( opacity * ( 1.0 - m_highlightFadeAmount ) );
            painter->drawPixmap( QPointF( 0, 0 ),
                                 Render::renderElement( "bubble", m_size ) );
            painter->setOpacity( opacity * m_highlightFadeAmount );
            painter->drawPixmap( QPointF( 0, 0 ),
                                 Render::renderElement( "bubble_hover", m_size ) );
            painter->setOpacity( opacity );
        }
        else
        {
            painter->drawPixmap( QPointF( 0, 0 ),
                                 Render::renderElement( option->state & QStyle::State_MouseOver ? "bubble_hover" : "bubble",
                                                        m_size ) );
        }

        // Draw game preview
        painter->drawPixmap( ( m_size.width() - previewSize.width() ) / 2,
                             padding + textAreaHeight,
                             Render::renderGamePreview( m_gameId, previewSize ) );

        // Draw label
        painter->setFont( scene()->font() );
        painter->setPen( KColorUtils::mix( Render::colorOfElement( "bubble_text_color" ),
                                           Render::colorOfElement( "bubble_hover_text_color" ), 
                                           m_highlightFadeAmount ) );
        painter->drawText( textRect, Qt::AlignCenter, m_label );
    }

private:
    QString m_label;
    int m_gameId;
    QSize m_size;
    QPropertyAnimation *m_anim;
    qreal m_highlightFadeAmount;
};


GameSelectionScene::GameSelectionScene( QObject * parent )
    : QGraphicsScene( parent )
{
    foreach (const DealerInfo * i, DealerInfoList::self()->games())
    {
        GameSelectionBox * box = new GameSelectionBox( i18n( i->name() ), i->ids().first() );
        m_boxes.append( box );
        addItem( box );

        m_signalMapper.setMapping( box, i->ids().first() );
        connect( box, SIGNAL(clicked()), &m_signalMapper, SLOT(map()) );
    }

    connect( &m_signalMapper, SIGNAL(mapped(int)), SIGNAL(gameSelected(int)) );

    qSort( m_boxes.begin(), m_boxes.end(), GameSelectionBox::lessThan );
}


GameSelectionScene::~GameSelectionScene()
{
}


void GameSelectionScene::resizeScene( const QSize & size )
{
    int numBoxes = m_boxes.size();
    QSizeF boxSize( Render::sizeOfElement( "bubble" ) );
    qreal boxAspect = boxSize.width() / boxSize.height();
    qreal sceneAspect = qreal( size.width() ) / size.height();

    // Determine the optimal number of rows/columns for the grid
    int numRows, numCols, bestNumRows = 1;
    qreal lowestError = 10e10;
    for ( numRows = 1; numRows <= numBoxes; ++numRows )
    {
        numCols = ceil( qreal( numBoxes ) / numRows );
        int numNonEmptyRows = ceil( qreal( numBoxes ) / numCols );
        qreal gridAspect = boxAspect * numCols / numNonEmptyRows;
        qreal error = gridAspect > sceneAspect ? gridAspect / sceneAspect
                                               : sceneAspect / gridAspect;
        if ( error < lowestError )
        {
            lowestError = error;
            bestNumRows = numRows;
        }
    }
    numRows = bestNumRows;
    numCols = ceil( qreal( numBoxes ) / bestNumRows );

    // Calculate the box and grid dimensions
    qreal gridAspect = boxAspect * ( numCols + spacingRatio * ( numCols + 1 ) )
                                  / ( numRows + spacingRatio * ( numRows + 1 ) );
    int boxWidth, boxHeight;
    if ( gridAspect > sceneAspect )
    {
        boxWidth = size.width() / ( numCols + spacingRatio * ( numCols + 1 ) );
        boxHeight = boxWidth / boxAspect;
    }
    else
    {
        boxHeight = size.height() / ( numRows + spacingRatio * ( numRows + 1 ) );
        boxWidth = boxHeight * boxAspect;
    }
    int gridWidth = boxWidth * ( numCols + spacingRatio * ( numCols + 1 ) );
    int gridHeight = boxHeight * ( numRows + spacingRatio * ( numRows + 1 ) );

    // Set up the sceneRect so that the grid is centered
    setSceneRect( ( gridWidth - size.width() ) / 2 - boxWidth * spacingRatio,
                  ( gridHeight - size.height() ) / 2 - boxHeight * spacingRatio,
                  size.width(),
                  size.height() );

    // Initial font size estimate
    QPainter p;
    int maxLabelWidth = boxWidth * ( 1 - 2 * boxPaddingRatio );
    int pixelFontSize = boxHeight * textToTotalHeightRatio - boxWidth * boxPaddingRatio;
    QFont f;
    f.setPixelSize( pixelFontSize );
    p.setFont( f );

    int row = 0;
    int col = 0;
    foreach ( GameSelectionBox * box, m_boxes )
    {
        // Reduce font size until the label fits
        while ( pixelFontSize > 0 &&
                p.boundingRect( QRectF(), box->label() ).width() > maxLabelWidth )
        {
            f.setPixelSize( --pixelFontSize );
            p.setFont( f );
        }

        // Position and size the boxes
        box->setPos( col * ( boxWidth * ( 1 + spacingRatio ) ),
                      row * ( boxHeight * ( 1 + spacingRatio ) ) );
        box->setSize( QSize( boxWidth, boxHeight ) );

        // Increment column and row
        ++col;
        if ( col == numCols )
        {
            col = 0;
            ++row;
        }
    }

    setFont( f );
}

#include "gameselectionscene.moc"
#include "moc_gameselectionscene.cpp"
