#ifndef GAMESTATS_IMPL_H_
#define GAMESTATS_IMPL_H_

#include <qdialog.h>
#include "ui_gamestats.h"

class GameStatsImpl : public QDialog, Ui::GameStats
{
	public:
		GameStatsImpl(QWidget* aParent);

		void setGameType(int i);
		void showGameType(int i);
};

#endif

