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
#include <kstdaction.h>
#include <kaction.h>
#include <dealer.h>
#include <kdebug.h>
#include "cardmaps.h"
#include <kcarddialog.h>

pWidget::pWidget( const char* _name )
    : KMainWindow(0, _name), dill(0)
{
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
    QValueList<DealerInfo*>::ConstIterator it;
    uint max_type = 0;

    for (it = DealerInfoList::self()->games().begin();
         it != DealerInfoList::self()->games().end(); ++it)
    {
        // while we develop, it may happen that some lower
        // indices do not exist
        uint index = (*it)->gameindex;
        for (uint i = 0; i <= index; i++)
            if (list.count() <= i)
                list.append("unknown");
        list[index] = i18n((*it)->name);
        kdDebug() << "add " << list[index] << endl;
        if (max_type < index)
            max_type = index;
    }
    games->setItems(list);

    /*

    backs = new KSelectAction(i18n("&Card backside"), 0, this,
                              SLOT(changeBackside()),
                              actionCollection(), "backside");

    list.clear();
    list.append(i18n( "KDE" ));
    list.append(i18n( "Classic blue" ));
    list.append(i18n( "Classic red" ));
    list.append(i18n( "Technical" ));
    backs->setItems(list);

*/
    backs = new KAction(i18n("&Switch backside"), 0, this,
                        SLOT(changeBackside()),
                        actionCollection(), "backside");

    animation = new KToggleAction(i18n( "&Animation on startup" ),
                                  0, this, SLOT(animationChanged()),
                                  actionCollection(), "animation");

    KConfig *config = kapp->config();
    KConfigGroupSaver cs(config, settings_group );
    QString bg = config->readEntry( "Back", KCardDialog::getDefaultDeck());
    setBackSide( bg );

    bool animate = config->readBoolEntry( "Animation", true);
    animation->setChecked( animate );

    uint game = config->readNumEntry("DefaultGame", 0);
    if (game > max_type)
        game = max_type;
    kdDebug() << "DefaultGame " << game << " " << max_type << endl;
    games->setCurrentItem(game);
    kdDebug() << "DefaultGame " << games->currentItem() << endl;

    createGUI(QString::null, false);

    newGameType();

    setCentralWidget(dill);
    resize(700, 400);
}

void pWidget::undoMove() {
    if( dill )
        dill->undo();
}

void pWidget::changeBackside() {
    KConfig *config = kapp->config();
    KConfigGroupSaver kcs(config, settings_group);

    QString deck = config->readEntry("Back");
    QString dummy;
    if (KCardDialog::getCardDeck(deck, dummy, this, KCardDialog::NoCards) == QDialog::Accepted)
    {
        setBackSide(deck);
    }
}

void pWidget::animationChanged() {
    bool anim = animation->isChecked();
    KConfig *config = kapp->config();
    KConfigGroupSaver cs(config, settings_group );
    config->writeEntry( "Animation", anim);
}

void pWidget::restart() {
    dill->restart();
}

void pWidget::newGameType()
{
    delete dill;
    dill = 0;

    uint id = games->currentItem();
    for (QValueList<DealerInfo*>::ConstIterator it = DealerInfoList::self()->games().begin(); it != DealerInfoList::self()->games().end(); ++it) {
        if ((*it)->gameindex == id) {
            dill = (*it)->createGame(this);
            break;
        }
    }

    if (!dill) {
        kdError() << "unimplemented game type " << id << endl;
        dill = DealerInfoList::self()->games().first()->createGame(this);
    }
    QString name = games->currentText();
    QString newname;
    for (uint i = 0; i < name.length(); i++)
        if (name.at(i) != QChar('&'))
            newname += name.at(i);

    setCaption( newname );

    KConfig *config = kapp->config();
    KConfigGroupSaver kcs(config, settings_group);
    config->writeEntry("DefaultGame", id);

    dill->show();
}

void pWidget::setBackSide(const QString &id)
{
    KConfig *config = kapp->config();
    KConfigGroupSaver kcs(config, settings_group);
    QPixmap pm(id);
    if(!pm.isNull()) {
        cardMap::self()->setBackSide(pm);
        config->writeEntry("Back", id);
    } else
        KMessageBox::sorry(this,
                           i18n("Could not load background image!"));

    if (dill) {
        dill->canvas()->setAllChanged();
        dill->canvas()->update();
    }
}

#include "pwidget.moc"

