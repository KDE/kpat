# SPDX-FileCopyrightText: 2023 Friedrich W. H. Kossebau <kossebau@kde.org>
#
# SPDX-License-Identifier: BSD-2-Clause

#[=======================================================================[.rst:
FindFreecellSolver
------------------

Try to find the Freecell Solver library.

This will define the following variables:

``FreecellSolver_FOUND``
    TRUE if (the requested version of) Freecell Solver is available
``FreecellSolver_VERSION``
    The version of Freecell Solver
``FreecellSolver_LIBRARIES``
    The libraries of Freecell Solver for use with target_link_libraries()
``FreecellSolver_INCLUDE_DIRS``
    The include dirs of Freecell Solver for use with target_include_directories()

If ``FreecellSolver_FOUND`` is TRUE, it will also define the following imported
target:

``FreecellSolver::FreecellSolver``
    The FreecellSolver library

The target has a property ``FREECELLSOLVER_HAS_SOFT_SUSPEND`` which specifies
if the library has the FCS_STATE_SOFT_SUSPEND_PROCESS state or not.
#]=======================================================================]

find_package(PkgConfig QUIET)
pkg_check_modules(PC_FreecellSolver libfreecell-solver QUIET)

find_library(FreecellSolver_LIBRARIES
    NAMES freecell-solver
    HINTS ${PC_FreecellSolver_LIBRARY_DIRS}
)

find_path(FreecellSolver_INCLUDE_DIRS
    NAMES freecell-solver/fcs_user.h
    HINTS ${PC_FreecellSolver_INCLUDE_DIRS}
)

set(FreecellSolver_VERSION ${PC_FreecellSolver_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FreecellSolver
    REQUIRED_VARS
        FreecellSolver_LIBRARIES
        FreecellSolver_INCLUDE_DIRS
    VERSION_VAR
        FreecellSolver_VERSION
)

set(fcs_has_soft_suspend FALSE)
if(FreecellSolver_FOUND)
    try_compile(fcs_has_soft_suspend "${CMAKE_CURRENT_BINARY_DIR}"
        SOURCES "${CMAKE_CURRENT_LIST_DIR}/fcs_soft_suspend_test.c"
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${FreecellSolver_INCLUDE_DIRS}"
        LINK_LIBRARIES ${FreecellSolver_LIBRARIES})
endif()

if(FreecellSolver_FOUND AND NOT TARGET FreecellSolver::FreecellSolver)
    add_library(FreecellSolver::FreecellSolver UNKNOWN IMPORTED)
    set_target_properties(FreecellSolver::FreecellSolver PROPERTIES
        IMPORTED_LOCATION "${FreecellSolver_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${PC_FreecellSolver_CFLAGS}"
        INTERFACE_INCLUDE_DIRECTORIES "${FreecellSolver_INCLUDE_DIRS}"
        FREECELLSOLVER_HAS_SOFT_SUSPEND "${fcs_has_soft_suspend}"
    )
endif()

mark_as_advanced(FreecellSolver_LIBRARIES FreecellSolver_INCLUDE_DIRS FreecellSolver_VERSION)

include(FeatureSummary)
set_package_properties(FreecellSolver PROPERTIES
    DESCRIPTION "Library for Solving Freecell and Similar Solitaire Games."
    URL "https://fc-solve.shlomifish.org/"
)
