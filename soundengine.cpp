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

#include "soundengine.h"
#include <QStandardPaths>



SoundEngine::SoundEngine( QObject * parent )
  : QObject( parent ),
    m_cardPickedUp( QStandardPaths::locate(QStandardPaths::DataLocation, "sounds/card-pickup.ogg" ) ),
    m_cardPutDown( QStandardPaths::locate(QStandardPaths::DataLocation, "sounds/card-down.ogg" ) )
{
}


SoundEngine::~SoundEngine()
{
}


void SoundEngine::cardsPickedUp()
{
    m_cardPickedUp.start();
}


void SoundEngine::cardsPutDown()
{
    m_cardPutDown.start();
}



