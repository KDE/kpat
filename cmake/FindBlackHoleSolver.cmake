# SPDX-FileCopyrightText: 2023 Friedrich W. H. Kossebau <kossebau@kde.org>
#
# SPDX-License-Identifier: BSD-2-Clause

#[=======================================================================[.rst:
FindBlackHoleSolver
-------------------

Try to find the Black Hole Solitaire Solver library.

This will define the following variables:

``BlackHoleSolver_FOUND``
    TRUE if (the requested version of) Black Hole Solitaire Solver is available
``BlackHoleSolver_VERSION``
    The version of Black Hole Solitaire Solver
``BlackHoleSolver_LIBRARIES``
    The libraries of Black Hole Solitaire Solver for use with target_link_libraries()
``BlackHoleSolver_INCLUDE_DIRS``
    The include dirs of Black Hole Solitaire Solver for use with target_include_directories()

If ``BlackHoleSolver_FOUND`` is TRUE, it will also define the following imported
target:

``BlackHoleSolver::BlackHoleSolver``
    The Black Hole Solitaire Solver library
#]=======================================================================]

find_package(PkgConfig QUIET)
pkg_check_modules(PC_BlackHoleSolver libblack-hole-solver QUIET)

find_library(BlackHoleSolver_LIBRARIES
    NAMES black_hole_solver
    HINTS ${PC_BlackHoleSolver_LIBRARY_DIRS}
)

find_path(BlackHoleSolver_INCLUDE_DIRS
    NAMES black-hole-solver/black_hole_solver.h
    HINTS ${PC_BlackHoleSolver_INCLUDE_DIRS}
)

set(BlackHoleSolver_VERSION ${PC_BlackHoleSolver_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BlackHoleSolver
    REQUIRED_VARS
        BlackHoleSolver_LIBRARIES
        BlackHoleSolver_INCLUDE_DIRS
    VERSION_VAR
        BlackHoleSolver_VERSION
)

if(BlackHoleSolver_FOUND AND NOT TARGET BlackHoleSolver::BlackHoleSolver)
    add_library(BlackHoleSolver::BlackHoleSolver UNKNOWN IMPORTED)
    set_target_properties(BlackHoleSolver::BlackHoleSolver PROPERTIES
        IMPORTED_LOCATION "${BlackHoleSolver_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${PC_BlackHoleSolver_CFLAGS}"
        INTERFACE_INCLUDE_DIRECTORIES "${BlackHoleSolver_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(BlackHoleSolver_LIBRARIES BlackHoleSolver_INCLUDE_DIRS BlackHoleSolver_VERSION)

include(FeatureSummary)
set_package_properties(BlackHoleSolver PROPERTIES
    DESCRIPTION "Library for Solving Black Hole and All in a Row Solitaires"
    URL "https://www.shlomifish.org/open-source/projects/black-hole-solitaire-solver/"
)
