#ifndef GAMESTATS_IMPL_H_
#define GAMESTATS_IMPL_H_

#include "gamestats.h"

class GameStatsImpl : public GameStats
{
	public:
		GameStatsImpl(QWidget* aParent, const char* aname);

		virtual void setGameType(int i);
		virtual void showGameType(int i);
};

#endif

