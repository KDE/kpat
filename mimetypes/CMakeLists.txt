find_package( SharedMimeInfo REQUIRED )

install( FILES kpatience.xml DESTINATION ${KDE_INSTALL_MIMEDIR} )
update_xdg_mimetypes( ${KDE_INSTALL_MIMEDIR} )
