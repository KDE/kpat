/*
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
#ifndef GAMESTATS_IMPL_H_
#define GAMESTATS_IMPL_H_

#include <qdialog.h>
#include "ui_gamestats.h"

class GameStatsImpl : public QDialog, Ui::GameStats
{
    Q_OBJECT

	public:
		explicit GameStatsImpl(QWidget* aParent);

		void showGameType(int i);
        public slots:
		void setGameType(int i);
		void resetStats();
};

#endif

