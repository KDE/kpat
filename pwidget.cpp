/*
   patience -- main program
     Copyright (C) 1995  Paul Olav Tvete

   Permission to use, copy, modify, and distribute this software and its
   documentation for any purpose and without fee is hereby granted,
   provided that the above copyright notice appear in all copies and that
   both that copyright notice and this permission notice appear in
   supporting documentation.

   This file is provided AS IS with no warranties of any kind.  The author
   shall have no liability with respect to the infringement of copyrights,
   trade secrets or any patents by this file or any part thereof.  In no
   event will the author be liable for any lost revenue or profits or
   other special, indirect and consequential damages.


   Heavily modified by Mario Weilguni <mweilguni@sime.com>
*/

#include <stdio.h>

#include <qregexp.h>
#include <qobjcoll.h>
#include <kmessagebox.h>

#include <kapp.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <klocale.h>
#include "pwidget.h"
#include "version.h"
#include "basiccard.h"
#include <kstdaction.h>
#include <kaction.h>
#include <dealer.h>
#include <kdebug.h>

#include "computation.h"
#include "freecell.h"
#include "grandf.h"
#include "idiot.h"
#include "klondike.h"
#include "microsol.h"
#include "mod3.h"
#include "napoleon.h"
#include "ten.h"

pWidget::pWidget( const char* _name )
    : KMainWindow(0, _name)
{
    type = PT_NONE;
    dill = 0;

    KStdAction::quit(kapp, SLOT(quit()), actionCollection(), "game_exit");

    KStdAction::undo(this, SLOT(undoMove()),
                     actionCollection(), "undo_move");
    KAction *restart = KStdAction::openNew(this, SLOT(restart()),
                                           actionCollection(), "restart_game");
    restart->setText(i18n("&Restart game"));

    games = new KSelectAction(i18n("&Game Type"), 0, this,
                              SLOT(newGameType()),
                              actionCollection(), "game_type");
    QStringList list;
    list.append( i18n( "&Klondike" ) );
    list.append( i18n( "MicroSolitaire" ));
    list.append( i18n( "Calculation" ));
    list.append( i18n( "Napoleon's Tomb" ));
    list.append( i18n( "The Idiot" ));
    list.append( i18n( "Grandfather" ));
    list.append( i18n( "Ten"));
    list.append( i18n( "Mod3"));
    list.append( i18n( "Freecell"));
    games->setItems(list);

    backs = new KSelectAction(i18n("&Card backside"), 0, this,
                              SLOT(changeBackside()),
                              actionCollection(), "backside");

    list.clear();
    list.append(i18n( "KDE" ));
    list.append(i18n( "Classic blue" ));
    list.append(i18n( "Classic red" ));
    list.append(i18n( "Technical" ));
    backs->setItems(list);

    animation = new KToggleAction(i18n( "&Animation on startup" ),
                                  0, this, SLOT(animationChanged()),
                                  actionCollection(), "animation");


    KConfig *config = kapp->config();
    KConfigGroupSaver cs(config, settings_group );
    config->setGroup( settings_group );
    int bg = config->readNumEntry( "Backside", 0 );
    setBackSide( bg );

    bool animate = config->readBoolEntry( "Animation", true);
    animation->setChecked( animate );

    actionNewGame( PT_GRANDFATHER );
    createGUI();
}

void pWidget::undoMove() {
    if( dill )
        dill->undo();
}

void pWidget::changeBackside() {
    setBackSide(backs->currentItem());
}

void pWidget::animationChanged() {
    bool anim = animation->isChecked();
    KConfig *config = kapp->config();
    KConfigGroupSaver cs(config, settings_group );
    config->writeEntry( "Animation", anim);
}

void pWidget::newGameType() {
    kdDebug() << "newGameType\n";
}

void pWidget::restart() {
    dill->restart();
}

void pWidget::actionNewGame(int _id )
{
    delete dill;
    dill = 0;

    switch( _id ) {
    case PT_TEN:
        dill = new Ten(this);
        break;

    case PT_MOD3:
        dill = new Mod3(this);
        break;

    case PT_FREECELL:
        dill = new Freecell(this);
        break;

    case PT_GRANDFATHER:
        dill = new Grandf(this);
        break;

    case PT_KLONDIKE:
        dill = new Klondike(this);
        break;

    case PT_MSOLITAIRE:
        dill = new MicroSolitaire(this);
        break;

    case PT_COMPUTATION:
        dill = new Computation(this);
        break;

    case PT_IDIOT:
        dill = new Idiot(this);
        break;

    case PT_NAPOLEON:
        dill = new Napoleon(this);
        break;

    default:
        kdFatal() << "unimplemented game type " << type << endl;
        break;
    }

    dill->show();
    setCentralWidget(dill);

    setCaption( kapp->caption() );

    setDefaultType();
}

void pWidget::setDefaultType()
{
    if(type != PT_NONE) {
        KConfig *config = kapp->config();
        KConfigGroupSaver kcs(config, settings_group);
        config->writeEntry("DefaultGame", type);
    }
}

int pWidget::getDefaultType()
{
    KConfig *config = kapp->config();
    KConfigGroupSaver kcs(config, settings_group);
    return config->readNumEntry("DefaultGame", PT_KLONDIKE);
}

void pWidget::setBackSide(int id)
{
    KConfig *config = kapp->config();
    KConfigGroupSaver kcs(config, settings_group);
    QPixmap pm = BarIcon(QString("back%1").arg(id));
    if(!pm.isNull()) {
        cardMap::self()->setBackSide(pm);
        config->writeEntry("Backside", id);
    } else
        KMessageBox::sorry(this,
                           i18n("Could not load background image!"));

    backs->setCurrentItem(id);
    if (dill)
        dill->repaintCards();

}

#include "pwidget.moc"

