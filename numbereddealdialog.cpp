/*
 *  Copyright 2010 Parker Coates <parker.coates@kdemail.net>
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

#include "numbereddealdialog.h"

#include "dealerinfo.h"

#include <KComboBox>
#include <KLocale>

#include <QtCore/QList>
#include <QtGui/QFormLayout>
#include <QtGui/QSpinBox>


NumberedDealDialog::NumberedDealDialog( QWidget * parent )
    : KDialog( parent )
{
    setWindowTitle( i18n( "New Numbered Deal" ) );
    setButtons( KDialog::Ok | KDialog::Cancel );

    QWidget * widget = new QWidget( this );
    QFormLayout * layout = new QFormLayout( widget );
    layout->setFieldGrowthPolicy( QFormLayout::AllNonFixedFieldsGrow );

    m_gameType = new KComboBox( widget );
    layout->addRow( i18n( "Game:" ), m_gameType );

    m_dealNumber = new QSpinBox( widget );
    m_dealNumber->setRange( 1, INT_MAX );
    m_dealNumber->setToolTip( i18n( "Freecell deals are numbered the same as those in the Freecell FAQ." ) );
    layout->addRow( i18n( "Deal number:" ), m_dealNumber );

    setMainWidget( widget );

    QMap<QString,int> nameToIdMap;
    foreach ( DealerInfo * game, DealerInfoList::self()->games() )
    {
        QList<int> ids = game->ids();

        // If a game has more than one ID, ignore the first one as it's the
        // generic ID for all of the games sub-types.
        if ( ids.size() > 1 )
            ids.removeFirst();

        foreach ( int id, ids )
            nameToIdMap.insert( QString( game->name( id ) ), id );
    }

    QMap<QString,int>::const_iterator it = nameToIdMap.constBegin();
    QMap<QString,int>::const_iterator end = nameToIdMap.constEnd();
    for ( ; it != end; ++it )
    {
        // Map combobox indices to game IDs
        m_indexToIdMap[m_gameType->count()] = it.value();
        m_gameType->addItem( it.key() );
    }

    m_gameType->setFocus();
    m_gameType->setMaxVisibleItems( m_indexToIdMap.size() );

    setGameType( m_indexToIdMap[0] );

    connect( this, SIGNAL(okClicked()), this, SLOT(handleOkClicked()) );
}


void NumberedDealDialog::setGameType( int gameId )
{
    int comboIndex = m_indexToIdMap.key( gameId );
    m_gameType->setCurrentIndex( comboIndex );
}


void NumberedDealDialog::setDealNumber( int dealNumber )
{
    m_dealNumber->setValue( dealNumber );
}


void NumberedDealDialog::handleOkClicked()
{
    emit dealChosen( m_indexToIdMap.value( m_gameType->currentIndex() ), m_dealNumber->value() );
}


#include "numbereddealdialog.moc"

