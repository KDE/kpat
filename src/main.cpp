/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
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

// own
#include "dealer.h"
#include "dealerinfo.h"
#include "kpat_debug.h"
#include "kpat_version.h"
#include "mainwindow.h"
#include "patsolve/solverinterface.h"
// KCardGame
#include <KCardDeck>
#include <KCardTheme>
// KF
#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
// Qt
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDomDocument>
#include <QElapsedTimer>
#include <QFile>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTime>
// Std
#include <climits>

static DealerScene *getDealer(int wanted_game, const QString &name)
{
    const auto games = DealerInfoList::self()->games();
    for (DealerInfo *di : games) {
        if ((wanted_game < 0) ? (di->untranslatedBaseName() == name) : di->providesId(wanted_game)) {
            DealerScene *d = di->createGame();
            Q_ASSERT(d);
            d->setDeck(new KCardDeck(KCardTheme(), d));
            d->initialize();

            if (!d->solver()) {
                qCCritical(KPAT_LOG) << "There is no solver for" << di->baseName();
                return nullptr;
            }

            return d;
        }
    }
    return nullptr;
}

KAboutData fillAboutData()
{
    KAboutData aboutData(QStringLiteral("kpat"),
                         i18n("KPatience"),
                         QStringLiteral(KPAT_VERSION_STRING),
                         i18n("KDE Patience Game"),
                         KAboutLicense::GPL_V2,
                         i18n("© 1995 Paul Olav Tvete\n© 2000 Stephan Kulow"),
                         QString(),
                         QStringLiteral("https://apps.kde.org/kpat"));

    aboutData.addAuthor(i18n("Paul Olav Tvete"), i18n("Author of original Qt version"), QStringLiteral("paul@troll.no"));
    aboutData.addAuthor(i18n("Mario Weilguni"), i18n("Initial KDE port"), QStringLiteral("mweilguni@kde.org"));
    aboutData.addAuthor(i18n("Matthias Ettrich"), QString(), QStringLiteral("ettrich@kde.org"));
    aboutData.addAuthor(i18n("Rodolfo Borges"), i18n("New game types"), QStringLiteral("barrett@9hells.org"));
    aboutData.addAuthor(i18n("Peter H. Ruegg"), QString(), QStringLiteral("kpat@incense.org"));
    aboutData.addAuthor(i18n("Michael Koch"), i18n("Bug fixes"), QStringLiteral("koch@kde.org"));
    aboutData.addAuthor(i18n("Marcus Meissner"), i18n("Shuffle algorithm for game numbers"), QStringLiteral("mm@caldera.de"));
    aboutData.addAuthor(i18n("Tom Holroyd"), i18n("Initial patience solver"), QStringLiteral("tomh@kurage.nimh.nih.gov"));
    aboutData.addAuthor(i18n("Stephan Kulow"), i18n("Rewrite and current maintainer"), QStringLiteral("coolo@kde.org"));
    aboutData.addAuthor(i18n("Erik Sigra"), i18n("Klondike improvements"), QStringLiteral("sigra@home.se"));
    aboutData.addAuthor(i18n("Josh Metzler"), i18n("Spider implementation"), QStringLiteral("joshdeb@metzlers.org"));
    aboutData.addAuthor(i18n("Maren Pakura"), i18n("Documentation"), QStringLiteral("maren@kde.org"));
    aboutData.addAuthor(i18n("Inge Wallin"), i18n("Bug fixes"), QStringLiteral("inge@lysator.liu.se"));
    aboutData.addAuthor(i18n("Simon Hürlimann"), i18n("Menu and toolbar work"), QStringLiteral("simon.huerlimann@huerlisi.ch"));
    aboutData.addAuthor(i18n("Parker Coates"), i18n("Cleanup and polish"), QStringLiteral("coates@kde.org"));
    aboutData.addAuthor(i18n("Shlomi Fish"), i18n("Integration with Freecell Solver and further work"), QStringLiteral("shlomif@cpan.org"));
    aboutData.addAuthor(i18n("Michael Lang"), i18n("New game types"), QStringLiteral("criticaltemp@protonmail.com"));
    return aboutData;
}

void addParserOptions(QCommandLineParser &parser)
{
    parser.addOption(
        QCommandLineOption(QStringList() << QStringLiteral("solvegame"), i18n("Try to find a solution to the given savegame"), QStringLiteral("file")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("solve"), i18n("Dealer to solve (debug)"), QStringLiteral("num")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("start"), i18n("Game range start (default 0:INT_MAX)"), QStringLiteral("num")));
    parser.addOption(
        QCommandLineOption(QStringList() << QStringLiteral("end"), i18n("Game range end (default start:start if start given)"), QStringLiteral("num")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("testdir"), i18n("Directory with test cases"), QStringLiteral("directory")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("generate"), i18n("Generate random test cases")));
    parser.addPositionalArgument(QStringLiteral("file"), i18n("File to load"));
}

DealerScene *loadSaveGame(QString savegame)
{
    QFile of(savegame);
    of.open(QIODevice::ReadOnly);
    QDomDocument doc;
    doc.setContent(&of);

    DealerScene *dealer;
    QString id_attr = doc.documentElement().attribute(QStringLiteral("id"));
    if (!id_attr.isEmpty()) {
        dealer = getDealer(id_attr.toInt(), QString());
        if (!dealer)
            return nullptr;
        dealer->loadLegacyFile(&of);
    } else {
        of.seek(0);
        QXmlStreamReader xml(&of);
        if (!xml.readNextStartElement()) {
            qCritical() << "Failed to read XML" << savegame;
        }
        dealer = getDealer(DealerInfoList::self()->gameIdForFile(xml), QString());
        if (!dealer)
            return nullptr;

        of.seek(0);
        dealer->loadFile(&of, false);
    }
    return dealer;
}

bool singleSolve(QCommandLineParser &parser)
{
    QString savegame = parser.value(QStringLiteral("solvegame"));
    if (savegame.isEmpty())
        return false;

    DealerScene *dealer = loadSaveGame(savegame);
    if (!dealer) {
        // we tried
        return true;
    }
    dealer->solver()->translate_layout();

    int ret = dealer->solver()->patsolve();
    if (ret == SolverInterface::SolutionExists)
        fprintf(stdout, "won\n");
    else if (ret == SolverInterface::NoSolutionExists)
        fprintf(stdout, "lost\n");
    else
        fprintf(stdout, "unknown\n");

    delete dealer;
    return true;
}

bool generateDeal(DealerScene *dealer, int dealer_id, QString testdir)
{
    QElapsedTimer mytime;

    if (dealer->deck())
        dealer->deck()->stopAnimations();
    int game_id = QRandomGenerator::global()->bounded(INT_MAX);
    dealer->startNew(game_id);
    mytime.start();
    dealer->solver()->translate_layout();
    int ret = dealer->solver()->patsolve();
    if (ret == SolverInterface::SolutionExists) {
        fprintf(stdout, "%d: %d won (%lld ms)\n", dealer_id, game_id, mytime.elapsed());
        QFile file(QStringLiteral("%1/%2-%3-1").arg(testdir).arg(dealer_id).arg(game_id));
        file.open(QFile::WriteOnly);
        dealer->saveLegacyFile(&file);
        return true;
    } else if (ret == SolverInterface::NoSolutionExists) {
        fprintf(stdout, "%d: %d lost (%lld ms)\n", dealer_id, game_id, mytime.elapsed());
        QFile file(QStringLiteral("%1/%2-%3-0").arg(testdir).arg(dealer_id).arg(game_id));
        file.open(QFile::WriteOnly);
        dealer->saveLegacyFile(&file);
        return true;
    } else {
        fprintf(stdout, "%d: %d unknown (%lld ms)\n", dealer_id, game_id, mytime.elapsed());
        return false;
    }
}

bool generate(QCommandLineParser &parser)
{
    QString testdir = parser.value(QStringLiteral("testdir"));
    if (testdir.isEmpty())
        return false;
    if (!parser.isSet(QStringLiteral("generate")))
        return false;
    for (int dealer_id = 0; dealer_id < 20; dealer_id++) {
        DealerScene *dealer = getDealer(dealer_id, QString());
        if (!dealer)
            continue;
        int count = 100;
        while (count) {
            if (generateDeal(dealer, dealer_id, testdir))
                count--;
        }
        delete dealer;
    }
    return true;
}

bool determineRange(QCommandLineParser &parser, int &wanted_game, QString &wanted_name, int &start_index, int &end_index)
{
    wanted_game = -1;
    if (parser.isSet(QStringLiteral("solve"))) {
        wanted_name = parser.value(QStringLiteral("solve"));
        bool isInt = false;
        wanted_game = wanted_name.toInt(&isInt);
        if (!isInt)
            wanted_game = -1;
    } else {
        return false;
    }

    bool ok = false;
    end_index = -1;
    if (parser.isSet(QStringLiteral("end")))
        end_index = parser.value(QStringLiteral("end")).toInt(&ok);
    if (!ok)
        end_index = -1;
    ok = false;
    start_index = -1;
    if (parser.isSet(QStringLiteral("start")))
        start_index = parser.value(QStringLiteral("start")).toInt(&ok);
    if (!ok) {
        start_index = 0;
    }
    if (end_index == -1)
        end_index = start_index;

    return ok;
}

bool solveRange(QCommandLineParser &parser)
{
    int wanted_game, start_index, end_index;
    QString wanted_name;
    if (!determineRange(parser, wanted_game, wanted_name, start_index, end_index))
        return false;

    DealerScene *dealer = getDealer(wanted_game, wanted_name);
    if (!dealer)
        return true;

    QElapsedTimer mytime;
    for (int i = start_index; i <= end_index; i++) {
        mytime.start();
        dealer->deck()->stopAnimations();
        dealer->startNew(i);
        dealer->solver()->translate_layout();
        int ret = dealer->solver()->patsolve();
        if (ret == SolverInterface::SolutionExists)
            fprintf(stdout, "%d won (%lld ms)\n", i, mytime.elapsed());
        else if (ret == SolverInterface::NoSolutionExists)
            fprintf(stdout, "%d lost (%lld ms)\n", i, mytime.elapsed());
        else
            fprintf(stdout, "%d unknown (%lld ms)\n", i, mytime.elapsed());
    }
    fprintf(stdout, "all_moves %ld\n", all_moves);
    delete dealer;
    return true;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kpat"));

    KAboutData aboutData = fillAboutData();
    KAboutData::setApplicationData(aboutData);

    KCrash::initialize();

    QCommandLineParser parser;
    addParserOptions(parser);
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kpat")));

    if (singleSolve(parser)) {
        return 0;
    }

    if (generate(parser)) {
        return 0;
    }

    if (solveRange(parser)) {
        return 0;
    }

    QFile savedState(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1String("/" saved_state_file));

    MainWindow *w = new MainWindow;
    if (!parser.positionalArguments().isEmpty()) {
        if (!w->loadGame(QUrl::fromLocalFile(parser.positionalArguments().at(0)), true))
            w->slotShowGameSelectionScreen();
    } else if (savedState.exists()) {
        if (!w->loadGame(QUrl::fromLocalFile(savedState.fileName()), false))
            w->slotShowGameSelectionScreen();
    } else {
        w->slotShowGameSelectionScreen();
    }
    w->show();

    const KDBusService dbusService(KDBusService::Multiple);

    return app.exec();
}
