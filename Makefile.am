
INCLUDES = $(all_includes) 

LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD = -lkdeui -lkdecore  -lXext  $(LIB_QT)

bin_PROGRAMS = kpat

kpat_SOURCES = main.cpp patience.cpp rnd.cpp cardmaps.cpp \
	  computation.cpp klondike.cpp grandf.cpp ten.cpp\
	  basiccard.cpp card.cpp napoleon.cpp idiot.cpp\
	  microsol.cpp pwidget.cpp freecell.cpp mod3.cpp

BUILTSOURCES = patience.moc computation.moc klondike.moc\
	  grandf.moc basiccard.moc card.moc mod3.moc\
	  ten.moc napoleon.moc idiot.moc freecell.moc\
	  microsol.moc pwidget.moc cardmaps.moc

noinst_HEADERS = basiccard.h global.h napoleon.h card.h grandf.h patience.h \
				cardmaps.h idiot.h pwidget.h computation.h klondike.h rnd.h \
				copyright.h microsol.h ten.h freecell.h mod3.h version.h     
DISTCLEANFILES = $(BUILTSOURCES)


SUBDIRS = icons pics

docsdir = $(kde_htmldir)/en/kpat

# if there are more docs, use a sub directory
docs_DATA = index.html index-1.html index-2.html logotp3.gif

apps_DATA = kpat.kdelnk
appsdir = $(kde_appsdir)/Games

EXTRA_DIST = $(apps_DATA) $(docs_DATA)

messages:
	$(XGETTEXT) -C -ktranslate -ki18n -x $(includedir)/kde.pot $(kpat_SOURCES) && mv messages.po ../po/kpat.pot

# Dependencies
basiccard.cpp:basiccard.moc
basiccard.moc: basiccard.h
	$(MOC) basiccard.h -o basiccard.moc

card.cpp:card.moc
card.moc: card.h
	$(MOC) card.h -o card.moc

cardmaps.cpp:cardmaps.moc
cardmaps.moc: cardmaps.h
	$(MOC) cardmaps.h -o cardmaps.moc

computation.cpp:computation.moc
computation.moc: computation.h
	$(MOC) computation.h -o computation.moc

freecell.cpp:freecell.moc
freecell.moc: freecell.h
	$(MOC) freecell.h -o freecell.moc

grandf.cpp:grandf.moc
grandf.moc: grandf.h
	$(MOC) grandf.h -o grandf.moc

idiot.cpp:idiot.moc
idiot.moc: idiot.h
	$(MOC) idiot.h -o idiot.moc

klondike.cpp:klondike.moc
klondike.moc: klondike.h
	$(MOC) klondike.h -o klondike.moc

microsol.cpp:microsol.moc
microsol.moc: microsol.h
	$(MOC) microsol.h -o microsol.moc

mod3.cpp:mod3.moc
mod3.moc: mod3.h
	$(MOC) mod3.h -o mod3.moc

napoleon.cpp:napoleon.moc
napoleon.moc: napoleon.h
	$(MOC) napoleon.h -o napoleon.moc

patience.cpp:patience.moc
patience.moc: patience.h
	$(MOC) patience.h -o patience.moc

pwidget.cpp:pwidget.moc
pwidget.moc: pwidget.h
	$(MOC) pwidget.h -o pwidget.moc

ten.cpp:ten.moc
ten.moc: ten.h
	$(MOC) ten.h -o ten.moc

