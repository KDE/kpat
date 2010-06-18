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

install( FILES ${kpat_themes} DESTINATION  ${DATA_INSTALL_DIR}/kpat/themes )

install( FILES kpat.knsrc  DESTINATION  ${CONFIG_INSTALL_DIR} )

