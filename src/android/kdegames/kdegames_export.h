/*
    SPDX-FileCopyrightText: 2026 KPatience Android port contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// The libkdegames classes in this directory are compiled straight into the
// kpat executable, so nothing needs to be exported.

#ifndef KDEGAMES_EXPORT_H
#define KDEGAMES_EXPORT_H

#define KDEGAMES_EXPORT
#define KDEGAMES_NO_EXPORT
#define KDEGAMES_DEPRECATED
#define KDEGAMES_DEPRECATED_EXPORT
#define KDEGAMES_DEPRECATED_VERSION(major, minor, text)
#define KDEGAMES_ENABLE_DEPRECATED_SINCE(major, minor) 0
#define KDEGAMES_BUILD_DEPRECATED_SINCE(major, minor) 0

#endif
