/* Copyright (c) 2019 Shlomi Fish
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include <QProcess>
#include <QRegularExpression>
#include <QTest>

class TestSolver: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void runSolver();
};

void TestSolver::runSolver()
{
    QProcess kpat;
    kpat.start(QStringLiteral("../bin/kpat"), QStringList() << QStringLiteral("--start") << "1" << QStringLiteral("--end") << "3" << QStringLiteral("--solve") << QStringLiteral("Golf"));
    QCOMPARE(kpat.waitForFinished(), true);
    QCOMPARE(kpat.exitStatus(), QProcess::NormalExit);
    QCOMPARE(kpat.exitCode(), 0);
    auto bytes = kpat.readAllStandardOutput();
    QCOMPARE(QRegularExpression("\\n2 won \\(").match(QString(bytes)).hasMatch(), true);
}

QTEST_MAIN(TestSolver)
#include "golf_solver_wins.moc"
