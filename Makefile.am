
INCLUDES = -I$(top_srcdir)/libkdegames $(all_includes) 

bin_PROGRAMS = kpat

kpat_SOURCES = main.cpp cardmaps.cpp card.cpp dealer.cpp \
	pwidget.cpp pile.cpp deck.cpp pile_algorithms.cpp kings.cpp freecell.cpp klondike.cpp simon.cpp grandf.cpp \
	mod3.cpp idiot.cpp napoleon.cpp computation.cpp  gypsy.cpp fortyeight.cpp \
	yukon.cpp clock.cpp golf.cpp spider.cpp \
	gamestatsimpl.cpp \
	gamestats.ui
kpat_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kpat_LDADD = $(LIB_KFILE) $(LIB_KDEGAMES) ./freecell-solver/libfcs.la
kpat_DEPENDENCIES = $(LIB_KDEGAMES_DEP)

METASOURCES = AUTO

SUBDIRS = icons freecell-solver

xdg_apps_DATA = kpat.desktop

rcdir = $(kde_datadir)/kpat
rc_DATA = kpatui.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kpat.pot

bgdir = $(kde_datadir)/kpat/backgrounds
bg_DATA = green.png

