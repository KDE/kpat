/*
 * Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
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

#include "soundengine.h"

#include <KStandardDirs>


SoundEngine::SoundEngine( QObject * parent )
  : QObject( parent )
{
    m_mediaObject = Phonon::createPlayer( Phonon::GameCategory );
    m_mediaObject->setParent( this );

    QString path = KStandardDirs::locate( "appdata", "sounds/card-pickup.ogg" );
    m_cardPickedUp = Phonon::MediaSource( path );

    path = KStandardDirs::locate( "appdata", "sounds/card-down.ogg" );
    m_cardPutDown = Phonon::MediaSource( path );
}


SoundEngine::~SoundEngine()
{
}


void SoundEngine::cardsPickedUp()
{
    m_mediaObject->setCurrentSource( m_cardPickedUp );
    m_mediaObject->play();
}


void SoundEngine::cardsPutDown()
{
    m_mediaObject->setCurrentSource( m_cardPutDown );
    m_mediaObject->play();
}


#include "soundengine.moc"
