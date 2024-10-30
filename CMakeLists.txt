cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "24")
set(RELEASE_SERVICE_VERSION_MINOR "08")
set(RELEASE_SERVICE_VERSION_MICRO "3")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kpat VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "6.0.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(KDEGitCommitHooks)
include(KDEClangFormat)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMDeprecationSettings)
include(FindPkgConfig)
include(FeatureSummary)
include(GenerateExportHeader)

include(InternalMacros)

option(WITH_BH_SOLVER "Make use of https://github.com/shlomif/black-hole-solitaire for solving Golf" ON)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Widgets
    Svg
    Test
    Xml
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Completion
    Config
    ConfigWidgets
    CoreAddons
    Crash
    DBusAddons
    DocTools
    GuiAddons
    I18n
    KIO
    NewStuff
    WidgetsAddons
    XmlGui
)

find_package(KDEGames6 6.0.0 REQUIRED)

find_package(FreecellSolver)
set_package_properties(FreecellSolver PROPERTIES TYPE REQUIRED)

if (WITH_BH_SOLVER)
    find_package(BlackHoleSolver)
    set_package_properties(BlackHoleSolver PROPERTIES TYPE REQUIRED)
endif()

ecm_set_disabled_deprecation_versions(
    QT 6.5
    KF 6.0
    KDEGAMES 6.0
)

add_subdirectory(icons)
add_subdirectory(mimetypes)
add_subdirectory(previews)
add_subdirectory(sounds)
add_subdirectory(themes)
add_subdirectory(doc)
add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()

ki18n_install(po)
kdoctools_install(po)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
