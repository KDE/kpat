/*
 * Copyright (C) 2001-2009 Stephan Kulow <coolo@kde.org>
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

#ifndef SPEEDS_H
#define SPEEDS_H

#include <QtGlobal>


const qreal SPEED_FACTOR = 1;

const qreal DEAL_SPEED = 4 / SPEED_FACTOR;

const int DURATION_DEAL        = SPEED_FACTOR * 1500;
const int DURATION_WON         = SPEED_FACTOR * 2000;
const int DURATION_DEMO        = SPEED_FACTOR * 300;
const int DURATION_MOVE        = SPEED_FACTOR * 230;
const int DURATION_RELAYOUT    = SPEED_FACTOR * 120;
const int DURATION_FANCYSHOW   = SPEED_FACTOR * 350;

const int DURATION_AUTODROP         = SPEED_FACTOR * 250;
const int DURATION_AUTODROP_MINIMUM = SPEED_FACTOR * 80;
const qreal AUTODROP_SPEEDUP_FACTOR = 0.90;

const int TIME_BETWEEN_MOVES =  SPEED_FACTOR * 250;

#endif
