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

#ifndef DEALERINFO_H
#define DEALERINFO_H

// Qt
#include <KLocalizedString>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QXmlStreamReader>
class DealerInfoList;
class DealerScene;

class DealerInfo
{
public:
    enum GameIds {
        KlondikeDrawOneId = 0,
        GrandfatherId = 1,
        AcesUpId = 2,
        FreecellGeneralId = 3,
        CastleGeneralId = 4,
        Mod3Id = 5,
        GypsyId = 7,
        FortyAndEightId = 8,
        SimpleSimonId = 9,
        YukonId = 10,
        GrandfathersClockId = 11,
        GolfId = 12,
        KlondikeDrawThreeId = 13,
        SpiderOneSuitId = 14,
        SpiderTwoSuitId = 15,
        SpiderFourSuitId = 16,
        SpiderGeneralId = 17,
        KlondikeGeneralId = 18,
        BakersDozenGeneralId = 19,
        BakersDozenId = 20,
        BakersDozenSpanishId = 21,
        BakersDozenCastlesId = 22,
        BakersDozenPortugueseId = 23,
        BakersDozenCustomId = 24,
        FreecellId = 30,
        FreecellBakersId = 31,
        FreecellEightOffId = 32,
        FreecellForeId = 33,
        FreecellSeahavenId = 34,
        FreecellCustomId = 39,
        CastleBeleagueredId = 40,
        CastleCitadelId = 41,
        CastleExiledKingsId = 42,
        CastleStreetAlleyId = 43,
        CastleSiegecraftId = 44,
        CastleStrongholdId = 45,
        CastleCustomId = 49,
        SpiderThreeSuitId = 50
    };

    DealerInfo(const KLocalizedString &untranslatedBaseName, int baseId, const QMap<int, KLocalizedString> &subTypes = {});
    virtual ~DealerInfo();

    QString baseName() const;
    QString untranslatedBaseName() const;
    QString baseIdString() const;
    int baseId() const;

    QList<int> subtypeIds() const;

    QList<int> distinctIds() const;
    bool providesId(int id) const;
    QString nameForId(int id) const;

    virtual DealerScene *createGame() const = 0;

private:
    const KLocalizedString m_baseName;
    const QString m_baseIdString;
    const int m_baseId;

    const QMap<int, KLocalizedString> m_subtypes;
};

class DealerInfoList
{
private:
    friend class DealerInfoListPrivate;
    explicit DealerInfoList();
    virtual ~DealerInfoList();

public:
    static DealerInfoList *self();
    void add(DealerInfo *di);
    const QList<DealerInfo *> games() const;
    int gameIdForFile(QXmlStreamReader &xml) const;

private:
    QList<DealerInfo *> m_list;
};

#endif
