/*
    This file is part of the KDE games library
    SPDX-FileCopyrightText: 2001 Andreas Beckermann <b_mann@gmx.de>
    SPDX-FileCopyrightText: 2007 Simon Hürlimann <simon.huerlimann@huerlisi.ch>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kgamestandardaction.h"

// KF
#include <KActionCollection>
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KStandardShortcut>
// Qt
#include <QIcon>
// Std
#include <string>

// Helper class for storing raw data in static tables which can be used for QString instance
// creation at runtime without copying/converting to new memalloc'ed memory, as well as avoiding
// that way storing the strings directly as QStrings resulting in non-constexpr init code on
// library loading
struct RawStringData {
    template<std::size_t StringSize>
    constexpr inline RawStringData(const char16_t (&_data)[StringSize])
        : data(_data)
        , size(std::char_traits<char16_t>::length(_data))
    {
    }
    constexpr inline RawStringData(std::nullptr_t)
    {
    }
    constexpr inline RawStringData() = default;

    inline QString toString() const
    {
        if (!data) {
            return QString();
        }

        return Qt::Literals::StringLiterals::operator""_s(data, size);
    }

private:
    const char16_t *const data = nullptr;
    const std::size_t size = 0;
};

struct KGameStandardActionInfo {
    KGameStandardAction::GameStandardAction id;
    KStandardShortcut::StandardShortcut globalAccel; // if we reuse a global accel
    int shortcut; // specific shortcut (NH: should be configurable)
    const RawStringData psName;
    const KLazyLocalizedString psLabel;
    const KLazyLocalizedString psWhatsThis;
    const RawStringData psIconName;
    const KLazyLocalizedString psToolTip;
};

#define CTRL(x) QKeyCombination(Qt::CTRL | Qt::Key_##x).toCombined()

const KGameStandardActionInfo g_rgActionInfo[] = {
    // clang-format off
    // "game" menu
    { KGameStandardAction::New,             KStandardShortcut::New,       0,                    u"game_new",
      kli18nc("new game", "&New"),  kli18n("Start a new game."), u"document-new",        kli18n("Start a new game") },
    { KGameStandardAction::Load,            KStandardShortcut::Open,      0,                    u"game_load",
      kli18n("&Load…"),             {},                         u"document-open",        kli18n("Open a saved game") },
    { KGameStandardAction::LoadRecent,      KStandardShortcut::AccelNone, 0,                    u"game_load_recent",
      kli18n("Load &Recent"),       {},                         nullptr,                 kli18n("Open a recently saved game") },
    { KGameStandardAction::Restart,         KStandardShortcut::Reload,    0,                    u"game_restart",
      kli18n("Restart &Game"),      {},                         u"view-refresh",         kli18n("Restart the game") },
    { KGameStandardAction::Save,            KStandardShortcut::Save,      0,                    u"game_save",
      kli18n("&Save"),              {},                         u"document-save",        kli18n("Save the current game") },
    { KGameStandardAction::SaveAs,          KStandardShortcut::AccelNone, 0,                    u"game_save_as",
      kli18n("Save &As…"),          {},                         u"document-save-as",     kli18n("Save the current game to another file") },
    { KGameStandardAction::End,             KStandardShortcut::End,       0,                    u"game_end",
      kli18n("&End Game"),          {},                         u"window-close",         kli18n("End the current game") },
    { KGameStandardAction::Pause,           KStandardShortcut::AccelNone, Qt::Key_P,            u"game_pause",
      kli18n("Pa&use"),             {},                         u"media-playback-pause", kli18n("Pause the game") },
    { KGameStandardAction::Highscores,      KStandardShortcut::AccelNone, CTRL(H),              u"game_highscores",
      kli18n("Show &High Scores"),  {},                         u"games-highscores",     kli18n("Show high scores") },
    { KGameStandardAction::ClearHighscores, KStandardShortcut::AccelNone, 0,                    u"game_clear_highscores",
      kli18n("&Clear High Scores"), {},                         u"clear_highscore",      kli18n("Clear high scores") },
    { KGameStandardAction::Statistics,      KStandardShortcut::AccelNone, 0,                    u"game_statistics",
      kli18n("Show Statistics"),    {},                         u"highscore",            kli18n("Show statistics") },
    { KGameStandardAction::ClearStatistics, KStandardShortcut::AccelNone, 0,                    u"game_clear_statistics",
      kli18n("&Clear Statistics"),  {},                         u"flag",                 kli18n("Delete all-time statistics.") },
    { KGameStandardAction::Print,           KStandardShortcut::Print,     0,                    u"game_print",
      kli18n("&Print…"),            {},                         u"document-print",       {} },
    { KGameStandardAction::Quit,            KStandardShortcut::Quit,      0,                    u"game_quit",
      kli18n("&Quit"),              {},                         u"application-exit",     kli18n("Quit the program") },

    // "move" menu
    { KGameStandardAction::Repeat,  KStandardShortcut::AccelNone, 0,                    u"move_repeat",
      kli18n("Repeat"),     {}, nullptr,                kli18n("Repeat the last move") },
    { KGameStandardAction::Undo,    KStandardShortcut::Undo,      0,                    u"move_undo",
      kli18n("Und&o"),      {}, u"edit-undo",            kli18n("Undo the last move") },
    { KGameStandardAction::Redo,    KStandardShortcut::Redo,      0,                    u"move_redo",
      kli18n("Re&do"),      {}, u"edit-redo",            kli18n("Redo the latest move") },
    { KGameStandardAction::Roll,    KStandardShortcut::AccelNone, CTRL(R),              u"move_roll",
      kli18n("&Roll Dice"), {}, u"roll",                 kli18n("Roll the dice") },
    { KGameStandardAction::EndTurn, KStandardShortcut::AccelNone, 0,                    u"move_end_turn",
      kli18n("End Turn"),   {}, u"games-endturn",        {}  },
    { KGameStandardAction::Hint,    KStandardShortcut::AccelNone, Qt::Key_H,            u"move_hint",
      kli18n("&Hint"),      {}, u"games-hint",           kli18n("Give a hint") },
    { KGameStandardAction::Demo,    KStandardShortcut::AccelNone, Qt::Key_D,            u"move_demo",
      kli18n("&Demo"),      {}, u"media-playback-start", kli18n("Play a demo") },
    { KGameStandardAction::Solve,   KStandardShortcut::AccelNone, 0,                    u"move_solve",
      kli18n("&Solve"),     {}, u"games-solve",          kli18n("Solve the game") },

    // "settings" menu
    { KGameStandardAction::Carddecks,           KStandardShortcut::AccelNone, 0, u"options_configure_carddecks",
      kli18n("Configure &Carddecks…"),   {}, nullptr, {} },
    { KGameStandardAction::ConfigureHighscores, KStandardShortcut::AccelNone, 0, u"options_configure_highscores",
      kli18n("Configure &High Scores…"), {}, nullptr, {} },

    { KGameStandardAction::ActionNone, KStandardShortcut::AccelNone, 0, nullptr, {}, {}, nullptr, {} }
    // clang-format on
};

static const KGameStandardActionInfo *infoPtr(KGameStandardAction::GameStandardAction id)
{
    for (uint i = 0; g_rgActionInfo[i].id != KGameStandardAction::ActionNone; i++) {
        if (g_rgActionInfo[i].id == id)
            return &g_rgActionInfo[i];
    }
    return nullptr;
}

QAction *KGameStandardAction::_k_createInternal(KGameStandardAction::GameStandardAction id, QObject *parent)
{
    QAction *pAction = nullptr;
    const KGameStandardActionInfo *pInfo = infoPtr(id);
    // qCDebug(GAMES_UI) << "KGameStandardAction::create( " << id << "=" << (pInfo ? pInfo->psName : (const char*)nullptr) << "," << parent << " )";
    if (pInfo) {
        const QString sLabel = pInfo->psLabel.toString();
        switch (id) {
        case LoadRecent:
            pAction = new KRecentFilesAction(sLabel, parent);
            break;
        case Pause:
        case Demo:
            pAction = new KToggleAction(QIcon::fromTheme(pInfo->psIconName.toString()), sLabel, parent);
            break;
        default:
            pAction = new QAction(QIcon::fromTheme(pInfo->psIconName.toString()), sLabel, parent);
            break;
        }

        // clang-format off
        const QList<QKeySequence> cut =
            (pInfo->globalAccel != KStandardShortcut::AccelNone) ? KStandardShortcut::shortcut(pInfo->globalAccel) :
            (pInfo->shortcut != 0)                               ? QList<QKeySequence>{QKeySequence(pInfo->shortcut)} :
            /* else */ QList<QKeySequence>();
        // clang-format on

        if (!cut.isEmpty()) {
            pAction->setShortcuts(cut);
            pAction->setProperty("defaultShortcuts", QVariant::fromValue(cut));
        }
        if (!pInfo->psToolTip.isEmpty())
            pAction->setToolTip(pInfo->psToolTip.toString());
        if (!pInfo->psWhatsThis.isEmpty())
            pAction->setWhatsThis(pInfo->psWhatsThis.toString());
        else if (!pInfo->psToolTip.isEmpty())
            pAction->setWhatsThis(pInfo->psToolTip.toString());

        pAction->setObjectName(pInfo->psName.toString());
    }

    KActionCollection *collection = qobject_cast<KActionCollection *>(parent);
    if (collection && pAction)
        collection->addAction(pAction->objectName(), pAction);

    return pAction;
}

QAction *KGameStandardAction::create(GameStandardAction id, const QObject *recvr, const char *slot, QObject *parent)
{
    QAction *pAction = _k_createInternal(id, parent);
    if (recvr && slot) {
        switch (id) {
        case LoadRecent:
            QObject::connect(pAction, SIGNAL(urlSelected(QUrl)), recvr, slot);
            break;
        default:
            QObject::connect(pAction, SIGNAL(triggered(bool)), recvr, slot);
            break;
        }
    }
    return pAction;
}

QString KGameStandardAction::name(GameStandardAction id)
{
    const KGameStandardActionInfo *pInfo = infoPtr(id);
    return (pInfo) ? pInfo->psName.toString() : QString();
}

QAction *KGameStandardAction::gameNew(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(New, recvr, slot, parent);
}
QAction *KGameStandardAction::load(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Load, recvr, slot, parent);
}
KRecentFilesAction *KGameStandardAction::loadRecent(const QObject *recvr, const char *slot, QObject *parent)
{
    return static_cast<KRecentFilesAction *>(KGameStandardAction::create(LoadRecent, recvr, slot, parent));
}
QAction *KGameStandardAction::save(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Save, recvr, slot, parent);
}
QAction *KGameStandardAction::saveAs(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(SaveAs, recvr, slot, parent);
}
QAction *KGameStandardAction::end(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(End, recvr, slot, parent);
}
KToggleAction *KGameStandardAction::pause(const QObject *recvr, const char *slot, QObject *parent)
{
    return static_cast<KToggleAction *>(KGameStandardAction::create(Pause, recvr, slot, parent));
}
QAction *KGameStandardAction::highscores(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Highscores, recvr, slot, parent);
}
QAction *KGameStandardAction::statistics(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Highscores, recvr, slot, parent);
}
QAction *KGameStandardAction::clearStatistics(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(ClearStatistics, recvr, slot, parent);
}
QAction *KGameStandardAction::print(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Print, recvr, slot, parent);
}
QAction *KGameStandardAction::quit(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Quit, recvr, slot, parent);
}

QAction *KGameStandardAction::repeat(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Repeat, recvr, slot, parent);
}
QAction *KGameStandardAction::undo(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Undo, recvr, slot, parent);
}

QAction *KGameStandardAction::redo(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Redo, recvr, slot, parent);
}

QAction *KGameStandardAction::roll(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Roll, recvr, slot, parent);
}
QAction *KGameStandardAction::endTurn(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(EndTurn, recvr, slot, parent);
}

QAction *KGameStandardAction::carddecks(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Carddecks, recvr, slot, parent);
}
QAction *KGameStandardAction::configureHighscores(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(ConfigureHighscores, recvr, slot, parent);
}
QAction *KGameStandardAction::hint(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Hint, recvr, slot, parent);
}
KToggleAction *KGameStandardAction::demo(const QObject *recvr, const char *slot, QObject *parent)
{
    return static_cast<KToggleAction *>(KGameStandardAction::create(Demo, recvr, slot, parent));
}
QAction *KGameStandardAction::solve(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Solve, recvr, slot, parent);
}
QAction *KGameStandardAction::restart(const QObject *recvr, const char *slot, QObject *parent)
{
    return KGameStandardAction::create(Restart, recvr, slot, parent);
}
