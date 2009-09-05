project(kpat)

add_subdirectory( icons ) 
add_subdirectory( demos )

########### next target ###############

set(RELEASE "1")

set(kpat_SRCS
   main.cpp 
   cardmaps.cpp 
   card.cpp 
   dealer.cpp 
   view.cpp
   mainwindow.cpp 
   pile.cpp 
   deck.cpp 
   demo.cpp
   pile_algorithms.cpp
   statisticsdialog.cpp
   patsolve/patsolve.cpp
   patsolve/memory.cpp
   patsolve/klondikesolver.cpp
   klondike.cpp
   render.cpp
)

if (RELEASE)
set(kpat_SRCS ${kpat_SRCS}
   patsolve/spidersolver.cpp
   spider.cpp
   patsolve/freecellsolver.cpp
   freecell.cpp 
   patsolve/simonsolver.cpp
   simon.cpp
   patsolve/idiotsolver.cpp
   idiot.cpp
   patsolve/yukonsolver.cpp
   yukon.cpp
   patsolve/golfsolver.cpp
   golf.cpp
   patsolve/clocksolver.cpp
   clock.cpp
   patsolve/mod3solver.cpp
   mod3.cpp
   patsolve/grandfsolver.cpp
   grandf.cpp
   patsolve/gypsysolver.cpp
   gypsy.cpp
   patsolve/fortyeightsolver.cpp
   fortyeight.cpp
)
endif(RELEASE)

kde4_add_ui_files(kpat_SRCS statisticsdialog.ui )

kde4_add_app_icon(kpat_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/hi*-app-kpat.png")
kde4_add_executable(kpat ${kpat_SRCS})

IF (QT_QTOPENGL_FOUND)
target_link_libraries(kpat  ${KDE4_KIO_LIBS} kdegames ${QT_QTOPENGL_LIBRARY})
ELSE (QT_QTOPENGL_FOUND)
target_link_libraries(kpat  ${KDE4_KIO_LIBS} kdegames)
ENDIF (QT_QTOPENGL_FOUND)

install(TARGETS kpat  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES kpat.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES theme.svgz DESTINATION  ${DATA_INSTALL_DIR}/kpat )
install( FILES kpatui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kpat )

install( FILES kpat_update_cardwidth.upd DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )
