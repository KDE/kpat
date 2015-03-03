set( kpat_themes
   ancientegypt.desktop
   ancientegypt.svgz
   ancientegypt.png

   cleangreen.desktop
   cleangreen.svgz
   cleangreen.png

   greenblaze.desktop
   greenblaze.svgz
   greenblaze.png

   oldertheme.desktop
   oldertheme.svgz
   oldertheme.png
)

install( FILES ${kpat_themes} DESTINATION  ${KDE_INSTALL_DATADIR}/kpat/themes )

install( FILES kpat.knsrc  DESTINATION  ${KDE_INSTALL_CONFDIR} )

