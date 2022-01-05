set( libkcardgame_SRCS
    kabstractcarddeck.cpp
    kcard.cpp
    kcarddeck.cpp
    kcardpile.cpp
    kcardscene.cpp
    kcardtheme.cpp
    kcardthemewidget.cpp
)
ecm_generate_headers(libkcardgame_CamelCase_HEADERS
    HEADER_NAMES
        KAbstractCardDeck
        KCard
        KCardDeck
        KCardPile
        KCardScene
        KCardTheme
        KCardThemeWidget
)
ecm_qt_declare_logging_category(libkcardgame_SRCS
    HEADER libkcardgame_debug.h
    IDENTIFIER LIBKCARDGAME_LOG
    CATEGORY_NAME org.kde.games.kcardgame
    DESCRIPTION "LibKCardGame (KPatience)"
    EXPORT KPAT
)

add_library(kcardgame SHARED ${libkcardgame_SRCS})
generate_export_header(kcardgame BASE_NAME libkcardgame)
target_link_libraries(kcardgame
    PUBLIC
        KF5::ConfigWidgets
        KF5::ConfigGui
        Qt${QT_MAJOR_VERSION}::Widgets
    PRIVATE
        KF5::Completion
        KF5::GuiAddons
        KF5::NewStuff
        KF5::I18n
        Qt${QT_MAJOR_VERSION}::Svg
)

install(TARGETS kcardgame ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kcardtheme.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
