#include "kfixedtopwidget.h"
#include "kfixedtopwidget.moc"
#include <stdio.h>

int nores = 0;

KFixedTopWidget::KFixedTopWidget(
                                  const char *name = NULL )
  : QWidget( 0L, name )
 {
   kmenubar = NULL;
   kmainwidget = NULL;
   kstatusbar = NULL;
   borderwidth = 1;

   kmainwidgetframe = new QFrame( this );
   CHECK_PTR( kmainwidgetframe );
   kmainwidgetframe ->setFrameStyle( QFrame::Panel | QFrame::Sunken);
   kmainwidgetframe ->setLineWidth(2);
   kmainwidgetframe ->hide();
 }

KFixedTopWidget::~KFixedTopWidget()
 {
 }

int KFixedTopWidget::addToolBar( KToolBar *toolbar, int index )
{
  if ( index == -1 )
    toolbars.append( toolbar );
  else
    toolbars.insert( index, toolbar );
  index = toolbars.at();
  connect ( toolbar, SIGNAL( moved (BarPosition) ),
            this, SLOT( updateRects() ) );
  updateRects();
  return index;
}

void KFixedTopWidget::setView( QWidget *view, bool show_frame )
{
  kmainwidget = view;
  if( show_frame ){

    // Set a default frame borderwidth, for a toplevelwidget with
    // frame.

    if(borderwidth == 0 )
      setFrameBorderWidth(1);

    kmainwidgetframe->show();
  }

  // In the case setView(...,TRUE),
  // we leave the default frame borderwith at 0 so that we don't get
  // an unwanted border -- after all we didn't request a frame. If you
  // still want a border ( though no frame, call setFrameBorderWidth()
  // before setView(...,FALSE).

}

void KFixedTopWidget::setMenu( KMenuBar *menu )
{
  kmenubar = menu;
  connect ( kmenubar, SIGNAL( moved (menuPosition) ),
            this, SLOT( updateRects() ) );
  
}

void KFixedTopWidget::setStatusBar( KStatusBar *statusbar )
{
  kstatusbar = statusbar;
}

void KFixedTopWidget::focusInEvent( QFocusEvent *)
{
  repaint( FALSE );
}

void KFixedTopWidget::focusOutEvent( QFocusEvent *)
{
  repaint( FALSE );
}

void KFixedTopWidget::updateRects()
{

  static int rec = 0;

  if(++rec == 20) {
    fprintf(stderr, "Recursion too deep, exiting\n");
    exit(0);
  }

  int t=0, b=0, l=0, r=0;
  int to=-1, bo=-1, lo=-1, ro=-1;
  int h = height();

  if( kmenubar && kmenubar->isVisible() )
	switch (kmenubar->menuBarPos())
	  {
	  case KMenuBar::Top:
		kmenubar->resize(width(), kmenubar->height());
		t+=kmenubar->height();
		kmenubar->move(0, 0);
		
		break;
	  case KMenuBar::Bottom:
		kmenubar->resize(width(), kmenubar->height());
		b+=kmenubar->height();
		kmenubar->move(0, height()-b);
		break;
	  case KMenuBar::Floating:
		break;
	  }

  /*
  if ( kmenubar && kmenubar->isVisible() )
    t += kmenubar->height(); // the menu is always on top. Oh, yeah?
  */
      
  if ( kstatusbar && kstatusbar->isVisible() )
   {
     kstatusbar->setGeometry(0, height() - b - kstatusbar->height(),
                             width(), kstatusbar->height());
     b += kstatusbar->height();
   }

  for ( KToolBar *toolbar = toolbars.first() ;
        toolbar != NULL ; toolbar = toolbars.next() )
    if ( toolbar->barPos() == KToolBar::Top && toolbar->isVisible() )
     {
       toolbar->updateRects (TRUE);     // Sven: You have to do this
       if ( to < 0 )
        {
          to = 0;
          t += toolbar->height();
        }
       if (to + toolbar->width() > width())
        {
          to = 0;
          t += toolbar->height();
        }
       toolbar->move( to, t-toolbar->height() );
       to += toolbar->width();
     }

  for ( KToolBar *toolbar = toolbars.first();
        toolbar != NULL; toolbar = toolbars.next() )
    if ( toolbar->barPos() == KToolBar::Bottom && toolbar->isVisible() ) {
      toolbar->updateRects (TRUE);   // Sven: You have to this
      if ( bo < 0 ) {
        bo = 0;
        b += toolbar->height();
      }
      if (bo + toolbar->width() > width()) {
        bo = 0;
        b += toolbar->height();
      }
      toolbar->move(bo, height()  - b );
      bo += toolbar->width();
    }

  h = height() - t - b;
  for ( KToolBar *toolbar = toolbars.first();
        toolbar != NULL; toolbar = toolbars.next() )
    if ( toolbar->barPos() == KToolBar::Left && toolbar->isVisible() ) {
      toolbar->setMaxHeight(h);   // Sven: You have to do this here
      toolbar->updateRects (TRUE);        // Sven: You have to this
      if ( lo < 0 ) {
        lo = 0;
        l += toolbar->width();
      }
      if ( lo + toolbar->height() > h) {
        lo = 0;
        l += toolbar->width();
      }
      toolbar->move( l-toolbar->width(), t  + lo );
      lo += toolbar->height();
    }

  for ( KToolBar *toolbar = toolbars.first();
        toolbar != NULL; toolbar = toolbars.next() )
    if ( toolbar->barPos() == KToolBar::Right && toolbar->isVisible() ) {
      toolbar->setMaxHeight(h);   // Sven: You have to do this here
      toolbar->updateRects (TRUE);   // Sven: You have to this
      if ( ro < 0 ) {
        ro = 0;
        r += toolbar->width();
      }
      if (ro + toolbar->height() > h) {
        ro = 0;
        r += toolbar->width();
      }
      toolbar->move(width() - r , t + ro);
      ro += toolbar->height();
    }

  view_left=0;
  view_right=width();
  view_top=0;
  view_bottom=height();

  for (KToolBar *toolbar = toolbars.first();
       toolbar != NULL; toolbar = toolbars.next()) {

    if ( toolbar->barPos() == KToolBar::Top && toolbar->isVisible() )
      view_top = toolbar->y() + toolbar->height();// - 2;
    else if ( kmenubar && kmenubar->isVisible() &&
              kmenubar->menuBarPos() == KMenuBar::Top) {
      if( view_top < kmenubar->height()/* - 2*/ )
        view_top = kmenubar->height();// - 2;
    } else if ( view_top < 0 )
      view_top = 0;

    if ( toolbar->barPos() == KToolBar::Bottom && toolbar->isVisible() )
      view_bottom = toolbar->y();// + 2;
    else if ( kstatusbar && kstatusbar->isVisible() ) {
      if ( view_bottom > kstatusbar->y() )
        view_bottom = kstatusbar->y();
    } else if ( view_bottom > height() )
      view_bottom = height();

    if ( toolbar->barPos() == KToolBar::Right && toolbar->isVisible() )
      view_right = toolbar->x();//+2;

    if ( toolbar->barPos() == KToolBar::Right && !toolbar->isVisible() )
      view_right = width();

    if ( toolbar->barPos() == KToolBar::Left && toolbar->isVisible() )
      view_left = toolbar->x()+toolbar->width();// - 2;

    if ( toolbar->barPos() == KToolBar::Left && !toolbar->isVisible() )
      view_left = 0;
  }

  //printf("l = %d, r = %d, t = %d, b = %d\n", view_left, view_right, view_top,
  //view_bottom);

  if (kstatusbar && kstatusbar->isVisible())
   {
     if ( view_bottom > kstatusbar->y() )
       view_bottom = kstatusbar->y();
   }
  else
   {
     if (kmenubar && kmenubar->isVisible())
       if (kmenubar->menuBarPos() == KMenuBar::Bottom)
         if ( view_bottom > kmenubar->y() )
           view_bottom = kmenubar->y();
   }

  if (kmenubar && kmenubar->isVisible())
   {
     if (kmenubar->menuBarPos() == KMenuBar::Top)
       if (view_top < kmenubar->height())
         view_top = kmenubar->height();
   }

  if (kmainwidget)
   {
     QSize s(view_right - view_left - 2 * borderwidth,
	     view_bottom - view_top - 2 * borderwidth );
     QWidget *w = kmainwidget;

     // check for fixed size widgets
     if(w->minimumSize() == w->maximumSize()) {
       QSize si = size();
       si.setWidth(si.width() - s.width() + w->minimumSize().width());
       si.setHeight(si.height() - s.height() + w->minimumSize().height());
       if(si != size()) {
	 nores = 1;		// prevent indef. recursion
	 setFixedSize(si);
	 nores = 0;
	 updateRects();
	 rec--;
	 return;
       }
     }

     kmainwidgetframe->setGeometry( view_left, view_top,
                                    view_right - view_left,
                                    view_bottom - view_top );

     kmainwidget->setGeometry( view_left + borderwidth, view_top + borderwidth,
                               view_right - view_left - 2 * borderwidth,
                               view_bottom - view_top - 2 * borderwidth );
   }

  rec--;
}

void KFixedTopWidget::setFrameBorderWidth(int size){

  borderwidth = size;

}

void KFixedTopWidget::resizeEvent( QResizeEvent * )
{
  //menu resizes themself
  if(nores == 0)
    updateRects();
}

KStatusBar *KFixedTopWidget::statusBar()
{
  return kstatusbar;
}

KToolBar *KFixedTopWidget::toolBar( int ID )
{
  return toolbars.at( ID );
}

void KFixedTopWidget::enableToolBar( KToolBar::BarStatus stat, int ID )
{
  KToolBar *t = toolbars.at( ID );
  if ( t )
    t->enable( stat );
  updateRects();
}

void KFixedTopWidget::enableStatusBar( KStatusBar::BarStatus stat )
{
  CHECK_PTR( kstatusbar );
  if ( ( stat == KStatusBar::Toggle && kstatusbar->isVisible() )
       || stat == KStatusBar::Hide )
    kstatusbar->hide();
  else
    kstatusbar->show();
  updateRects();
}


