project(kpat)

add_subdirectory( icons ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkdegames  )

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
   idiot.cpp
   patsolve/idiot.cpp
   yukon.cpp
   patsolve/yukon.cpp
   #fortyeight.cpp
   #patsolve/fortyeight.cpp
   golf.cpp
   patsolve/golf.cpp
   mod3.cpp
   grandf.cpp
   napoleon.cpp
   computation.cpp
   gypsy.cpp
   clock.cpp
)
endif(RELEASE)

kde4_automoc(${kpat_SRCS})

kde4_add_ui_files(kpat_SRCS gamestats.ui )

kde4_add_executable(kpat ${kpat_SRCS})

IF (QT_QTOPENGL_FOUND)
target_link_libraries(kpat  ${KDE4_KIO_LIBS} kdegames ${QT_QTOPENGL_LIBRARY})
ELSE (QT_QTOPENGL_FOUND)
target_link_libraries(kpat  ${KDE4_KIO_LIBS} kdegames)
ENDIF (QT_QTOPENGL_FOUND)

install(TARGETS kpat  DESTINATION ${BIN_INSTALL_DIR} )


########### install files ###############

install( FILES kpat.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES pile.svg won.svg  DESTINATION  ${DATA_INSTALL_DIR}/kpat )
install( FILES background.svg DESTINATION  ${DATA_INSTALL_DIR}/kpat/backgrounds )
install( FILES kpatui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kpat )

