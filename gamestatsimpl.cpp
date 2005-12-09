#include "gamestatsimpl.h"
#include "dealer.h"
#include "version.h"

#include <qcombobox.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QList>

#include <kapplication.h>
#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>

GameStatsImpl::GameStatsImpl(QWidget* aParent, const char* aname)
	: GameStats(aParent, aname)
{
	QStringList list;
	QList<DealerInfo*>::ConstIterator it;
	for (it = DealerInfoList::self()->games().begin();
			it != DealerInfoList::self()->games().end(); ++it)
	{
		// while we develop, it may happen that some lower
		// indices do not exist
		int index = (*it)->gameindex;
		for (int i = 0; i <= index; i++)
			if (list.count() <= i)
				list.append("unknown");
		list[index] = i18n((*it)->name);
		list[index].replace('&',"");
	}
	GameType->insertStringList(list);
	showGameType(0);
}

void GameStatsImpl::showGameType(int id)
{
	GameType->setCurrentItem(id);
	setGameType(id);
}

void GameStatsImpl::setGameType(int id)
{
	// Trick to reset string to original value
	languageChange();
	KConfigGroup cg(KGlobal::config(), scores_group);
	unsigned int t = cg.readUnsignedNumEntry(QString("total%1").arg(id),0);
	Played->setText(Played->text().arg(t));
	unsigned int w = cg.readUnsignedNumEntry(QString("won%1").arg(id),0);
	Won->setText(Won->text().arg(w));
	if (t)
		WonPerc->setText(WonPerc->text().arg(w*100/t));
	else
		WonPerc->setText(WonPerc->text().arg(0));
	WinStreak->setText(
		WinStreak->text().arg(cg.readUnsignedNumEntry(QString("maxwinstreak%1").arg(id),0)));
	LooseStreak->setText(
		LooseStreak->text().arg(cg.readUnsignedNumEntry(QString("maxloosestreak%1").arg(id),0)));
}
