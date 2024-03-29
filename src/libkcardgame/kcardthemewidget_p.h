/*
 *  Portions Copyright (C) 2009 by Davide Bettio <davide.bettio@kdemail.net>
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

#ifndef KCARDTHEMEWIDGET_P_H
#define KCARDTHEMEWIDGET_P_H

#include "kcardthemewidget.h"
// own
#include "kcardtheme.h"
// KF
#include <KImageCache>
// Qt
#include <QAbstractItemDelegate>
#include <QMutex>
#include <QSet>
#include <QThread>

class KLineEdit;
class QPushButton;
class QListView;

namespace KNSWidgets
{
class Button;
}

class PreviewThread : public QThread
{
    Q_OBJECT

public:
    PreviewThread(const KCardThemeWidgetPrivate *d, const QList<KCardTheme> &themes);
    void run() override;
    void halt();

Q_SIGNALS:
    void previewRendered(const KCardTheme &theme, const QImage &image);

private:
    const KCardThemeWidgetPrivate *const d;
    const QList<KCardTheme> m_themes;
    bool m_haltFlag;
    QMutex m_haltMutex;
};

class CardThemeModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit CardThemeModel(KCardThemeWidgetPrivate *d, QObject *parent = nullptr);
    virtual ~CardThemeModel();

    void reload();
    QModelIndex indexOf(const QString &dirName) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private Q_SLOTS:
    void deleteThread();
    void submitPreview(const KCardTheme &theme, const QImage &image);

private:
    const KCardThemeWidgetPrivate *const d;
    QMap<QString, KCardTheme> m_themes;
    QMap<QString, QPixmap *> m_previews;
    PreviewThread *m_thread;
};

class CardThemeDelegate : public QAbstractItemDelegate
{
public:
    explicit CardThemeDelegate(KCardThemeWidgetPrivate *d, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    const KCardThemeWidgetPrivate *const d;
};

class KCardThemeWidgetPrivate : public QObject
{
    Q_OBJECT

public:
    explicit KCardThemeWidgetPrivate(KCardThemeWidget *parent);

public Q_SLOTS:
    void updateLineEdit(const QModelIndex &index);
    void updateListView(const QString &dirName);

public:
    KCardThemeWidget *q;

    KImageCache *cache;

    CardThemeModel *model;
    QListView *listView;
    KLineEdit *hiddenLineEdit;
    KNSWidgets::Button *newDeckButton;

    int itemMargin;
    int textHeight;
    qreal abstractPreviewWidth;
    QSize baseCardSize;
    QSize previewSize;
    QSize itemSize;
    QString previewString;
    QList<QList<QString>> previewLayout;
    QSet<QString> requiredFeatures;
};

#endif
