# SPDX-FileCopyrightText: 2026 KPatience Android port contributors
#
# SPDX-License-Identifier: BSD-3-Clause

# On Android, Qt cannot look up data files in the APK through QStandardPaths.
# Therefore the data KPat normally installs into share/ (game themes, game
# previews, sounds) plus a selection of card decks are compiled into the
# executable as Qt resources below ":/share/", mirroring the install layout.
# The icons used in the menus and the toolbar are added as icon theme
# ":/icons/breeze", see main.cpp.
#
# kpat_add_android_data(<target>) generates the resource file and adds it to
# <target>.

function(kpat_add_android_data target)
    set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/android-data")
    file(MAKE_DIRECTORY "${_gen_dir}")
    set(_entries "")
    set(_icon_entries "")

    # Adds <source> as ":/share/<alias>". If the alias ends with .svgz the
    # (uncompressed) SVG source gets gzipped first, as the .desktop files of
    # themes and card decks reference .svgz files.
    macro(_add_file source alias)
        set(_src "${source}")
        if ("${alias}" MATCHES "\\.svgz$" AND "${source}" MATCHES "\\.svg$")
            string(REPLACE "/" "_" _flat "${alias}")
            set(_src "${_gen_dir}/${_flat}")
            if (NOT EXISTS "${_src}" OR "${source}" IS_NEWER_THAN "${_src}")
                file(ARCHIVE_CREATE
                    OUTPUT "${_src}"
                    PATHS "${source}"
                    FORMAT raw
                    COMPRESSION GZip
                )
            endif()
        endif()
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source}")
        string(APPEND _entries "        <file alias=\"${alias}\">${_src}</file>\n")
    endmacro()

    # game themes
    foreach(_theme ancientegypt cleangreen greenblaze royalhearts)
        _add_file("${CMAKE_SOURCE_DIR}/themes/${_theme}.desktop" "kpat/themes/${_theme}.desktop")
        _add_file("${CMAKE_SOURCE_DIR}/themes/${_theme}.png" "kpat/themes/${_theme}.png")
        _add_file("${CMAKE_SOURCE_DIR}/themes/${_theme}.svg" "kpat/themes/${_theme}.svgz")
    endforeach()

    # previews shown in the game selection screen
    file(GLOB _previews "${CMAKE_SOURCE_DIR}/previews/*.png")
    foreach(_preview ${_previews})
        get_filename_component(_name "${_preview}" NAME)
        _add_file("${_preview}" "kpat/previews/${_name}")
    endforeach()

    # sounds
    foreach(_sound card-pickup.ogg card-down.ogg)
        _add_file("${CMAKE_SOURCE_DIR}/sounds/${_sound}" "kpat/sounds/${_sound}")
    endforeach()

    # card decks
    file(GLOB _decks LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/android/carddecks/svg-*")
    foreach(_deck ${_decks})
        get_filename_component(_deck_id "${_deck}" NAME)
        file(GLOB _deck_files "${_deck}/index.desktop" "${_deck}/*.png" "${_deck}/*.svg")
        foreach(_file ${_deck_files})
            get_filename_component(_name "${_file}" NAME)
            if (_name MATCHES "\\.svg$")
                set(_name "${_name}z")
            endif()
            _add_file("${_file}" "carddecks/${_deck_id}/${_name}")
        endforeach()
    endforeach()

    # icon theme
    set(_icons_dir "${CMAKE_CURRENT_SOURCE_DIR}/android/icons/breeze")
    file(GLOB _icons "${_icons_dir}/actions/22/*.svg")
    foreach(_icon "${_icons_dir}/index.theme" ${_icons})
        file(RELATIVE_PATH _name "${_icons_dir}" "${_icon}")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_icon}")
        string(APPEND _icon_entries "        <file alias=\"breeze/${_name}\">${_icon}</file>\n")
    endforeach()
    string(APPEND _icon_entries "        <file alias=\"breeze/apps/48/kpat.svg\">${CMAKE_SOURCE_DIR}/icons/kpat.48.64.svg</file>\n")

    set(_qrc "${_gen_dir}/kpat-android-data.qrc")
    file(WRITE "${_qrc}.tmp" "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n    <qresource prefix=\"/share\">\n${_entries}    </qresource>\n    <qresource prefix=\"/icons\">\n${_icon_entries}    </qresource>\n</RCC>\n")
    file(COPY_FILE "${_qrc}.tmp" "${_qrc}" ONLY_IF_DIFFERENT)
    target_sources(${target} PRIVATE "${_qrc}")
endfunction()
