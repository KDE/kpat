set( libkcardgame_SRCS
    kabstractcarddeck.cpp
    kcard.cpp
    kcardcache.cpp
    kcardscene.cpp
    kcardtheme.cpp
    kcardthemewidget.cpp
    kstandardcarddeck.cpp
    pile.cpp
    standardcard.cpp
)

kde4_add_library( libkcardgame SHARED ${libkcardgame_SRCS} )
target_link_libraries( libkcardgame kdegames )

install( TARGETS libkcardgame ${INSTALL_TARGETS_DEFAULT_ARGS} )
