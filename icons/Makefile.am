
icon_DATA = kpat.xpm kpat-lq.xpm

icondir = $(kde_icondir)

EXTRA_DIST = $(icon_DATA) mini-kpat.xpm kpat.xpm

install-data-local:
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) mini-kpat.xpm $(kde_minidir)/kpat.xpm

uninstall-local:
	rm -f $(kde_minidir)/kpat.xpm


