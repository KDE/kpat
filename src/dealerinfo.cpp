/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009 Parker Coates <coates@kde.org>
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

#include "dealerinfo.h"

static QString createBaseIdString(const KLocalizedString &baseName)
{
    QString result;

    const QString untranslatedBaseName = QString::fromUtf8(baseName.untranslatedText());
    result.reserve(untranslatedBaseName.size());
    for (QChar c : untranslatedBaseName) {
        if (c.isLetterOrNumber())
            result += c.toLower();
    }
    result.squeeze();
    return result;
}

DealerInfo::DealerInfo(const KLocalizedString &untranslatedBaseName, int baseId, const QMap<int, KLocalizedString> &subTypes)
    : m_baseName(untranslatedBaseName)
    , m_baseIdString(createBaseIdString(untranslatedBaseName))
    , m_baseId(baseId)
    , m_subtypes(subTypes)
{
    DealerInfoList::self()->add(this);
}

DealerInfo::~DealerInfo()
{
}

QString DealerInfo::baseName() const
{
    return m_baseName.toString();
}

QString DealerInfo::untranslatedBaseName() const
{
    return QString::fromUtf8(m_baseName.untranslatedText());
}

QString DealerInfo::baseIdString() const
{
    return m_baseIdString;
}

int DealerInfo::baseId() const
{
    return m_baseId;
}

QList<int> DealerInfo::subtypeIds() const
{
    return m_subtypes.keys();
}

QList<int> DealerInfo::distinctIds() const
{
    if (m_subtypes.isEmpty())
        return QList<int>() << m_baseId;
    else
        return m_subtypes.keys();
}

bool DealerInfo::providesId(int id) const
{
    return id == m_baseId || m_subtypes.contains(id);
}

QString DealerInfo::nameForId(int id) const
{
    if (id == m_baseId)
        return baseName();

    auto it = m_subtypes.find(id);
    if (it != m_subtypes.constEnd())
        return it.value().toString();
    else
        return QString();
}

class DealerInfoListPrivate
{
public:
    DealerInfoList instance;
};

Q_GLOBAL_STATIC(DealerInfoListPrivate, dilp)

DealerInfoList *DealerInfoList::self()
{
    return &(dilp->instance);
}

DealerInfoList::DealerInfoList()
{
}

DealerInfoList::~DealerInfoList()
{
}

void DealerInfoList::add(DealerInfo *di)
{
    m_list.append(di);
}

const QList<DealerInfo *> DealerInfoList::games() const
{
    return m_list;
}

int DealerInfoList::gameIdForFile(QXmlStreamReader &xml) const
{
    QStringView gameType = xml.attributes().value(QStringLiteral("game-type"));
    for (const DealerInfo *di : games()) {
        if (di->baseIdString() == gameType) {
            return di->baseId();
            break;
        }
    }
    return -1;
}
