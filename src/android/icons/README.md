<!--
SPDX-FileCopyrightText: 2026 KPatience Android port contributors
SPDX-License-Identifier: CC0-1.0
-->

# Icons bundled with the Android build

The Breeze icon theme is not available on Android, so the icons KPatience
shows in its toolbar and menus are bundled here and compiled into the APK as
Qt resources (see `../AndroidData.cmake`).

The SVG files are copies of `icons/actions/22/<name>.svg` from
[breeze-icons](https://invent.kde.org/frameworks/breeze-icons), commit
`621e0aa0d16def210d355a6c48583de550df1fe6`, with symbolic links resolved.
Breeze icons are Copyright (C) 2014 Uri Herrera and others, licensed under
LGPL-3.0-or-later.

When adding an action with a new icon to KPatience, add the icon here too.
