add_subdirectory(libkcardgame)

ecm_setup_version(${kpat_VERSION}
    VARIABLE_PREFIX KPAT
    VERSION_HEADER kpat_version.h
)

set(WITH_FCS_SOFT_SUSPEND ${fcs_soft_suspend})
configure_file(patsolve-config.h.in patsolve-config.h)

set(kpat_SRCS ${libfcs_SRCS}
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

    clock.cpp 
    patsolve/clocksolver.cpp
    fortyeight.cpp
    patsolve/fortyeightsolver.cpp
    freecell.cpp 
    patsolve/freecellsolver.cpp
    golf.cpp
    patsolve/golfsolver.cpp
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
    patsolve/spidersolver.cpp
    yukon.cpp
    patsolve/yukonsolver.cpp
)

ki18n_wrap_ui( kpat_SRCS statisticsdialog.ui )
kconfig_add_kcfg_files( kpat_SRCS settings.kcfgc )

ecm_qt_declare_logging_category(kpat_SRCS
    HEADER kpat_debug.h
    IDENTIFIER KPAT_LOG
    CATEGORY_NAME org.kde.kpat
)

file(GLOB ICONS_SRCS "${CMAKE_SOURCE_DIR}/icons/*-apps-kpat.png")
ecm_add_app_icon(kpat_SRCS ICONS ${ICONS_SRCS})

add_executable(kpat ${kpat_SRCS})

target_link_libraries(kpat
    KF5::Crash
    KF5::DBusAddons
    KF5::I18n
    KF5::KIOCore
    KF5::GuiAddons
    KF5::Completion
    KF5::XmlGui
    KF5KDEGames
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

install(FILES kpat.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
install(PROGRAMS org.kde.kpat.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kpat.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES kpatui.rc  DESTINATION  ${KDE_INSTALL_KXMLGUI5DIR}/kpat)
install(FILES kpat.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
