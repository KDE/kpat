<!--
SPDX-FileCopyrightText: 2026 KPatience Android port contributors
SPDX-License-Identifier: CC0-1.0
-->

# Card decks bundled with the Android build

On desktop systems KPatience uses the card decks installed by libkdegames.
libkdegames is not available on Android, so a small selection of its decks
is bundled here and compiled into the APK as Qt resources (see
`../AndroidData.cmake`).

The decks are unmodified copies of `src/carddecks/<deck>` from
[libkdegames](https://invent.kde.org/games/libkdegames), commit
`4eade5ba1dbdbdd42c373dcd3561304461123a4d`. The copyright and license notes
of each deck (`COPYRIGHT`, `COPYING`, `AUTHORS`) are kept in its directory.

`svg-oxygen-air` must stay, as it is KPatience's default card deck.
