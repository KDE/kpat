<!--
SPDX-FileCopyrightText: 2026 KPatience Android port contributors
SPDX-License-Identifier: CC0-1.0
-->

# KPatience for Android

KPatience builds for Android from the same sources as the desktop version.
The game engine, all game types and the solvers are unchanged. The Android
build differs from the desktop build only where a desktop dependency is not
available on Android:

| Desktop | Android |
|---|---|
| libkdegames (theme rendering, sounds, standard actions) | The few classes KPatience uses are built into the app from `src/android/kdegames` (copied from libkdegames, with the QML and KNewStuff parts removed). Sounds are played through Qt Multimedia. |
| Data files installed to `share/` (themes, previews, sounds) | Compiled into the app as Qt resources below `:/share/`, see `src/android/AndroidData.cmake`. |
| Card decks from libkdegames | A selection of decks is bundled from `src/android/carddecks`. |
| Breeze icon theme | The icons KPatience shows are bundled from `src/android/icons`. |
| KIO (loading/saving remote files) | Files are read and written with `QFile`, which also handles Android `content://` URIs. |
| KNewStuff (downloading decks and themes) | Not available. |
| KCrash, KDBusAddons, KDocTools | Not used. "Help with Current Game" opens the online handbook. |
| Black Hole Solver (Golf) | Not built, as in KDE's other Craft builds. Golf is still playable, without the solver. |

On top of that, the Android version:

* saves the current game whenever the app goes to the background, because
  Android may close a background app without warning (when "Remember State
  on Exit" is enabled, which is the default);
* uses the Android back button to go from a game back to the game selection,
  and to quit from there.

## Building

The APK is built with [KDE Craft](https://community.kde.org/Craft), inside
KDE's Android CI image, which provides the Android SDK and NDK as well as
the host tools. `.github/workflows/android.yml` does this on every push and
uploads the APK as the `kpat-android-arm64` artifact.

To build locally:

```sh
docker run -ti --rm -v "$PWD":/kpat \
    invent-registry.kde.org/sysadmin/ci-images/android-qt611 bash

# inside the container
python3 -c "$(curl -fsSL https://raw.githubusercontent.com/KDE/craft/master/setup/CraftBootstrap.py)" \
    --prefix ~/CraftRoot --use-defaults
cp /kpat/.github/craft/kpat.py \
    ~/CraftRoot/etc/blueprints/locations/craft-blueprints-kde/kde/kdegames/kpat/kpat.py
source ~/CraftRoot/craft/craftenv.sh
craft --src-dir /kpat kpat
craft --src-dir /kpat --package kpat
```

`.github/craft/kpat.py` is the Craft blueprint of KPatience with the
desktop-only dependencies left out on Android. The APK ends up in
`~/CraftRoot/tmp`. It is unsigned (or debug-signed); sign it with
`apksigner` before distributing it.
