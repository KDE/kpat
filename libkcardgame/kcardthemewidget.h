/*
 *  Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#ifndef KCARDTHEMEWIDGET_H
#define KCARDTHEMEWIDGET_H

#include "libkcardgame_export.h"

#include <KConfigDialog>
#include <KConfigSkeleton>
#include <QWidget>


class LIBKCARDGAME_EXPORT KCardThemeWidget : public QWidget
{
public:
    explicit KCardThemeWidget( const QSet<QString> & requiredFeatures, const QString & previewString, QWidget * parent = 0 );
    virtual ~KCardThemeWidget();

    void setCurrentSelection( const QString & dirName );
    QString currentSelection() const;

private:
    class KCardThemeWidgetPrivate * const d;
};


class LIBKCARDGAME_EXPORT KCardThemeDialog : public KConfigDialog
{
public:
    explicit KCardThemeDialog( QWidget * parent, KConfigSkeleton * config, const QSet<QString> & requiredFeatures, const QString & previewString );
    virtual ~KCardThemeDialog();

    static bool showDialog();
};

#endif
