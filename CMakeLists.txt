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

set(kpat_SRCS ${libfcs_SRCS}
   main.cpp 
   cardmaps.cpp 
   card.cpp 
   dealer.cpp 
   pwidget.cpp 
   pile.cpp 
   deck.cpp 
   pile_algorithms.cpp 
   klondike.cpp 
   gamestatsimpl.cpp )

kde4_automoc(${kpat_SRCS})

kde4_add_ui_files(kpat_SRCS gamestats.ui )

kde4_add_executable(kpat ${kpat_SRCS})

target_link_libraries(kpat  ${KDE4_KIO_LIBS} kdegames )

install_targets(/bin kpat )


########### install files ###############

install( FILES kpat.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES green.png  DESTINATION  ${DATA_INSTALL_DIR}/kpat/backgrounds )
install( FILES kpatui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kpat )




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
