
INCLUDES = -I$(top_srcdir)/libkdegames $(all_includes) 

AM_CPPFLAGS = -DINDIRECT_STATE_STORAGE -DFCS_STATE_STORAGE=FCS_STATE_STORAGE_INTERNAL_HASH
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD = $(LIB_KFILE) $(top_builddir)/libkdegames/libkdegames.la ./freecell-solver/libfcs.la

bin_PROGRAMS = kpat

kpat_SOURCES = main.cpp cardmaps.cpp card.cpp dealer.cpp \
	pwidget.cpp pile.cpp deck.cpp pile_algorithms.cpp kings.cpp freecell.cpp klondike.cpp simon.cpp grandf.cpp \
	mod3.cpp idiot.cpp napoleon.cpp computation.cpp  gypsy.cpp fortyeight.cpp \
	yukon.cpp clock.cpp golf.cpp

METASOURCES = AUTO

SUBDIRS = icons freecell-solver

apps_DATA = kpat.desktop
appsdir   = $(kde_appsdir)/Games

rcdir = $(kde_datadir)/kpat
rc_DATA = kpatui.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kpat.pot
