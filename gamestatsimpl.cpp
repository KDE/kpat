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
#include "gamestatsimpl.h"
#include "dealer.h"
#include "version.h"

//Added by qt3to4:
#include <QList>

#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfiggroup.h>

GameStatsImpl::GameStatsImpl(QWidget* aParent)
	: QDialog(aParent),
	  indexMap()
{
	setupUi(this);

	foreach (DealerInfo * game, DealerInfoList::self()->games())
	{
		// Map combobox indices to game indices
		indexMap[GameType->count()] = game->gameindex;

		QString name = KGlobal::locale()->removeAcceleratorMarker(game->name);
		GameType->addItem(name);
	}

	showGameType(0);
	connect(buttonOk, SIGNAL(clicked(bool)), SLOT(accept()));
	connect(GameType, SIGNAL(activated(int)), SLOT(selectionChanged(int)));
	connect(buttonReset, SIGNAL(clicked(bool)), SLOT(resetStats()));
}

void GameStatsImpl::selectionChanged(int comboIndex)
{
	int gameIndex = indexMap[comboIndex];
	setGameType(gameIndex);
}

void GameStatsImpl::showGameType(int gameIndex)
{
	int comboIndex = indexMap.key(gameIndex);
	GameType->setCurrentIndex(comboIndex);
	setGameType(gameIndex);
}

void GameStatsImpl::setGameType(int gameIndex)
{
	KConfigGroup cg(KGlobal::config(), scores_group);
	unsigned int t = cg.readEntry(QString("total%1").arg(gameIndex),0);
	Played->setText(QString::number(t));
	unsigned int w = cg.readEntry(QString("won%1").arg(gameIndex),0);
	if (t)
		Won->setText(i18n("%1 (%2%)", w, w*100/t));
	else
		Won->setText( QString::number(w));
	WinStreak->setText( QString::number( cg.readEntry(QString("maxwinstreak%1").arg(gameIndex), 0)));
	LooseStreak->setText( QString::number( cg.readEntry(QString("maxloosestreak%1").arg(gameIndex), 0)));
	unsigned int l = cg.readEntry(QString("loosestreak%1").arg(gameIndex),0);
	if (l)
		CurrentStreak->setText( i18np("1 loss", "%1 losses", l) );
	else
		CurrentStreak->setText( i18np("1 win", "%1 wins",
			cg.readEntry(QString("winstreak%1").arg(gameIndex),0)) );
}

void GameStatsImpl::resetStats()
{
	int id = GameType->currentIndex();
	Q_ASSERT(id >= 0);
	KConfigGroup cg(KGlobal::config(), scores_group);
	cg.writeEntry(QString("total%1").arg(id),0);
	cg.writeEntry(QString("won%1").arg(id),0);
	cg.writeEntry(QString("maxwinstreak%1").arg(id),0);
	cg.writeEntry(QString("maxloosestreak%1").arg(id),0);
	cg.writeEntry(QString("loosestreak%1").arg(id),0);
	cg.writeEntry(QString("winstreak%1").arg(id),0);

	setGameType(id);
}

#include "gamestatsimpl.moc"
