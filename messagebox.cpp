/*
 * Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#include "messagebox.h"

#include "renderer.h"

#include <QFontMetrics>
#include <QPainter>


namespace
{
    const qreal textToBoxSizeRatio = 0.8;
    const int MessageBoxMinimumFontSize = 5;
    const int maximumFontSize = 36;
}


MessageBox::MessageBox()
  : KGameRenderedItem( Renderer::self(), "message_frame" ),
    m_fontCached( false )
{

}


void MessageBox::setMessage( const QString & message )
{
    if ( m_message != message )
    {
        m_message = message;
        m_fontCached = false;
        update();
    }
}


QString MessageBox::message() const
{
    return m_message;
}


void MessageBox::setSize( const QSize & size )
{
    if ( size != renderSize() )
    {
        setRenderSize( size );
        m_fontCached = false;
    }
}


QSize MessageBox::size() const
{
    return renderSize();
}


void MessageBox::paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
    QRect rect = pixmap().rect();

    if ( rect.isEmpty() )
        return;

    KGameRenderedItem::paint( painter, option, widget );

    painter->setFont( m_font );

    if ( !m_fontCached )
    {
        int availableWidth = rect.width() * textToBoxSizeRatio;
        int availableHeight = rect.height() * textToBoxSizeRatio;
        int size = maximumFontSize;

        QFont f = painter->font();
        f.setPointSize( size );
        painter->setFont( f );
        do
        {
            QRect br = painter->boundingRect( rect, Qt::AlignLeft | Qt::AlignTop, m_message );
            if ( br.width() < availableWidth && br.height() < availableHeight )
                break;

            size = qBound( MessageBoxMinimumFontSize,
                           qMin( size * availableWidth / br.width(),
                                 size * availableHeight / br.height() ),
                           size - 1 );

            QFont f = painter->font();
            f.setPointSize( size );
            painter->setFont( f );
        }
        while ( size > MessageBoxMinimumFontSize );

        m_font = painter->font();
        m_fontCached = true;
    }

    painter->setPen( Renderer::self()->colorOfElement( "message_text_color" ) );
    painter->drawText( rect, Qt::AlignCenter, m_message );
}


