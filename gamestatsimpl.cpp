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
	: QDialog(aParent)
{
	setupUi(this);
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
	GameType->addItems(list);
	showGameType(0);
	connect(buttonOk, SIGNAL(clicked(bool)), SLOT(accept()));
        connect(GameType, SIGNAL(activated(int)), SLOT(setGameType(int)));
}

void GameStatsImpl::showGameType(int id)
{
	GameType->setCurrentIndex(id);
	setGameType(id);
}

void GameStatsImpl::setGameType(int id)
{
	KConfigGroup cg(KGlobal::config(), scores_group);
	unsigned int t = cg.readEntry(QString("total%1").arg(id),0);
	Played->setText(QString::number(t));
	unsigned int w = cg.readEntry(QString("won%1").arg(id),0);
	if (t)
		Won->setText(i18n("%1 (%2%)", w, w*100/t));
	else
		Won->setText( QString::number(w));
	WinStreak->setText( QString::number( cg.readEntry(QString("maxwinstreak%1").arg(id), 0)));
	LooseStreak->setText( QString::number( cg.readEntry(QString("maxloosestreak%1").arg(id), 0)));
	unsigned int l = cg.readEntry(QString("loosestreak%1").arg(id),0);
	if (l)
		CurrentStreak->setText( i18np("1 loss", "%1 losses", l) );
	else
		CurrentStreak->setText( i18np("1 win", "%1 wins",
			cg.readEntry(QString("winstreak%1").arg(id),0)) );
}

#include "gamestatsimpl.moc"
