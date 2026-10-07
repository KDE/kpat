/*
    This file is part of the KDE games library
    SPDX-FileCopyrightText: 2001 Andreas Beckermann <b_mann@gmx.de>
    SPDX-FileCopyrightText: 2007 Simon Hürlimann <simon.huerlimann@huerlisi.ch>

    SPDX-License-Identifier: LGPL-2.0-only

    This class was shamelessly stolen from kdelibs/kdeui/kstdction.[cpp|h] and
    after that just edited for our needs
*/

#ifndef KGAMESTANDARDACTION_H
#define KGAMESTANDARDACTION_H

// own
#include "kdegames_export.h"
// KF
#include <KRecentFilesAction>
#include <KToggleAction>
// Qt
#include <QAction>
// std
#include <type_traits>

/**
 * Extension for KStandardAction in KDE Games
 *
 * This class is an extension to the usual KStandardAction class which provides
 * easy access to often used KDE actions.
 *
 * Using these actions helps maintaining consistency among the games.
 *
 * Games often use different menu entries than other programs, e.g. games use
 * the menu "game" instead of "file". This class provides the entries which
 * differ from the usual KStandardAction entries.
 *
 * @see <tt>KStandardAction</tt>
 *
 * @author Andreas Beckermann <b_mann@gmx.de>
 */
namespace KGameStandardAction
{
/**
 * The standard menubar and toolbar actions.
 */
enum GameStandardAction {
    // Game menu
    New = 1,
    Load,
    LoadRecent,
    Save,
    SaveAs,
    End,
    Pause,
    Highscores,
    Statistics,
    Print,
    Quit,
    // Move menu
    Repeat,
    Undo,
    Redo,
    Roll,
    EndTurn,
    // Settings menu
    Carddecks,
    ConfigureHighscores,
    ClearHighscores,
    ClearStatistics,
    Restart,
    Hint,
    Demo,
    Solve,
    ActionNone
};

/**
 * Creates an action corresponding to the
 * KStandardAction::StandardAction enum.
 */
KDEGAMES_EXPORT QAction *create(GameStandardAction id, const QObject *recvr, const char *slot, QObject *parent);

/**
 * @internal
 */
KDEGAMES_EXPORT QAction *_k_createInternal(GameStandardAction id, QObject *parent);

/**
 * This overloads create() to allow using the new connect syntax.
 *
 * @note If you use @c LoadRecent as @p id, you should manually connect to the urlSelected(const QUrl &)
 * signal of the returned KRecentFilesAction instead or use KGameStandardAction::loadRecent(Receiver *, Func, QObject*).
 *
 * @see create(GameStandardAction, const QObject *, const char *, QObject *)
 * @since 7.3
 */
#ifdef K_DOXYGEN
inline QAction *create(GameStandardAction id, const QObject *recvr, Func slot, QObject *parent)
#else
template<class Receiver, class Func>
inline typename std::enable_if<!std::is_convertible<Func, const char *>::value, QAction>::type *
create(GameStandardAction id, const Receiver *recvr, Func slot, QObject *parent)
#endif
{
    QAction *action = _k_createInternal(id, parent);
    QObject::connect(action, &QAction::triggered, recvr, slot);
    return action;
}

/**
 * This will return the internal name of a given standard action.
 */
KDEGAMES_EXPORT QString name(GameStandardAction id);

// we have to disable the templated function for const char* as Func, since it is ambiguous otherwise
// TODO: KF6: unify const char* version and new style by removing std::enable_if
#ifdef K_DOXYGEN
#define KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(name, enumValue) inline QAction *name(const QObject *recvr, Func slot, QObject *parent);
#define KSTANDARDGAMETOGGLEACTION_WITH_NEW_STYLE_CONNECT(name, enumValue) inline KToggleAction *name(const QObject *recvr, Func slot, QObject *parent);
#else
#define KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(name, enumValue)                                                                                            \
    template<class Receiver, class Func>                                                                                                                       \
    inline typename std::enable_if<!std::is_convertible<Func, const char *>::value, QAction>::type *name(const Receiver *recvr, Func slot, QObject *parent)    \
    {                                                                                                                                                          \
        return create(enumValue, recvr, slot, parent);                                                                                                         \
    }
#define KSTANDARDGAMETOGGLEACTION_WITH_NEW_STYLE_CONNECT(name, enumValue)                                                                                      \
    template<class Receiver, class Func>                                                                                                                       \
    inline typename std::enable_if<!std::is_convertible<Func, const char *>::value, KToggleAction>::type *name(const Receiver *recvr,                          \
                                                                                                               Func slot,                                      \
                                                                                                               QObject *parent)                                \
    {                                                                                                                                                          \
        QAction *ret = create(enumValue, recvr, slot, parent);                                                                                                 \
        Q_ASSERT(qobject_cast<KToggleAction *>(ret));                                                                                                          \
        return static_cast<KToggleAction *>(ret);                                                                                                              \
    }
#endif

/**
 * Start a new game.
 */
KDEGAMES_EXPORT QAction *gameNew(const QObject *recvr, const char *slot, QObject *parent);
/**
 * Start a new game.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(gameNew, New)

/**
 * Load a previously saved game.
 */
KDEGAMES_EXPORT QAction *load(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Load a previously saved game.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(load, Load)

// FIXME why not to delete this and use just KStandardAction::openRecent???
// loadRecent seems to mimic its behaviour
/**
 * Load a recently loaded game.
 * The signature of slot is of the form slotURLSelected(const QUrl&)
 */
KDEGAMES_EXPORT KRecentFilesAction *loadRecent(const QObject *recvr, const char *slot, QObject *parent);
/**
 * Load a recently loaded game.
 * @since 7.3
 */
#ifdef K_DOXYGEN
inline KRecentFilesAction *loadRecent(const QObject *recvr, Func slot, QObject *parent)
#else
template<class Receiver, class Func>
inline typename std::enable_if<!std::is_convertible<Func, const char *>::value, KRecentFilesAction>::type *
loadRecent(const Receiver *recvr, Func slot, QObject *parent)
#endif
{
    QAction *action = _k_createInternal(LoadRecent, parent);
    KRecentFilesAction *recentAction = qobject_cast<KRecentFilesAction *>(action);
    Q_ASSERT(recentAction);
    QObject::connect(recentAction, &KRecentFilesAction::urlSelected, recvr, slot);
    return recentAction;
}

/**
 * Save the current game.
 */
KDEGAMES_EXPORT QAction *save(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Save the current game.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(save, Save)

/**
 * Save the current game under a different filename.
 */
KDEGAMES_EXPORT QAction *saveAs(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Save the current game under a different filename.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(saveAs, SaveAs)

/**
 * Pause the game.
 */
KDEGAMES_EXPORT KToggleAction *pause(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Pause the game.
 * @since 7.3
 */
KSTANDARDGAMETOGGLEACTION_WITH_NEW_STYLE_CONNECT(pause, Pause)

/**
 * Show the highscores.
 */
KDEGAMES_EXPORT QAction *highscores(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Show the highscores.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(highscores, Highscores)

/**
 * Clear highscores.
 */
KDEGAMES_EXPORT QAction *clearHighscores(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Clear highscores.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(clearHighscores, ClearHighscores)

/**
 * Show the statistics.
 */
KDEGAMES_EXPORT QAction *statistics(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Show the statistics.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(statistics, Statistics)

/**
 * Clear statistics.
 */
KDEGAMES_EXPORT QAction *clearStatistics(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Clear statistics.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(clearStatistics, ClearStatistics)

/**
 * End the current game, but do not quit the program.
 * Think of a "close" entry.
 */
KDEGAMES_EXPORT QAction *end(const QObject *recvr, const char *slot, QObject *parent);

/**
 * End the current game, but do not quit the program.
 * Think of a "close" entry.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(end, End)

/**
 * Print current game.
 * Not useful in all games.
 */
KDEGAMES_EXPORT QAction *print(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Print current game.
 * Not useful in all games.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(print, Print)

/**
 * Quit the game.
 */
KDEGAMES_EXPORT QAction *quit(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Quit the game.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(quit, Quit)

/**
 * Repeat the last move.
 */
KDEGAMES_EXPORT QAction *repeat(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Repeat the last move.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(repeat, Repeat)

/**
 * Undo the last move.
 */
KDEGAMES_EXPORT QAction *undo(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Undo the last move.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(undo, Undo)

/**
 * Redo the last move (which has been undone).
 */
KDEGAMES_EXPORT QAction *redo(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Redo the last move (which has been undone).
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(redo, Redo)

/**
 * Roll die or dice.
 */
KDEGAMES_EXPORT QAction *roll(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Roll die or dice.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(roll, Roll)

/**
 * End the current turn (not the game).
 * Usually to let the next player start.
 */
KDEGAMES_EXPORT QAction *endTurn(const QObject *recvr, const char *slot, QObject *parent);

/**
 * End the current turn (not the game).
 * Usually to let the next player start.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(endTurn, EndTurn)

/**
 * Display configure carddecks dialog.
 */
KDEGAMES_EXPORT QAction *carddecks(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Display configure carddecks dialog.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(carddecks, Carddecks)

/**
 * Display configure highscores dialog.
 */
KDEGAMES_EXPORT QAction *configureHighscores(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Display configure highscores dialog.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(configureHighscores, ConfigureHighscores)

/**
 * Give an advice/hint.
 */
KDEGAMES_EXPORT QAction *hint(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Give an advice/hint.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(hint, Hint)

/**
 * Show a demo.
 */
KDEGAMES_EXPORT KToggleAction *demo(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Show a demo.
 * @since 7.3
 */
KSTANDARDGAMETOGGLEACTION_WITH_NEW_STYLE_CONNECT(demo, Demo)

/**
 * Solve the game.
 */
KDEGAMES_EXPORT QAction *solve(const QObject *recvr, const char *slot, QObject *parent);

/**
 * Solve the game.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(solve, Solve)

/**
 * Restart the game.
 */
KDEGAMES_EXPORT QAction *restart(const QObject *recvr, const char *slot, QObject *parent);
/**
 * Restart the game.
 * @since 7.3
 */
KSTANDARDGAMEACTION_WITH_NEW_STYLE_CONNECT(restart, Restart)
}

#endif
