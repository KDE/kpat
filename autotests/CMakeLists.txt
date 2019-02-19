include(ECMAddTests)
include_directories(AFTER "${CMAKE_CURRENT_SOURCE_DIR}/..")
include_directories(AFTER "${CMAKE_CURRENT_BINARY_DIR}/..")
ecm_add_test(
    shuffle_test.cpp
    TEST_NAME ShuffleTest
    LINK_LIBRARIES Qt5::Test
    NAME_PREFIX "kpat-"
)
ecm_add_test(
    "../dealer.cpp"
    "../dealerinfo.cpp"
    "../golf.cpp"
    "../patsolve/golfsolver.cpp"
    "../patsolve/memory.cpp"
    "../patsolve/patsolve.cpp"
    "../messagebox.cpp"
    "../kpat_debug.cpp"
    "../patpile.cpp"
    "../pileutils.cpp"
    "../renderer.cpp"
    solver_format.cpp
    TEST_NAME SolverFormatTest
    LINK_LIBRARIES Qt5::Test kcardgame
        KF5KDEGames
        ${BLACK_HOLE_SOLVER_LDFLAGS}
    NAME_PREFIX "kpat-"
)
