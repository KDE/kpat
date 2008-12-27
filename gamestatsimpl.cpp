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
#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfiggroup.h>

GameStatsImpl::GameStatsImpl(QWidget* aParent)
	: KDialog(aParent),
	  indexMap()
{
	QWidget* widget = new QWidget(this);
	ui = new Ui::GameStats();
	ui->setupUi(widget);

	setWindowTitle(i18n("Statistics"));
	setMainWidget(widget);
	setButtons(KDialog::Reset | KDialog::Close);
	setDefaultButton(KDialog::Close);
	ui->GameType->setFocus();
	ui->GameType->setMaxVisibleItems(DealerInfoList::self()->games().size());

	QMap<QString,int> nameToIndex;
	foreach (DealerInfo* game, DealerInfoList::self()->games())
	{
		QString name = KGlobal::locale()->removeAcceleratorMarker(game->name);
		nameToIndex[name] = game->ids.first();
	}
	foreach (const QString& name, nameToIndex.keys())
	{
		// Map combobox indices to game indices
		indexMap[ui->GameType->count()] = nameToIndex[name];
		ui->GameType->addItem(name);
	}

	showGameType(indexMap[0]);

	connect(ui->GameType, SIGNAL(activated(int)), SLOT(selectionChanged(int)));
	connect(this, SIGNAL(resetClicked()), SLOT(resetStats()));
}

void GameStatsImpl::selectionChanged(int comboIndex)
{
	int gameIndex = indexMap[comboIndex];
	setGameType(gameIndex);
}

void GameStatsImpl::showGameType(int gameIndex)
{
	int comboIndex = indexMap.key(gameIndex);
	ui->GameType->setCurrentIndex(comboIndex);
	setGameType(gameIndex);
}

void GameStatsImpl::setGameType(int gameIndex)
{
	KConfigGroup cg(KGlobal::config(), scores_group);
	unsigned int t = cg.readEntry(QString("total%1").arg(gameIndex),0);
	ui->Played->setText(QString::number(t));
	unsigned int w = cg.readEntry(QString("won%1").arg(gameIndex),0);
	if (t)
		ui->Won->setText(i18n("%1 (%2%)", w, w*100/t));
	else
		ui->Won->setText( QString::number(w));
	ui->WinStreak->setText( QString::number( cg.readEntry(QString("maxwinstreak%1").arg(gameIndex), 0)));
	ui->LooseStreak->setText( QString::number( cg.readEntry(QString("maxloosestreak%1").arg(gameIndex), 0)));
	unsigned int l = cg.readEntry(QString("loosestreak%1").arg(gameIndex),0);
	if (l)
		ui->CurrentStreak->setText( i18np("1 loss", "%1 losses", l) );
	else
		ui->CurrentStreak->setText( i18np("1 win", "%1 wins",
	cg.readEntry(QString("winstreak%1").arg(gameIndex),0)) );
}

void GameStatsImpl::resetStats()
{
	int gameIndex = indexMap[ui->GameType->currentIndex()];
	Q_ASSERT(gameIndex >= 0);
	KConfigGroup cg(KGlobal::config(), scores_group);
	cg.writeEntry(QString("total%1").arg(gameIndex),0);
	cg.writeEntry(QString("won%1").arg(gameIndex),0);
	cg.writeEntry(QString("maxwinstreak%1").arg(gameIndex),0);
	cg.writeEntry(QString("maxloosestreak%1").arg(gameIndex),0);
	cg.writeEntry(QString("loosestreak%1").arg(gameIndex),0);
	cg.writeEntry(QString("winstreak%1").arg(gameIndex),0);
	cg.sync();

	setGameType(gameIndex);
}

#include "gamestatsimpl.moc"
