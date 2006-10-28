project(kpat)

add_subdirectory( icons ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkdegames  )

ADD_DEFINITIONS(-DFCS_STATE_STORAGE=FCS_STATE_STORAGE_INTERNAL_HASH -DFCS_STACK_STORAGE=FCS_STACK_STORAGE_INTERNAL_HASH) 

set(libfcs_SRCS
	freecell-solver/alloc.c 
	freecell-solver/app_str.c 
	freecell-solver/caas.c 
	freecell-solver/card.c 
	freecell-solver/cl_chop.c 
	freecell-solver/cmd_line.c 
	freecell-solver/fcs_dm.c 
	freecell-solver/fcs_hash.c 
	freecell-solver/fcs_isa.c 
	freecell-solver/freecell.c 
	freecell-solver/intrface.c 
	freecell-solver/lib.c 
	freecell-solver/lookup2.c 
	freecell-solver/move.c 
	freecell-solver/pqueue.c 
	freecell-solver/preset.c 
	freecell-solver/rand.c 
	freecell-solver/scans.c 
	freecell-solver/simpsim.c 
	freecell-solver/state.c
)
########### next target ###############

set(RELEASE "1")

set(kpat_SRCS ${libfcs_SRCS}
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
   klondike.cpp
   freecell.cpp
)

if (RELEASE)
set(kpat_SRCS ${kpat_SRCS}
   mod3.cpp
   simon.cpp
   grandf.cpp
   idiot.cpp
   napoleon.cpp
   computation.cpp
   gypsy.cpp
   fortyeight.cpp
   yukon.cpp
   clock.cpp
   golf.cpp
   spider.cpp
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
install( FILES green.png  DESTINATION  ${DATA_INSTALL_DIR}/kpat/backgrounds )
install( FILES kpatui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kpat )

