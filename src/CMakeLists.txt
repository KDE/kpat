add_subdirectory(libkcardgame)

add_executable(kpat
    biziutils.h)

ecm_setup_version(${kpat_VERSION}
    VARIABLE_PREFIX KPAT
    VERSION_HEADER kpat_version.h
)

get_target_property(WITH_FCS_SOFT_SUSPEND FreecellSolver::FreecellSolver FREECELLSOLVER_HAS_SOFT_SUSPEND)
configure_file(patsolve-config.h.in patsolve-config.h)

target_sources(kpat PRIVATE
    main.cpp
    dealer.cpp
    dealerinfo.cpp
    gameselectionscene.cpp
    mainwindow.cpp
    messagebox.cpp
    numbereddealdialog.cpp
    patpile.cpp
    pileutils.cpp
    renderer.cpp
    soundengine.cpp
    statisticsdialog.cpp
    view.cpp
    patsolve/abstract_fc_solve_solver.cpp
    patsolve/memory.cpp
    patsolve/patsolve.cpp

    bakersdozen.cpp
    patsolve/bakersdozensolver.cpp
    bizilizi.cpp
    patsolve/bizilizisolver.cpp
    castle.cpp
    patsolve/castlesolver.cpp
    clock.cpp 
    patsolve/clocksolver.cpp
    fortyeight.cpp
    patsolve/fortyeightsolver.cpp
    freecell.cpp 
    patsolve/freecellsolver.cpp
    golf.cpp
    grandf.cpp
    patsolve/grandfsolver.cpp
    normandy.cpp
    patsolve/normandysolver.cpp
    idiot.cpp
    patsolve/idiotsolver.cpp
    klondike.cpp
    patsolve/klondikesolver.cpp
    mod3.cpp
    patsolve/mod3solver.cpp
    simon.cpp
    patsolve/simonsolver.cpp
    spider.cpp
    patsolve/spidersolver2.cpp
    yukon.cpp
    patsolve/yukonsolver.cpp
    kpat.qrc
)
if (WITH_BH_SOLVER)
    target_sources(kpat PRIVATE
        patsolve/golfsolver.cpp
    )
endif()

ki18n_wrap_ui(kpat statisticsdialog.ui)
kconfig_add_kcfg_files(kpat settings.kcfgc)

ecm_qt_declare_logging_category(kpat
    HEADER kpat_debug.h
    IDENTIFIER KPAT_LOG
    CATEGORY_NAME kpat
    OLD_CATEGORY_NAMES org.kde.kpat
    DESCRIPTION "KPatience game"
    EXPORT KPAT
)

file(GLOB ICONS_SRCS "${CMAKE_SOURCE_DIR}/icons/*-apps-kpat.png")
ecm_add_app_icon(kpat ICONS ${ICONS_SRCS})

target_link_libraries(kpat
    KF6::I18n
    KF6::GuiAddons
    KF6::Completion
    KF6::XmlGui
    kcardgame
    FreecellSolver::FreecellSolver
)
if (ANDROID)
    # The parts of libkdegames KPat uses, built into kpat itself
    target_sources(kpat PRIVATE
        android/kdegames/colorproxy_p.cpp
        android/kdegames/colorproxy_p.h
        android/kdegames/kdegames_logging.cpp
        android/kdegames/kdegames_logging.h
        android/kdegames/kgamerenderer.cpp
        android/kdegames/kgamerenderer.h
        android/kdegames/kgamerenderer_p.h
        android/kdegames/kgamerendereditem.cpp
        android/kdegames/kgamerendereditem.h
        android/kdegames/kgamerendererclient.cpp
        android/kdegames/kgamerendererclient.h
        android/kdegames/kgamesound.cpp
        android/kdegames/kgamesound.h
        android/kdegames/kgamestandardaction.cpp
        android/kdegames/kgamestandardaction.h
        android/kdegames/kgametheme.cpp
        android/kdegames/kgametheme.h
        android/kdegames/kgametheme_p.h
        android/kdegames/kgamethemeprovider.cpp
        android/kdegames/kgamethemeprovider.h
        android/kdegames/kgamethemeselector.cpp
        android/kdegames/kgamethemeselector.h
        android/kdegames/kgamethemeselector_p.h
    )
    target_include_directories(kpat PRIVATE android/kdegames)
    target_link_libraries(kpat
        KF6::ConfigWidgets
        KF6::WidgetsAddons
        Qt6::Svg
        Qt6::Xml
    )
    if (TARGET Qt6::Multimedia)
        target_link_libraries(kpat Qt6::Multimedia)
        target_compile_definitions(kpat PRIVATE KPAT_HAVE_QTMULTIMEDIA)
    endif()

    include(android/AndroidData.cmake)
    kpat_add_android_data(kpat)

    include(ECMAddAndroidApk)
    ecm_add_android_apk(kpat ANDROID_DIR ${CMAKE_SOURCE_DIR}/android)
else()
    target_link_libraries(kpat
        KDEGames6
        KF6::Crash
        KF6::DBusAddons
        KF6::KIOCore
    )
endif()
if (WITH_BH_SOLVER)
    target_link_libraries(kpat
        BlackHoleSolver::BlackHoleSolver
    )
endif()

install(TARGETS kpat ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS org.kde.kpat.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kpat.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
if (NOT ANDROID)
    install(FILES kpat.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
endif()

ecm_qt_install_logging_categories(
    EXPORT KPAT
    FILE kpat.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
