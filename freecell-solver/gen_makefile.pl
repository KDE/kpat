#!/usr/bin/perl

use strict;

my @objects=(qw(card dfs fcs_dm fcs_hash fcs_isa freecell),
    qw(main md5c move preset state));

my @headers=(qw(card config fcs fcs_dm fcs_enums fcs_hash fcs_isa),
    qw(fcs_move fcs_user md5 move preset state));

my @defines=(qw(HASH_STATE_STORAGE INTERNAL_HASH_IMPLEMENTATION WIN32));

my @debug_defines = (qw(DEBUG));

print "all: fc-solve.exe fc-solve-debug.exe\n\n";

print "OFLAGS=" . "/Og2 " . join(" ", (map { "/D".$_ } @defines)) . "\n";
print "DFLAGS=\$(OFLAGS) " . join(" ", (map { "/D".$_ } @debug_defines)) . "\n";

print "INCLUDES=" . join(" ", (map { $_.".h" } @headers)). "\n";
print "CC=cl\n";

print "\n\n";


foreach my $o (@objects)
{
    print "$o.obj: $o.c \$(INCLUDES)\n";
    print "\t\$(CC) /c /Fo$o.obj \$(OFLAGS) $o.c\n";
    print "\n";
    print "$o-debug.obj: $o.c \$(INCLUDES)\n";
    print "\t\$(CC) /c /Fo$o-debug.obj \$(DFLAGS) $o.c\n";
    print "\n";
}

print "fc-solve.exe: " . join(" ", (map { $_ . ".obj" } @objects)) . "\n";
print "\t\$(CC) /Fefc-solve.exe /F0x2000000 " . join(" ", (map { $_ . ".obj" } @objects)) . "\n";
print "\n";
print "fc-solve-debug.exe: " . join(" ", (map { $_ . "-debug.obj" } @objects)) . "\n";
print "\t\$(CC) /Fefc-solve-debug.exe /F0x2000000 " . join(" ", (map { $_ . "-debug.obj" } @objects)) . "\n";

print "clean: ";
print "\tdel *.obj *.exe\n";

