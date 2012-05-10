project( kpat )

if(NOT COMPILING_KDEGAMES)
	find_package(KDE4 REQUIRED)
	include(KDE4Defaults)
	include(MacroLibrary)
	find_package(KDEGames REQUIRED)

	add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
	add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
	include_directories(${QDBUS_INCLUDE_DIRS} ${KDE4_INCLUDES} ${KDEGAMES_INCLUDE_DIRS})
endif(NOT COMPILING_KDEGAMES)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/libkcardgame/include )

add_subdirectory( icons )
add_subdirectory( libkcardgame )
add_subdirectory( mimetypes )
add_subdirectory( previews )
add_subdirectory( sounds )
add_subdirectory( themes )

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

kde4_add_ui_files( kpat_SRCS statisticsdialog.ui )
kde4_add_kcfg_files( kpat_SRCS settings.kcfgc )
kde4_add_app_icon( kpat_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/hi*-app-kpat.png" )
kde4_add_executable( kpat ${kpat_SRCS} )
target_link_libraries( kpat kcardgame kdegames ${KDE4_KIO_LIBS} ${KDE4_PHONON_LIBS} )
install( TARGETS kpat ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( PROGRAMS kpat.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kpatui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kpat )
install( FILES kpat.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
