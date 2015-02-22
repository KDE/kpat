/*
 * Copyright (C) 2004 Bart Vanhauwaert <bvh-cplusplus@irule.be>
 * Copyright (C) 2005-2008 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2008-2009 Parker Coates <coates@kde.org>
 *
 * License of original code:
 * -------------------------------------------------------------------------
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation.
 *
 *   This file is provided AS IS with no warranties of any kind.  The author
 *   shall have no liability with respect to the infringement of copyrights,
 *   trade secrets or any patents by this file or any part thereof.  In no
 *   event will the author be liable for any lost revenue or profits or
 *   other special, indirect and consequential damages.
 * -------------------------------------------------------------------------
 *
 * License of modifications/additions made after 2009-01-01:
 * -------------------------------------------------------------------------
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of 
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -------------------------------------------------------------------------
 */

#include "statisticsdialog.h"

#include "dealerinfo.h"
#include "version.h"

#include <KConfigGroup>
#include <QDebug>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QtCore/QList>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>


StatisticsDialog::StatisticsDialog(QWidget* aParent)
	: QDialog(aParent),
	  indexToIdMap()
{
	QWidget* widget = new QWidget(this);
	ui = new Ui::GameStats();
	ui->setupUi(widget);

	setWindowTitle(i18n("Statistics"));
	
        QVBoxLayout *mainLayout = new QVBoxLayout;
	setLayout(mainLayout);
	mainLayout->addWidget(widget);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close|QDialogButtonBox::Reset);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &StatisticsDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &StatisticsDialog::reject);
	mainLayout->addWidget(buttonBox);
	buttonBox->button(QDialogButtonBox::Close)->setDefault(true);


	QMap<QString,int> nameToIdMap;
	foreach ( DealerInfo * game, DealerInfoList::self()->games() )
		foreach ( int id, game->distinctIds() )
			nameToIdMap.insert( game->nameForId( id ), id );

	QMap<QString,int>::const_iterator it = nameToIdMap.constBegin();
	QMap<QString,int>::const_iterator end = nameToIdMap.constEnd();
	for (; it != end; ++it)
	{
		// Map combobox indices to game IDs
		indexToIdMap[ui->GameType->count()] = it.value();
		ui->GameType->addItem(it.key());
	}

	ui->GameType->setFocus();
	ui->GameType->setMaxVisibleItems(indexToIdMap.size());

	showGameType(indexToIdMap[0]);

	connect(ui->GameType, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &StatisticsDialog::selectionChanged);
	connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &StatisticsDialog::resetStats);
}

StatisticsDialog::~StatisticsDialog()
{
	delete ui;
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
	KConfigGroup cg(KSharedConfig::openConfig(), scores_group);
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
	KConfigGroup cg(KSharedConfig::openConfig(), scores_group);
	cg.writeEntry(QString("total%1").arg(gameIndex),0);
	cg.writeEntry(QString("won%1").arg(gameIndex),0);
	cg.writeEntry(QString("maxwinstreak%1").arg(gameIndex),0);
	cg.writeEntry(QString("maxloosestreak%1").arg(gameIndex),0);
	cg.writeEntry(QString("loosestreak%1").arg(gameIndex),0);
	cg.writeEntry(QString("winstreak%1").arg(gameIndex),0);
	cg.sync();

	setGameType(gameIndex);
}



// kate: replace-tabs off; replace-tabs-save off
