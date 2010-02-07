/*
 *  Copyright (C) 2008 Andreas Pakulat <apaku@gmx.de>
 *  Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
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

#ifndef KCARDCACHE_H
#define KCARDCACHE_H

class KCardTheme;

class QDateTime;
class QPixmap;
class QSize;
class QSizeF;
class QString;
class QStringList;

/**
 * \class KCardCache2 cardcache.h <KCardCache2>
 * 
 * This class implements a kdegames wide cache for cards.
 * 
 * Card games such as lskat or kpat should use this cache
 * to load the various decks into QPixmaps instead of inventing
 * their own. It uses KPixmapCache behind the scenes, set up to
 * use disk and memory caching. Thus a SVG card deck that was loaded 
 * by kpat for the size 100x200 doesn't need re-rendering when 
 * requested from lskat.
 * 
 * Usage is quite simple. During initialization of the game the
 * cache object should be created and it should be told to load the
 * currently selected theme.
 * <code>
 * myCache = new KCardCache2();
 * myCache->loadTheme( myTheme );
 * </code>
 * 
 * Later when actually drawing the cards the getter methods can be used to 
 * get the pixmap of a specific card at a specific size from a given theme.
 * <code>
 * myCache->getCard( myTheme, KCardCache2::Club, KCardCache2::Ace, calculatedSize );
 * </code>
 * 
 */
class KCardCache2
{
public:
    /**
     * Constructor creates and initializes a KPixmapCache for all KDE Games 
     * card games
     */
    KCardCache2( const KCardTheme & theme );

    /**
     * Cleans up the cache
     */
    ~KCardCache2();

    /**
     * Set the size of rendered pixmaps.
     *
     * Make sure to set a reasonable size, before fetching pixmaps from the cache.
     *
     * @param size the new size for rendering cards and backsides
     */
    void setSize( const QSize& size );

    /**
     * Returns the currently used size to render cards and backsides.
     *
     * @returns the size of pixmaps for rendering.
     */
    QSize size() const;

    /**
     * Return the currently used frontside theme
     * @returns the name of the frontside theme
     */
    KCardTheme theme() const;


    /**
     * Retrieve the frontside pixmap.
     *
     * The @p infos parameter is used to determine which frontside to load.
     * Make sure to set a reasonable size and theme, before calling this function.
     *
     * @param infos A combination of CardInfo flags to identify what type of card to
     * load. There are of course only certain combinations that make sense, like
     * King | Heart, some flags are used standalone, like Joker.
     *
     * @returns a QPixmap with the card frontside rendered
     *
     * @see setBackTheme
     * @see setSize
     * @see CardInfo
     */
    QPixmap renderCard( const QString & element ) const;

    /**
     * Retrieve the default size for the frontside card.
     *
     * Make sure to set a reasonable theme, before calling this function.
     *
     * @param infos A combination of CardInfo flags to identify what type of card to
     * load. There are of course only certain combinations that make sense, like
     * King | Heart, some flags are used standalone, like Joker.
     *
     * @returns the default size of the selected frontside card
     *
     */
    QSizeF naturalSize( const QString & element ) const;

    void insertOther( const QString & key, const QPixmap & pix );

    bool findOther( const QString & key, QPixmap & pix );

    QDateTime timestamp() const;

    /**
     * Invalidates all cached images in the current size for the current frontside theme
     */
    void invalidateCache();

    /**
     * Loads a whole theme in the background.
     *
     * Depending on the value of @p type only parts may be rendered.
     *
     * @param infos whether to load all entries in the theme or just the front or back
     * sides. Also its possible to specify a different deck, like a 32 card deck.
     */
    void loadInBackground( const QStringList & elements );
private:
    class KCardCache2Private* const d;
};

#endif
