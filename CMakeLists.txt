cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "20")
set(RELEASE_SERVICE_VERSION_MINOR "08")
set(RELEASE_SERVICE_VERSION_MICRO "2")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kpat VERSION ${RELEASE_SERVICE_VERSION})

set (QT_MIN_VERSION "5.12.0")
set (KF5_MIN_VERSION "5.59.0")

include(FindPkgConfig)
pkg_check_modules(FC_SOLVE REQUIRED libfreecell-solver)
option(WITH_BH_SOLVER "Make use of https://github.com/shlomif/black-hole-solitaire for solving Golf")
if (WITH_BH_SOLVER)
    pkg_check_modules(BLACK_HOLE_SOLVER REQUIRED libblack-hole-solver)
    add_definitions(-DWITH_BH_SOLVER)
endif()
try_compile(fcs_soft_suspend "${CMAKE_CURRENT_BINARY_DIR}" SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/fcs_soft_suspend_test.c"
    COMPILE_DEFINITIONS ${FC_SOLVE_INCLUDE_DIRS}
    LINK_LIBRARIES ${FC_SOLVE_LDFLAGS})

if (fcs_soft_suspend)
    add_definitions("-DWITH_FCS_SOFT_SUSPEND=1")
endif()
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

find_package(KF5KDEGames 4.9.0 REQUIRED)

include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)

ecm_setup_version(${RELEASE_SERVICE_VERSION} VARIABLE_PREFIX KPAT VERSION_HEADER kpat_version.h)

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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libkcardgame/include ${FC_SOLVE_INCLUDE_DIRS} ${BLACK_HOLE_SOLVER_INCLUDE_DIRS})

add_subdirectory(icons)
add_subdirectory(libkcardgame)
add_subdirectory(mimetypes)
add_subdirectory(previews)
add_subdirectory(sounds)
add_subdirectory(themes)
add_subdirectory(doc)
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()

set(kpat_SRCS ${libfcs_SRCS}
    main.cpp
    dealer.cpp
    dealerinfo.cpp
    gameselectionscene.cpp
    mainwindow.cpp
    messagebox.cpp
    numbereddealdialog.cpp
    patpile.cpp
    pileutils.cpp
    renderer.cpp
    soundengine.cpp
    statisticsdialog.cpp
    view.cpp
    patsolve/abstract_fc_solve_solver.cpp
    patsolve/memory.cpp
    patsolve/patsolve.cpp

    clock.cpp 
    patsolve/clocksolver.cpp
    fortyeight.cpp
    patsolve/fortyeightsolver.cpp
    freecell.cpp 
    patsolve/freecellsolver.cpp
    golf.cpp
    patsolve/golfsolver.cpp
    grandf.cpp
    patsolve/grandfsolver.cpp
    gypsy.cpp
    patsolve/gypsysolver.cpp
    idiot.cpp
    patsolve/idiotsolver.cpp
    klondike.cpp
    patsolve/klondikesolver.cpp
    mod3.cpp
    patsolve/mod3solver.cpp
    simon.cpp
    patsolve/simonsolver.cpp
    spider.cpp
    patsolve/spidersolver.cpp
    yukon.cpp
    patsolve/yukonsolver.cpp
)

ki18n_wrap_ui( kpat_SRCS statisticsdialog.ui )
kconfig_add_kcfg_files( kpat_SRCS settings.kcfgc )

ecm_qt_declare_logging_category(kpat_SRCS
    HEADER kpat_debug.h
    IDENTIFIER KPAT_LOG
    CATEGORY_NAME org.kde.kpat
)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*-apps-kpat.png")
ecm_add_app_icon(kpat_SRCS ICONS ${ICONS_SRCS})
add_executable(kpat ${kpat_SRCS})
target_link_libraries(kpat
    KF5::Crash
    KF5::DBusAddons
    KF5::I18n
    KF5::KIOCore
    KF5KDEGames
    kcardgame
    ${FC_SOLVE_LDFLAGS}
    ${BLACK_HOLE_SOLVER_LDFLAGS}
)

install(TARGETS kpat ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install(FILES kpat.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
install(PROGRAMS org.kde.kpat.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kpat.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES kpatui.rc  DESTINATION  ${KDE_INSTALL_KXMLGUI5DIR}/kpat)
install(FILES kpat.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
