/***********************-*-C++-*-********

  Rnd.h    C++ interface to C stdlib rand()

     Copyright (C) 1995  Paul Olav Tvete

 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.


//
// I couldn't get Libg++'s random function to work on my machine
//


****************************************/


#ifndef P_RND_CLUDGE
#define P_RND_CLUDGE


class Rnd {
public:
    Rnd(unsigned int seed=0);
    int rInt(int Range); // Returns 0..Range-1
};





#endif





