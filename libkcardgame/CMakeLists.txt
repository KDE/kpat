include(GenerateExportHeader)
set( libkcardgame_SRCS
    kabstractcarddeck.cpp
    kcard.cpp
    kcarddeck.cpp
    kcardpile.cpp
    kcardscene.cpp
    kcardtheme.cpp
    kcardthemewidget.cpp
)

add_library( kcardgame SHARED ${libkcardgame_SRCS} )
generate_export_header(kcardgame BASE_NAME libkcardgame)
target_link_libraries( kcardgame KF5::NewStuff KF5::GuiAddons Qt5::Svg KF5::Completion KF5::I18n)

install( TARGETS kcardgame ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES kcardtheme.knsrc  DESTINATION  ${KDE_INSTALL_CONFDIR} )
