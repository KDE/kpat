cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "23")
set(RELEASE_SERVICE_VERSION_MINOR "08")
set(RELEASE_SERVICE_VERSION_MICRO "4")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kpat VERSION ${RELEASE_SERVICE_VERSION})

set (QT_MIN_VERSION "5.15.2")
set (KF_MIN_VERSION "5.91.0")

include(FindPkgConfig)
pkg_check_modules(FC_SOLVE REQUIRED libfreecell-solver)
option(WITH_BH_SOLVER "Make use of https://github.com/shlomif/black-hole-solitaire for solving Golf" ON)
if (WITH_BH_SOLVER)
    pkg_check_modules(BLACK_HOLE_SOLVER REQUIRED libblack-hole-solver)
endif()
try_compile(fcs_soft_suspend "${CMAKE_CURRENT_BINARY_DIR}" SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fcs_soft_suspend_test.c"
    COMPILE_DEFINITIONS ${FC_SOLVE_INCLUDE_DIRS}
    LINK_LIBRARIES ${FC_SOLVE_LDFLAGS})

find_package(ECM ${KF_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMDeprecationSettings)
if (QT_MAJOR_VERSION STREQUAL "6")
    set(QT_REQUIRED_VERSION "6.4.0")
    set(KF_MIN_VERSION "5.240.0")
    set(KF_MAJOR_VERSION "6")
else()
    set(KF_MAJOR_VERSION "5")
endif()
include(GenerateExportHeader)
include(KDEGitCommitHooks)
include(KDEClangFormat)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Widgets Svg Test Xml)
find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS
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

if (QT_MAJOR_VERSION STREQUAL "6")
    find_package(KDEGames6 7.5.0 REQUIRED)
else()
    find_package(KF5KDEGames 7.3.1 REQUIRED)
endif()

ecm_set_disabled_deprecation_versions(
    QT 6.4
    KF 5.101.0
    KDEGAMES 7.3
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
