cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "21")
set(RELEASE_SERVICE_VERSION_MINOR "08")
set(RELEASE_SERVICE_VERSION_MICRO "2")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kpat VERSION ${RELEASE_SERVICE_VERSION})

set (QT_MIN_VERSION "5.12.0")
set (KF5_MIN_VERSION "5.64.0")

include(FindPkgConfig)
pkg_check_modules(FC_SOLVE REQUIRED libfreecell-solver)
option(WITH_BH_SOLVER "Make use of https://github.com/shlomif/black-hole-solitaire for solving Golf" ON)
if (WITH_BH_SOLVER)
    pkg_check_modules(BLACK_HOLE_SOLVER REQUIRED libblack-hole-solver)
endif()
try_compile(fcs_soft_suspend "${CMAKE_CURRENT_BINARY_DIR}" SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fcs_soft_suspend_test.c"
    COMPILE_DEFINITIONS ${FC_SOLVE_INCLUDE_DIRS}
    LINK_LIBRARIES ${FC_SOLVE_LDFLAGS})

find_package(ECM ${KF5_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Widgets Svg Test)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
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

find_package(KF5KDEGames 7.3.0 REQUIRED)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(GenerateExportHeader)

add_definitions(
    -DQT_NO_KEYWORDS
    -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_STRICT_ITERATORS
    -DQT_USE_QSTRINGBUILDER
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
    -DQT_NO_FOREACH
    -DQT_DISABLE_DEPRECATED_BEFORE=0x050e00
    -DQT_DEPRECATED_WARNINGS_SINCE=0x060000
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
