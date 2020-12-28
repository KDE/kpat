#! /usr/bin/env bash
$EXTRACTRC *.rc *.ui *.kcfg >> rc.cpp
$XGETTEXT *.cpp libkcardgame/*.cpp -o $podir/kpat.pot
