/* -*- C++ -*-
 *
 * patience -- main program
 *   Copyright (C) 1995  Paul Olav Tvete
 *
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
 *
 *
 * Heavily modified by Mario Weilguni <mweilguni@sime.com>
 *
 */

#ifndef __PWIDGET__H__
#define __PWIDGET__H__

#include <kmainwindow.h>

class Dealer;
class KToggleAction;
class KSelectAction;
class KAction;
class QWidgetStack;
class QLabel;

class pWidget: public KMainWindow {
    Q_OBJECT

public:
    pWidget( const char *name=0 );

protected slots:
    void undoMove();
    void changeBackside();
    void animationChanged();
    void newGameType();
    void restart();

private:
    void setBackSide(const QString &id);

    Dealer* dill;
    KSelectAction *games;
    KAction *backs;
    KToggleAction *animation;
};

#endif
