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

#include "statisticsdialog.h"

#include "dealer.h"
#include "version.h"

#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KLocale>

#include <QtCore/QList>


StatisticsDialog::StatisticsDialog(QWidget* aParent)
	: KDialog(aParent),
	  indexToIdMap()
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

	QMap<QString,int> nameToIdMap;
	foreach (DealerInfo* game, DealerInfoList::self()->games())
		nameToIdMap.insert(QString(game->name), game->ids.first());

	QMap<QString,int>::const_iterator it = nameToIdMap.constBegin();
	QMap<QString,int>::const_iterator end = nameToIdMap.constEnd();
	for (; it != end; ++it)
	{
		// Map combobox indices to game IDs
		indexToIdMap[ui->GameType->count()] = it.value();
		ui->GameType->addItem(it.key());
	}

	showGameType(indexToIdMap[0]);

	connect(ui->GameType, SIGNAL(activated(int)), SLOT(selectionChanged(int)));
	connect(this, SIGNAL(resetClicked()), SLOT(resetStats()));
}

void StatisticsDialog::selectionChanged(int comboIndex)
{
	int gameIndex = indexToIdMap[comboIndex];
	setGameType(gameIndex);
}

void StatisticsDialog::showGameType(int gameIndex)
{
	int comboIndex = indexToIdMap.key(gameIndex);
	ui->GameType->setCurrentIndex(comboIndex);
	setGameType(gameIndex);
}

void StatisticsDialog::setGameType(int gameIndex)
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
	ui->LoseStreak->setText( QString::number( cg.readEntry(QString("maxloosestreak%1").arg(gameIndex), 0)));
	unsigned int l = cg.readEntry(QString("loosestreak%1").arg(gameIndex),0);
	if (l)
		ui->CurrentStreak->setText( i18np("1 loss", "%1 losses", l) );
	else
		ui->CurrentStreak->setText( i18np("1 win", "%1 wins",
	cg.readEntry(QString("winstreak%1").arg(gameIndex),0)) );
}

void StatisticsDialog::resetStats()
{
	int gameIndex = indexToIdMap[ui->GameType->currentIndex()];
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

#include "statisticsdialog.moc"

// kate: replace-tabs off; replace-tabs-save off
