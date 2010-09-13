set( libkcardgame_SRCS
    common.cpp
    kabstractcarddeck.cpp
    kcard.cpp
    kcarddeck.cpp
    kcardpile.cpp
    kcardscene.cpp
    kcardtheme.cpp
    kcardthemewidget.cpp
)

kde4_add_library( kcardgame SHARED ${libkcardgame_SRCS} )
target_link_libraries( kcardgame kdegames ${KDE4_KNEWSTUFF3_LIBS} )

install( TARGETS kcardgame ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES kcardtheme.knsrc  DESTINATION  ${CONFIG_INSTALL_DIR} )
