kde4_header()

add_subdirectory( icons ) 
add_subdirectory( freecell-solver ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkdegames ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kpat_SRCS 
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

target_link_libraries(kpat  ${QT_AND_KDECORE_LIBS} fcs )

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
