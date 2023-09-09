add_library(kcardgame)

target_sources(kcardgame PRIVATE
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
ecm_qt_declare_logging_category(kcardgame
    HEADER libkcardgame_debug.h
    IDENTIFIER LIBKCARDGAME_LOG
    CATEGORY_NAME org.kde.games.kcardgame
    DESCRIPTION "LibKCardGame (KPatience)"
    EXPORT KPAT
)

generate_export_header(kcardgame BASE_NAME libkcardgame)
target_link_libraries(kcardgame
    PUBLIC
        KF${KF_MAJOR_VERSION}::ConfigWidgets
        KF${KF_MAJOR_VERSION}::ConfigGui
        Qt${QT_MAJOR_VERSION}::Widgets
    PRIVATE
        KF${KF_MAJOR_VERSION}::Completion
        KF${KF_MAJOR_VERSION}::GuiAddons
        KF${KF_MAJOR_VERSION}::NewStuffWidgets
        KF${KF_MAJOR_VERSION}::I18n
        Qt${QT_MAJOR_VERSION}::Svg
)

install(TARGETS kcardgame ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kcardtheme.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
