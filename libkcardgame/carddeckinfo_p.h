/*
    This file is part of the KDE games library
    Copyright 2008 Andreas Pakulat <apaku@gmx.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef __CARDDECKINFO_P_H_
#define __CARDDECKINFO_P_H_

#include <QMap>
#include <QString>
#include <QPixmap>
#include <QSize>


/**
 * Stores the information for one card front or back side.
 */
class KCardThemeInfo
{
  public:
   /** The translated name.
    */
   QString name;

   /** The untranslated name.
    */
   QString noi18Name;

   /** The comment (author and description).
    */
   QString comment;

   /** The full path information.
    */
   QString path;

   /** The translated name of the back side.
    */
   QString back;

   /** The preview image.
    */
   QPixmap preview;

   /** The full filename of the SVG file.
    */
   QString svgfile;

   /** Is this a default deck or set.
   */
   bool isDefault;
};

QDebug operator<<(QDebug debug, const KCardThemeInfo &cn);

namespace CardDeckInfo
{

KCardThemeInfo frontInfo( const QString& name );
KCardThemeInfo backInfo( const QString& name );

}

#endif
