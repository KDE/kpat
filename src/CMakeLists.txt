add_subdirectory(libkcardgame)

add_executable(kpat)

ecm_setup_version(${kpat_VERSION}
    VARIABLE_PREFIX KPAT
    VERSION_HEADER kpat_version.h
)

set(WITH_FCS_SOFT_SUSPEND ${fcs_soft_suspend})
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
    gypsy.cpp
    patsolve/gypsysolver.cpp
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
    CATEGORY_NAME org.kde.kpat
    DESCRIPTION "KPatience"
    EXPORT KPAT
)

file(GLOB ICONS_SRCS "${CMAKE_SOURCE_DIR}/icons/*-apps-kpat.png")
ecm_add_app_icon(kpat ICONS ${ICONS_SRCS})

if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(kpat KDEGames6)
else()
    target_link_libraries(kpat KF5KDEGames)
endif()

target_link_libraries(kpat
    KF${KF_MAJOR_VERSION}::Crash
    KF${KF_MAJOR_VERSION}::DBusAddons
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::GuiAddons
    KF${KF_MAJOR_VERSION}::Completion
    KF${KF_MAJOR_VERSION}::XmlGui
    kcardgame
    ${FC_SOLVE_LDFLAGS}
    ${BLACK_HOLE_SOLVER_LDFLAGS}
)
target_include_directories(kpat
    PRIVATE
        ${FC_SOLVE_INCLUDE_DIRS}
        ${BLACK_HOLE_SOLVER_INCLUDE_DIRS}
)

install(TARGETS kpat ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS org.kde.kpat.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kpat.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES kpat.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

ecm_qt_install_logging_categories(
    EXPORT KPAT
    FILE kpat.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
