set( libkcardgame_SRCS
    kabstractcarddeck.cpp
    kcard.cpp
    kcardcache.cpp
    kcardpile.cpp
    kcardscene.cpp
    kcardtheme.cpp
    kcardthemewidget.cpp
    kstandardcarddeck.cpp
)

kde4_add_library( libkcardgame SHARED ${libkcardgame_SRCS} )
target_link_libraries( libkcardgame kdegames ${KDE4_KNEWSTUFF3_LIBS} )

install( TARGETS libkcardgame ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES kcardtheme.knsrc  DESTINATION  ${CONFIG_INSTALL_DIR} )
