/*
 *  Copyright 2010 Parker Coates <coates@kde.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of 
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef NUMBEREDDEALDIALOG_H
#define NUMBEREDDEALDIALOG_H

class KComboBox;
#include <QDialog>

#include <QtCore/QMap>
class QSpinBox;

class NumberedDealDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit NumberedDealDialog( QWidget * parent );
        void setGameType( int gameId );
        void setDealNumber( int dealNumber );

    public slots:
        virtual void setVisible( bool visible );

    signals:
        void dealChosen( int gameId, int dealNumber );

    private slots:
        void handleOkClicked();

    private:
        KComboBox * m_gameType;
        QSpinBox * m_dealNumber;
        QMap<int,int> m_indexToIdMap;
};

#endif

