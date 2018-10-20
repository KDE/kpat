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

ecm_qt_declare_logging_category(libkcardgame_SRCS
    HEADER libkcardgame_debug.h
    IDENTIFIER LIBKCARDGAME_LOG
    CATEGORY_NAME org.kde.games.kcardgame
)


add_library( kcardgame SHARED ${libkcardgame_SRCS} )
generate_export_header(kcardgame BASE_NAME libkcardgame)
target_link_libraries( kcardgame KF5::NewStuff KF5::GuiAddons Qt5::Svg KF5::Completion KF5::I18n)

install( TARGETS kcardgame ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES kcardtheme.knsrc  DESTINATION  ${KDE_INSTALL_CONFDIR} )
