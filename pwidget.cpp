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

#include "pwidget.h"
#include "version.h"
#include "global.h"
#include "basiccard.h"

// menu constants
// file menu
#define ID_FQUIT	101

// game menu
#define ID_GUNDO		201
#define ID_GRESTART		202
#define ID_GNEWTYPE		203
#define ID_GNKLONDIKE		(ID_GNEWTYPE + PT_KLONDIKE)
#define ID_GNMSOLITAIRE		(ID_GNEWTYPE + PT_MSOLITAIRE)
#define ID_GNIDIOT		(ID_GNEWTYPE + PT_IDIOT)
#define ID_GNGRANDFATHER	(ID_GNEWTYPE + PT_GRANDFATHER)
#define ID_GNNAPOLEON		(ID_GNEWTYPE + PT_NAPOLEON)
#define ID_GNTEN		(ID_GNEWTYPE + PT_TEN)
#define ID_GNMOD3		(ID_GNEWTYPE + PT_MOD3)
#define ID_GNFREECELL		(ID_GNEWTYPE + PT_FREECELL)
#define ID_GNCOMPUTATION	(ID_GNEWTYPE + PT_COMPUTATION)

// option menu
#define ID_OBACK	801
#define ID_OAFTERBACK	820
#define ID_OANIMATION	821


#define ID_HHELP	100	

pWidget::pWidget( const char* _name ) 
  : KTMainWindow(_name)
{
  type = PT_NONE;
  dill = 0;

  m = new KMenuBar( this );
  connect( m, SIGNAL( activated( int ) ), this, SLOT( action( int ) ) );

  QPopupMenu *p1 = new QPopupMenu;
  p1->insertItem( i18n( "&Quit" ), ID_FQUIT );

  m->insertItem( i18n( "&File" ), p1 );

  QPopupMenu *p2 = new QPopupMenu;
  m->insertItem( i18n( "&Game" ), p2 );

  p2->insertItem( i18n( "&Undo"), ID_GUNDO );
  p2->insertItem( i18n( "&Restart game"), ID_GRESTART );

  QPopupMenu *popup = new QPopupMenu;

  popup->insertItem( i18n( "&Klondike" ), ID_GNKLONDIKE );
  popup->insertItem( i18n( "MicroSolitaire" ), ID_GNMSOLITAIRE );
  popup->insertItem( i18n( "Calculation" ), ID_GNCOMPUTATION );
  popup->insertItem( i18n( "Napoleon's Tomb" ), ID_GNNAPOLEON );
  popup->insertItem( i18n( "The Idiot" ), ID_GNIDIOT );
  popup->insertItem( i18n( "Grandfather" ), ID_GNGRANDFATHER);
  popup->insertItem( i18n( "Ten"), ID_GNTEN );
  popup->insertItem( i18n( "Mod3"), ID_GNMOD3 );
  popup->insertItem( i18n( "Freecell"), ID_GNFREECELL );

  p2->insertItem( i18n( "Choose new game" ), popup );

  // option menu
  QPopupMenu* m_opt = new QPopupMenu();
  m_opt->setCheckable( TRUE );
  QPopupMenu* m_opt_bg = new QPopupMenu();
  m_opt_bg->insertItem( i18n( "KDE" ), ID_OBACK);
  m_opt_bg->insertItem( i18n( "Classic blue" ), ID_OBACK + 1 );
  m_opt_bg->insertItem( i18n( "Classic red" ), ID_OBACK + 2 );
  m_opt_bg->insertItem( i18n( "Technical" ), ID_OBACK + 3 );
  m_opt->insertItem( i18n( "Card backside" ), m_opt_bg );
  m_opt->insertItem( i18n( "Animation on startup" ), ID_OANIMATION );
  m->insertItem( i18n( "&Options" ), m_opt );

  m->insertSeparator();

  QPopupMenu* help = helpMenu( i18n( "Patience" )
                                     + " " + KPAT_VERSION
                                     + i18n( "\n\nby Paul Olav Tvetei\n\n" )
                                     + i18n( "Additional work done by:\n" )
				     + "Rodolfo Borges (barrett@labma.ufrj.br)\n"
                                     + "Matthias Ettrich (ettrich@kde.org)\n"
                                     + "Mario Weilguni (mweilguni@sime.com)\n"
                                     + "Michael Koch (koch@kde.org)\n"
                                     + "Peter H. Ruegg (kpat@incense.org)");
  m->insertItem( i18n( "&Help" ), help );

  // create the toolbar
  tb = new KToolBar( this );

#define BarPosition
  connect( tb, SIGNAL( moved( BarPosition ) ), this, SLOT( slotToolbarChanged() ) );
#undef BarPosition

  tb->insertButton( BarIcon( "exit" ), ID_FQUIT, -1, i18n( "Quit" ) ); 
  tb->insertButton( BarIcon( "help" ), ID_HHELP, -1, i18n( "Help" ) );

  connect( tb, SIGNAL( clicked( int ) ), this, SLOT( action( int ) ) );

  // setup accelerators
  m->setAccel( CTRL + Key_K, ID_GNKLONDIKE );
  m->setAccel( CTRL + Key_M, ID_GNMSOLITAIRE );
  m->setAccel( CTRL + Key_C, ID_GNCOMPUTATION );
  m->setAccel( CTRL + Key_N, ID_GNNAPOLEON );
  m->setAccel( CTRL + Key_I, ID_GNIDIOT );
  m->setAccel( CTRL + Key_G, ID_GNGRANDFATHER );
  m->setAccel( CTRL + Key_T, ID_GNTEN );
  m->setAccel( CTRL + Key_3, ID_GNMOD3 );
  m->setAccel( CTRL + Key_F, ID_GNFREECELL );
  m->setAccel( CTRL + Key_Q, ID_FQUIT );
  m->setAccel( CTRL + Key_R, ID_GRESTART );
  m->setAccel( CTRL + Key_Z, ID_GUNDO );

  if( config )
  {
    config->setGroup( "General Settings" );
    int bg = config->readNumEntry( "Backside", 0 );
    setBackSide( bg );

    // move the toolbar to it's default position
    config->setGroup( "General Settings" );
    int tbpos = config->readNumEntry( "Toolbar_1_Position", 
				      (int) (KToolBar::Left) );
    tb->setBarPos( (KToolBar::BarPosition) (tbpos) );

    bool animate = (bool) (config->readNumEntry( "Animation", 1 ) != 0 );
    m->setItemChecked( ID_OANIMATION, animate );
  }

  setMenu( m );
  addToolBar( tb );
  m->show();
  tb->show();  

  // start default game
  actionNewGame( getDefaultType() + ID_GNEWTYPE );
  inInit = true;

}

void pWidget::action( int _id)
{
  bool animate;

  switch( _id )
  {
    case ID_FQUIT:
      delete this;
      kapp->quit();
      break;
    case ID_GUNDO:
      if( dill )
      {
        dill->undo();
      }
      break;
    case ID_GRESTART:
      if( dill )
      {
        dill->restart();
      }
      break;
    case ID_GNEWTYPE:
    case ID_GNKLONDIKE:
    case ID_GNMSOLITAIRE:
    case ID_GNIDIOT:
    case ID_GNGRANDFATHER:
    case ID_GNNAPOLEON:
    case ID_GNTEN:
    case ID_GNMOD3:
    case ID_GNFREECELL:
    case ID_GNCOMPUTATION:
      actionNewGame( _id );
      break;
    case ID_OANIMATION:
      animate = !(bool) (config->readNumEntry( "Animation", 1 ) != 0 );    
      config->setGroup( "General Settings" );
      config->writeEntry( "Animation", (int) animate );
      m->setItemChecked( ID_OANIMATION, animate );
      break;
    case ID_HHELP:
      KApplication::kApplication()->invokeHTMLHelp("", "");
      break;
  default:
    // background stuff?
    if( _id >= ID_OBACK &&
        _id < ID_OAFTERBACK)
    {
      setBackSide( _id - ID_OBACK );
    }
/*
    else
      fprintf(stderr, i18n( "COMMAND %d not implemented!\n" ), id );
*/
  }
}

void pWidget::actionNewGame(int _id )
{
  if( dill != 0 )
  {
    delete dill;
    dill = 0;
  }

  type = _id - ID_GNEWTYPE;
  switch( _id )
  {
    case ID_GNTEN:
      dill = new Ten(this, i18n("Ten").ascii());
      break;

    case ID_GNMOD3:
      dill = new Mod3(this, i18n("Mod3").ascii());
      break;
 
    case ID_GNFREECELL:
      dill = new Freecell(this, i18n("Freecell").ascii());
      break;

    case ID_GNGRANDFATHER:
      dill = new Grandf(this, i18n("Grandfather").ascii());
     break;

    case ID_GNKLONDIKE:
      dill = new Klondike(this, i18n("Klondike").ascii());
      break;

    case ID_GNMSOLITAIRE:
      dill = new MicroSolitaire(this, i18n("MicroSolitaire").ascii());
      break;

    case ID_GNCOMPUTATION:
      dill = new Computation(this, i18n("Calculation").ascii());
      break;

    case ID_GNIDIOT:
      dill = new Idiot(this, i18n("The Idiot").ascii());
      break;

    case ID_GNNAPOLEON:
      dill = new Napoleon(this, i18n("Napoleons Tomb").ascii());
      break;

    default:
      fprintf(stderr, 
	    i18n("kpat: unimplemented game type %d\n").ascii(),
  	    type);
      kapp->quit();
      break;
  }

  //dill->setFixedSize( dill->sizeHint() );
  dill->resize( dill->sizeHint() );
  dill->show();
  setView(dill);
  updateRects();

  setCaption( kapp->caption() );

  setDefaultType();  
}

void pWidget::focusInEvent(class QFocusEvent *)
{
  if (inInit) actionNewGame( getDefaultType() + ID_GNEWTYPE );
  inInit = FALSE;
}

void pWidget::setDefaultType()
{
  if(type != PT_NONE) {
    if(config != 0) {
      config->setGroup("General Settings");
      config->writeEntry("DefaultGame", type);
    }
  }
}

int pWidget::getDefaultType()
{
  if(config == 0)
    return PT_KLONDIKE;
  else {
    config->setGroup("General Settings");
    return config->readNumEntry("DefaultGame", PT_KLONDIKE);
  }
}

void pWidget::setBackSide(int id)
{
  if(cardmaps != 0) {
    if(id == 0) {
      cardmaps->setBackSide(0);
      config->setGroup("General Settings");
	config->writeEntry("Backside", 0);
    } else {
      QPixmap pm = BarIcon(QString("back%1").arg(id));
      if(pm.width() > 0 && pm.height() > 0) {
	cardmaps->setBackSide(&pm);
	config->setGroup("General Settings");
	config->writeEntry("Backside", id);
      } else
	KMessageBox::sorry(this, i18n("Could not load background image!"));
    }
  }

  // this hack is needed, update() is not enough
  QObjectList *ol = queryList("basicCard");
  QObjectListIt it( *ol );
  while(it.current()) {
    QWidget *w = (QWidget *)it.current();
    ++it;
    w->repaint(TRUE);
  }
  delete ol;
}

void pWidget::slotToolbarChanged()
{
  int p = (int)tb->barPos();
  config->setGroup("General Settings");
  config->writeEntry("Toolbar_1_Position", p);
}

#include "pwidget.moc"

