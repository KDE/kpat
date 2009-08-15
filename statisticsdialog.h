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

#ifndef GAMESTATSIMPL_H
#define GAMESTATSIMPL_H

#include "ui_statisticsdialog.h"

#include <KDialog>

#include <QtCore/QMap>


class StatisticsDialog : public KDialog
{
	Q_OBJECT

	public:
		explicit StatisticsDialog(QWidget* aParent);

		void showGameType(int gameIndex);
	public slots:
		void setGameType(int gameIndex);
		void resetStats();

	private slots:
		void selectionChanged(int comboIndex);

	private:
		Ui::GameStats * ui;
		QMap<int,int> indexToIdMap;
};

#endif

// kate: replace-tabs off; replace-tabs-save off
