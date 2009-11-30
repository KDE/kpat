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

#define SPEED_FACTOR 1

#define DURATION_DEAL        SPEED_FACTOR * 1500
#define DURATION_WON         SPEED_FACTOR * 900
#define DURATION_DEMO        SPEED_FACTOR * 300
#define DURATION_MOVEBACK    SPEED_FACTOR * 230
#define DURATION_INITIALDEAL SPEED_FACTOR * 500
#define DURATION_FLIP        SPEED_FACTOR * 180
#define DURATION_RELAYOUT    SPEED_FACTOR * 120
#define DURATION_FANCYSHOW   SPEED_FACTOR * 350

#define DURATION_AUTODROP         SPEED_FACTOR * 250
#define DURATION_AUTODROP_MINIMUM SPEED_FACTOR * 80
#define AUTODROP_SPEEDUP_FACTOR   0.90

#define TIME_BETWEEN_MOVES   SPEED_FACTOR * 200

#define DURATION_HOVERFADE     300
#define DURATION_CARDHIGHLIGHT 150

#endif // SPEEDS_H
