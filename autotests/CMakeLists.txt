include(ECMAddTests)
remove_definitions(
    -DQT_NO_CAST_FROM_ASCII
)
include_directories(
    AFTER
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/src
)
ecm_add_test(
    shuffle_test.cpp
    TEST_NAME ShuffleTest
    LINK_LIBRARIES Qt6::Test
    NAME_PREFIX "kpat-"
)
ecm_add_test(
    freecell_solver.cpp
    TEST_NAME FcSolveTest
    LINK_LIBRARIES Qt6::Test
    NAME_PREFIX "kpat-"
)

ecm_add_test(
    solve_by_name.cpp
    TEST_NAME SolveByVariantName
    LINK_LIBRARIES Qt6::Test
    NAME_PREFIX "kpat-"
)

ecm_qt_declare_logging_category(SolverFormatTest_LOG_SRCS
    HEADER kpat_debug.h
    IDENTIFIER KPAT_LOG
    CATEGORY_NAME kpat
)

if (WITH_BH_SOLVER)
    ecm_add_test(
        golf_solver_wins.cpp
        TEST_NAME GolfSolveTest
        LINK_LIBRARIES Qt6::Test
        NAME_PREFIX "kpat-"
    )

    ecm_add_test(
        "${CMAKE_SOURCE_DIR}/src/dealer.cpp"
        "${CMAKE_SOURCE_DIR}/src/dealerinfo.cpp"
        "${CMAKE_SOURCE_DIR}/src/golf.cpp"
        "${CMAKE_SOURCE_DIR}/src/patsolve/golfsolver.cpp"
        "${CMAKE_SOURCE_DIR}/src/patsolve/memory.cpp"
        "${CMAKE_SOURCE_DIR}/src/patsolve/patsolve.cpp"
        "${CMAKE_SOURCE_DIR}/src/messagebox.cpp"
        "${CMAKE_SOURCE_DIR}/src/patpile.cpp"
        "${CMAKE_SOURCE_DIR}/src/pileutils.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer.cpp"
        ${SolverFormatTest_LOG_SRCS}
        "settings_for_tests.cpp"
        solver_format.cpp
        TEST_NAME SolverFormatTest
        LINK_LIBRARIES
            Qt6::Test
            kcardgame
            KDEGames6
            KF6::I18n
            BlackHoleSolver::BlackHoleSolver
        NAME_PREFIX "kpat-"
    )
    # kpat code may include generated files, so by using any kpat file in the test
    # the test itself becomes dependent on the entire kpat target, even when not
    # using the target directly.
    # TODO: make libkpat a static library linked into the test and kpat itself so
    # the dependency order is "correct" and the compilation units are only built
    # once.
    add_dependencies(SolverFormatTest kpat)

endif()
