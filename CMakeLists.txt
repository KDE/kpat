kde4_header()
project(kpat)

add_subdirectory( icons ) 
add_subdirectory( freecell-solver ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkdegames ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )

ADD_DEFINITIONS(-DFCS_STATE_STORAGE=FCS_STATE_STORAGE_INTERNAL_HASH -DFCS_STACK_STORAGE=FCS_STACK_STORAGE_INTERNAL_HASH) 

set(libfcs_SRCS
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/alloc.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/app_str.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/caas.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/card.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/cl_chop.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/cmd_line.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/fcs_dm.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/fcs_hash.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/fcs_isa.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/freecell.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/intrface.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/lib.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/lookup2.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/move.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/pqueue.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/preset.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/rand.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/scans.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/simpsim.c 
	${CMAKE_SOURCE_DIR}/kpat/freecell-solver/state.c
)
########### next target ###############

set(kpat_SRCS ${libfcs_SRCS}
   main.cpp 
   cardmaps.cpp 
   card.cpp 
   dealer.cpp 
   pwidget.cpp 
   pile.cpp 
   deck.cpp 
   pile_algorithms.cpp 
   kings.cpp 
   freecell.cpp 
   klondike.cpp 
   simon.cpp 
   grandf.cpp 
   mod3.cpp 
   idiot.cpp 
   napoleon.cpp 
   computation.cpp 
   gypsy.cpp 
   fortyeight.cpp 
   yukon.cpp 
   clock.cpp 
   golf.cpp 
   spider.cpp 
   gamestatsimpl.cpp )

kde4_automoc(${kpat_SRCS})

kde4_add_ui3_files(kpat_SRCS gamestats.ui )

kde4_add_executable(kpat ${kpat_SRCS})

target_link_libraries(kpat  ${KDE4_KIO_LIBRARIES} kdegames )

install_targets(/bin kpat )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kpat.desktop )
install_files( ${DATA_INSTALL_DIR}/kpat/backgrounds FILES green.png )
install_files( ${DATA_INSTALL_DIR}/kpat FILES kpatui.rc )

kde4_footer()



#original Makefile.am contents follow:

#
#INCLUDES = -I$(top_srcdir)/libkdegames $(all_includes) 
#
#bin_PROGRAMS = kpat
#
#kpat_SOURCES = main.cpp cardmaps.cpp card.cpp dealer.cpp \
#	pwidget.cpp pile.cpp deck.cpp pile_algorithms.cpp kings.cpp freecell.cpp klondike.cpp simon.cpp grandf.cpp \
#	mod3.cpp idiot.cpp napoleon.cpp computation.cpp  gypsy.cpp fortyeight.cpp \
#	yukon.cpp clock.cpp golf.cpp spider.cpp \
#	gamestatsimpl.cpp \
#	gamestats.ui
#kpat_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#kpat_LDADD = $(LIB_KFILE) $(LIB_KDEGAMES) ./freecell-solver/libfcs.la
#kpat_DEPENDENCIES = $(LIB_KDEGAMES_DEP)
#
#METASOURCES = AUTO
#
#SUBDIRS = icons freecell-solver
#
#xdg_apps_DATA = kpat.desktop
#
#rcdir = $(kde_datadir)/kpat
#rc_DATA = kpatui.rc
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kpat.pot
#
#bgdir = $(kde_datadir)/kpat/backgrounds
#bg_DATA = green.png
#
