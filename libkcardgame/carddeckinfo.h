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
#ifndef __CARDDECKINFO_H_
#define __CARDDECKINFO_H_

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <libkdegames_export.h>

class KConfigGroup;

/**
 * \headerfile carddeckinfo.h <KCardDeckInfo>
 *
 * Namespace to supply access to card deck information, such as a list of all
 * card decks as well as allowing to access the actual files to render the
 * decks.
 */
namespace CardDeckInfo
{
   /** Retrieve the SVG file belonging to the given card deck (back side). 
    * @param name The untranslated name of the back deck.
    * @return The file name and path to the SVG file or QString() if not available. 
    */
   KDEGAMES_EXPORT QString backSVGFilePath(const QString& name);

   /** Retrieve the SVG file belonging to the given card set (front side). 
    * The SVG IDs used for the card back is '1_club' for Ace of clubs, '10_spade' for
    * 10 of spades, 'queen_heart' for Queen of Hearts, '2_diamond' for 2 of diamonds and
    * so on.
    * @param name The untranslated name of the card set.
    * @return The file name and path to the SVG file or QString() if not available. 
    */
   KDEGAMES_EXPORT QString frontSVGFilePath(const QString& name);

   /** Check whether the card set is SVG or not.
    * @param name The untranslated name of the card set.
    * @return True if SVG data is available.
    */
   KDEGAMES_EXPORT bool isSVGBack(const QString& name);

   /** Check whether the card back deck contains also an SVG file.
    * @param name The untranslated name of the card deck.
    * @return True if SVG data is available.
    */
   KDEGAMES_EXPORT bool isSVGFront(const QString& name);

   /** Retrieve the untranslated name of the default card set (front side).
    * @param pAllowPNG  Allow selection of fixed size cards sets.
    * @return The default card set name.
    */
   KDEGAMES_EXPORT QString defaultFrontName(bool pAllowPNG = true);

   /** Retrieve the untranslated name of the default card deck (back side).
    * @param pAllowPNG  Allow selection of fixed size cards sets.
    * @return The default card deck name.
    */
   KDEGAMES_EXPORT QString defaultBackName(bool pAllowPNG = true);

   /** Retrieve a untranslated name random card set (front side).
    * @param pAllowPNG  Allow selection of fixed size cards sets.
    * @return A radnom card set name.
    */
   KDEGAMES_EXPORT QString randomFrontName(bool pAllowPNG = true);

   /** Retrieve a untranslated name random card deck (back side).
    * @param pAllowPNG  Allow selection of fixed size cards sets.
    * @return A radnom card deck name.
    */
   KDEGAMES_EXPORT QString randomBackName(bool pAllowPNG = true);

   /**
    * Retrieve the directory where the card front sides are stored. The cards are
    * named 1.png, 2.png, etc. For SVG card decks use @ref cardSVGFilePath.
    * @param name The untranslated name of the card set.
    * @return The directory.
    */
   KDEGAMES_EXPORT QString frontDir(const QString& name);

   /**
    * Retrieve the filename of the card back side. 
    * For SVG  decks use @ref deckSVGFilePath.
    * @param name The untranslated name of the card deck.
    * @return The filename.
    */
   KDEGAMES_EXPORT QString backFilename(const QString& name);

   /**
    * retrieve a list of the untranslated names of all installed backsides
    * @returns a list of backside names, which can be 
    * used as input to the other functions.
    */
   KDEGAMES_EXPORT QStringList backNames();

   /**
    * retrieve a list of the untranslated names of all installed frontsides
    * @return a list of frontside names, which can be
    * used as input to the other functions.
    */
   KDEGAMES_EXPORT QStringList frontNames();

   /**
    * retrieve the configured front side untranslated theme name
    * from the @p group
    * @param group the KConfigGroup to read from
    * @param default the default theme to return if the config group has no setting for this
    * @returns the name of the front side theme name
    */
   KDEGAMES_EXPORT QString frontTheme( const KConfigGroup& group, const QString& defaultTheme = defaultFrontName(false) );

   /**
    * retrieve the configured back side untranslated theme name
    * from the @p group
    * @param group the KConfigGroup to read from
    * @param default the default theme to return if the config group has no setting for this
    * @returns the name of the back side theme name
    */
   KDEGAMES_EXPORT QString backTheme( const KConfigGroup& group, const QString& defaultTheme = defaultBackName(false) );

   /**
    * retrieve the current value for the show fixed size
    * card decks from the @p group
    * @param group the KConfigGroup to read from
    * @param allowDefault the default value in case the group has no setting
    * @returns true when fixed size decks should be shown, else false
    */
   KDEGAMES_EXPORT bool allowFixedSizeDecks( const KConfigGroup& group, bool allowDefault = false );

   /**
    * retrieve the current value for the lock front-to-backside
    * option from the @p group
    * @param group the KConfigGroup to read from
    * @param lockDefault the default value in case the group has no setting
    * @returns true when front and backside theme are locked together, else false
    */
   KDEGAMES_EXPORT bool lockFrontToBackside( const KConfigGroup& group, bool lockDefault = true );

   /**
    * store the given frontside @p theme name in the @p group
    * @param group the KConfigGroup to write to from
    * @param theme the theme untranslated name to store
    */
   KDEGAMES_EXPORT void writeFrontTheme( KConfigGroup& group, const QString& theme );

   /**
    * store the given backside @p theme name in the @p group
    * @param group the KConfigGroup to write to from
    * @param theme the theme untranslated name to store
    */
   KDEGAMES_EXPORT void writeBackTheme( KConfigGroup& group, const QString& theme );

   /**
    * store the whether fixed size decks are allowed in the @p group
    * @param group the KConfigGroup to write to from
    * @param allowFixedSize whether fixed size decks are allowed or not
    */
   KDEGAMES_EXPORT void writeAllowFixedSizeDecks( KConfigGroup& group, bool allowFixedSize );

   /**
    * store the whether front and backside theme selection is locked to the @p group
    * @param group the KConfigGroup to write to from
    * @param allowFixedSize whether front and backside theme selection is locked
    */
   KDEGAMES_EXPORT void writeLockFrontToBackside( KConfigGroup& group, bool lock );
}

#endif
