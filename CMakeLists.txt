project( kpat )

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.2.0")

find_package(ECM 1.3.0 REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Widgets Qml Quick QuickWidgets Svg Test)
find_package(KF5 REQUIRED COMPONENTS 
    CoreAddons 
    Config 
    WidgetsAddons 
    Config 
    DBusAddons 
    Declarative
    I18n 
    GuiAddons 
    ConfigWidgets 
    ItemViews 
    IconThemes 
    Completion 
    TextWidgets
    XmlGui
    KIO
    NotifyConfig
    NewStuff)

find_package(KF5KDEGames 4.9.0 REQUIRED)

include(FeatureSummary)
include(ECMInstallIcons)
include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(-DTRANSLATION_DOMAIN="kpat")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/libkcardgame/include )

add_subdirectory( icons )
add_subdirectory( libkcardgame )
add_subdirectory( mimetypes )
add_subdirectory( previews )
add_subdirectory( sounds )
add_subdirectory( themes )
add_subdirectory( doc )

set( kpat_SRCS
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
kde4_add_app_icon( kpat_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/hi*-app-kpat.png" )
add_executable( kpat ${kpat_SRCS} )
target_link_libraries( kpat kcardgame KF5KDEGames KF5::KIOCore Phonon::phonon4qt5 KF5::KDELibs4Support)
install( TARGETS kpat ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( PROGRAMS kpat.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kpatui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kpat )
install( FILES kpat.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
