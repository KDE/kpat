
INCLUDES = -I$(top_srcdir)/libkdegames $(all_includes) 

LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD = $(LIB_KDEUI) $(top_builddir)/libkdegames/libkdegames.la

bin_PROGRAMS = kpat

kpat_SOURCES = main.cpp cardmaps.cpp card.cpp dealer.cpp \
	pwidget.cpp pile.cpp deck.cpp grandf.cpp klondike.cpp mod3.cpp \
	idiot.cpp napoleon.cpp computation.cpp freecell.cpp gypsy.cpp

METASOURCES = AUTO

SUBDIRS = icons 

apps_DATA = kpat.desktop
appsdir   = $(kde_appsdir)/Games

rcdir = $(kde_datadir)/kpat
rc_DATA = kpatui.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kpat.pot
