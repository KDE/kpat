project(kpat)

add_subdirectory( icons ) 

########### next target ###############

set(RELEASE "1")

set(kpat_SRCS
   main.cpp 
   cardmaps.cpp 
   card.cpp 
   dealer.cpp 
   view.cpp
   pwidget.cpp 
   pile.cpp 
   deck.cpp 
   pile_algorithms.cpp
   gamestatsimpl.cpp
   patsolve/patsolve.cpp
   patsolve/memory.cpp
   patsolve/klondike.cpp
   klondike.cpp
   fortyeight.cpp
   patsolve/fortyeight.cpp
)

if (RELEASE)
set(kpat_SRCS ${kpat_SRCS}
   patsolve/spider.cpp
   spider.cpp
   patsolve/freecell.cpp
   freecell.cpp 
   patsolve/simon.cpp
   simon.cpp
   patsolve/idiot.cpp
   idiot.cpp
   patsolve/yukon.cpp
   yukon.cpp
   patsolve/golf.cpp
   golf.cpp
   patsolve/clock.cpp
   clock.cpp
   patsolve/mod3.cpp
   mod3.cpp
   patsolve/grandf.cpp
   grandf.cpp
   patsolve/gypsy.cpp
   gypsy.cpp
)
endif(RELEASE)

kde4_add_ui_files(kpat_SRCS gamestats.ui )

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
install( FILES pile.svg won.svg  DESTINATION  ${DATA_INSTALL_DIR}/kpat )
install( FILES background.svg DESTINATION  ${DATA_INSTALL_DIR}/kpat/backgrounds )
install( FILES kpatui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kpat )

install( FILES kpat_update_cardwidth.upd DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )
