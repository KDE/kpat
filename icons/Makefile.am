
kde_icon_DATA = kpat.xpm kpat-lq.xpm

EXTRA_DIST = $(kde_icon_DATA) mini-kpat.xpm kpat.xpm

install-data-local:
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) $(srcdir)/mini-kpat.xpm $(kde_minidir)/kpat.xpm
	$(mkinstalldirs) $(kde_icondir)/large
	$(INSTALL_DATA) $(srcdir)/large-kpat.xpm $(kde_icondir)/large/kpat.xpm

uninstall-local:
	rm -f $(kde_minidir)/kpat.xpm
	rm -f $(kde_icondir)/large/kpat.xpm


