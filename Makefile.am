
INCLUDES = $(all_includes) 

LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD = -lkdeui -lkdecore -lqt -lXext -lX11 $(LIBSOCKET)

SUFFIXES = .moc

.h.moc: 
	$(MOC) $< -o $@

bin_PROGRAMS = kpat

kpat_SOURCES = main.cpp patience.cpp rnd.cpp cardmaps.cpp \
	  computation.cpp klondike.cpp grandf.cpp ten.cpp\
	  basiccard.cpp card.cpp napoleon.cpp idiot.cpp\
	  microsol.cpp pwidget.cpp freecell.cpp mod3.cpp

BUILTSOURCES = patience.moc computation.moc klondike.moc\
	  grandf.moc basiccard.moc card.moc mod3.moc\
	  ten.moc napoleon.moc idiot.moc freecell.moc\
	  microsol.moc pwidget.moc cardmaps.moc

DISTCLEANFILES = $(BUILTSOURCES)

SUBDIRS = icons pics

docsdir = $(kde_htmldir)/en/kpat

# if there are more docs, use a sub directory
docs_DATA = index.html

apps_DATA = kpat.kdelnk
appsdir = $(kde_appsdir)/Games

messages:
	@XGETTEXT@ -C -ktranslate -x ../../kdelibs/po/kde.pot *.cpp && mv messages.po ../po/kpat.pot

# Dependencies
basiccard.cpp: basiccard.moc
card.cpp: card.moc
cardmaps.cpp: cardmaps.moc
computation.cpp: computation.moc
freecell.cpp: freecell.moc
grandf.cpp: grandf.moc
idiot.cpp: idiot.moc
klondike.cpp: klondike.moc
microsol.cpp: microsol.moc
mod3.cpp: mod3.moc
napoleon.cpp: napoleon.moc
patience.cpp: patience.moc
pwidget.cpp: pwidget.moc
ten.cpp: ten.moc
