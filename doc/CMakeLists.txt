########### install files ###############
#
#

kdoctools_create_handbook(index.docbook INSTALL_DESTINATION ${KDE_INSTALL_DOCBUNDLEDIR}/en SUBDIR kpat)
kdoctools_create_manpage( man-kpat.6.docbook 6 INSTALL_DESTINATION ${KDE_INSTALL_MANDIR})
