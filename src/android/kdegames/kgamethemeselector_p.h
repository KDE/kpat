/*
    SPDX-FileCopyrightText: 2009-2012 Stefan Majewsky <majewsky@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KG_THEMESELECTOR_P_H
#define KG_THEMESELECTOR_P_H

// Qt
#include <QStyledItemDelegate>

class KGameThemeDelegate : public QStyledItemDelegate
{
public:
    enum Roles {
        DescriptionRole = Qt::UserRole,
        AuthorRole,
        AuthorEmailRole,
        IdRole // not displayed, but used internally
    };

    explicit KGameThemeDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    /// @note The implementation is independent of @a option and @a index.
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QRect thumbnailRect(QRect baseRect) const;
};

#endif // KG_THEMESELECTOR_P_H
