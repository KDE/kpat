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

#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include "ui_statisticsdialog.h"

#include <QDialog>

#include <QtCore/QMap>


class StatisticsDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit StatisticsDialog(QWidget* aParent);
                ~StatisticsDialog();
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
