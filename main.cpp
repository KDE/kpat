/* 
     patience -- main program
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

 */

#include "pwidget.h"
#include <kapp.h>
#include "global.h"

KConfig *config = 0;
QString PICDIR;

int main( int argc, char **argv )
{
  KApplication a( argc, argv, "kpat");

  config = a.getConfig();
  PICDIR = a.kde_datadir() + "/kpat/pics/";

  pWidget *p = new pWidget(0);
  p->show();
  
  a.setMainWidget(p);
  a.exec();
}
