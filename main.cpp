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

#include <kapp.h>
#include <klocale.h>
#include <kcmdlineargs.h>

#include "version.h"
#include "pwidget.h"
#include "global.h"

static const char *description = I18N_NOOP("KDE Card Game");

KConfig* config = 0;

int main( int argc, char **argv )
{
  KCmdLineArgs::init(argc, argv, "kpat", description, KPAT_VERSION );

  KApplication a;

  config = a.config();

  pWidget *p = new pWidget(0);
  p->show();
  
  a.setMainWidget(p);
  a.exec();
}
