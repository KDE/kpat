
INCLUDES = $(all_includes) 

LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD =       $(LIB_KDEUI)

bin_PROGRAMS = kpat

kpat_SOURCES = main.cpp patience.cpp rnd.cpp cardmaps.cpp \
	  computation.cpp klondike.cpp grandf.cpp ten.cpp\
	  basiccard.cpp card.cpp napoleon.cpp idiot.cpp\
	  microsol.cpp pwidget.cpp freecell.cpp mod3.cpp

METASOURCES = patience.moc computation.moc klondike.moc\
	  grandf.moc basiccard.moc card.moc mod3.moc\
	  ten.moc napoleon.moc idiot.moc freecell.moc\
	  microsol.moc pwidget.moc cardmaps.moc

noinst_HEADERS = basiccard.h global.h napoleon.h card.h grandf.h patience.h \
				cardmaps.h idiot.h pwidget.h computation.h klondike.h rnd.h \
				copyright.h microsol.h ten.h freecell.h mod3.h version.h     

SUBDIRS = icons pics

docsdir = $(kde_htmldir)/en/kpat

# if there are more docs, use a sub directory
docs_DATA = index.html index-1.html index-2.html logotp3.gif

apps_DATA = kpat.kdelnk
appsdir = $(kde_appsdir)/Games

EXTRA_DIST = $(apps_DATA) $(docs_DATA)

messages:
	$(XGETTEXT) -C -ktranslate -ki18n -x $(includedir)/kde.pot $(kpat_SOURCES) && mv messages.po ../po/kpat.pot

