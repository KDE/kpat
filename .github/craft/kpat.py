# SPDX-FileCopyrightText: 2026 KPatience Android port contributors
# SPDX-License-Identifier: BSD-2-Clause
#
# Craft blueprint for KPatience that also covers Android.
#
# Based on kde/kdegames/kpat/kpat.py from craft-blueprints-kde. Copy it over
# that file in a Craft setup (as the GitHub workflow in this repository does)
# to build the Android APK:
#
#   craft --src-dir /path/to/kpat kpat
#   craft --src-dir /path/to/kpat --package kpat

import info
from CraftCore import CraftCore
from Package.CMakePackageBase import CMakePackageBase


class subinfo(info.infoclass):
    def setTargets(self):
        self.versionInfo.setDefaultValues()

        self.description = "KPat (aka KPatience)"

    def setDependencies(self):
        self.runtimeDependencies["virtual/base"] = None
        self.buildDependencies["kde/frameworks/extra-cmake-modules"] = None
        self.runtimeDependencies["libs/libfreecell-solver"] = None
        self.runtimeDependencies["libs/qt/qtbase"] = None
        self.runtimeDependencies["libs/qt/qtsvg"] = None
        self.runtimeDependencies["kde/frameworks/tier2/kcompletion"] = None
        self.runtimeDependencies["kde/frameworks/tier1/kconfig"] = None
        self.runtimeDependencies["kde/frameworks/tier3/kconfigwidgets"] = None
        self.runtimeDependencies["kde/frameworks/tier1/kcoreaddons"] = None
        self.runtimeDependencies["kde/frameworks/tier1/ki18n"] = None
        self.runtimeDependencies["kde/frameworks/tier3/kxmlgui"] = None
        self.runtimeDependencies["kde/frameworks/tier1/kwidgetsaddons"] = None
        self.runtimeDependencies["kde/frameworks/tier1/kguiaddons"] = None
        if CraftCore.compiler.isAndroid:
            # sounds; on the desktop libkdegames plays them
            self.runtimeDependencies["libs/qt/qtmultimedia"] = None
        else:
            self.runtimeDependencies["libs/black-hole-solver"] = None
            self.runtimeDependencies["kde/frameworks/tier2/kcrash"] = None
            self.runtimeDependencies["kde/frameworks/tier1/kdbusaddons"] = None
            self.runtimeDependencies["kde/frameworks/tier2/kdoctools"] = None
            self.runtimeDependencies["kde/frameworks/tier3/kio"] = None
            self.runtimeDependencies["kde/frameworks/tier3/knewstuff"] = None
            self.runtimeDependencies["kde/frameworks/tier3/kiconthemes"] = None
            self.runtimeDependencies["kde/kdegames/libkdegames"] = None
            self.runtimeDependencies["kde/plasma/breeze"] = None


class Package(CMakePackageBase):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.subinfo.options.configure.args += ["-DWITH_BH_SOLVER=OFF"]

    def createPackage(self):
        if not CraftCore.compiler.isAndroid:
            self.blacklist_file.append(self.blueprintDir() / "blacklist.txt")
            self.defines["alias"] = "kpat"
            self.defines["shortcuts"] = [{"name": "Kpat", "target": "bin/kpat.exe", "description": self.subinfo.description}]
        return super().createPackage()
