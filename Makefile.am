
INCLUDES = $(all_includes)

LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD =       $(LIB_KDEUI)

bin_PROGRAMS = kpat

kpat_SOURCES = main.cpp patience.cpp cardmaps.cpp basiccard.cpp \
	card.cpp dealer.cpp pwidget.cpp grandf.cpp
#	  computation.cpp klondike.cpp grandf.cpp ten.cpp\
	   napoleon.cpp idiot.cpp\
	  microsol.cpp pwidget.cpp freecell.cpp mod3.cpp

METASOURCES = AUTO

noinst_HEADERS = basiccard.h napoleon.h card.h grandf.h patience.h \
		cardmaps.h idiot.h pwidget.h computation.h klondike.h \
		copyright.h microsol.h ten.h freecell.h mod3.h version.h

SUBDIRS = icons pics

apps_DATA = kpat.desktop
appsdir   = $(kde_appsdir)/Games

EXTRA_DIST = $(apps_DATA)

rcdir = $(kde_datadir)/kpat
rc_DATA = kpatui.rc

messages:
	$(XGETTEXT) $(kpat_SOURCES) -o $(podir)/kpat.pot

