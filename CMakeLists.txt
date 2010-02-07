project( kpat )

add_subdirectory( icons )
add_subdirectory( libkcardgame )
add_subdirectory( previews )

set( kpat_SRCS
    main.cpp
    dealer.cpp
    dealerinfo.cpp
    gameselectionscene.cpp
    mainwindow.cpp
    patpile.cpp
    pileutils.cpp
    render.cpp
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
target_link_libraries( kpat ${KDE4_KIO_LIBS} kdegames libkcardgame )
install( TARGETS kpat ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES kpat.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES theme.svgz DESTINATION  ${DATA_INSTALL_DIR}/kpat )
install( FILES kpatui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kpat )
install( FILES kpat.kcfg DESTINATION ${KCFG_INSTALL_DIR} )
install( FILES kpat_update_cardwidth.upd DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )
