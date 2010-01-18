set( libkcardgame_SRCS
    abstractcard.cpp
    card.cpp
    carddeck.cpp
    cardscene.cpp
    pile.cpp
)

kde4_add_library( libkcardgame SHARED ${libkcardgame_SRCS} )
target_link_libraries( libkcardgame kdegames )

install( TARGETS libkcardgame ${INSTALL_TARGETS_DEFAULT_ARGS} )
