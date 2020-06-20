/*
 * Copyright (C) 2018 Shlomi Fish <shlomif@cpan.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <QTest>
#include "shuffle.h"

class TestCardsShuffle: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void shuffle_seed1();
    void shuffle_seed24();
};

typedef const char * Card;
typedef QList<Card> CardList;


const CardList input({"AC", "AD", "AH", "AS", "2C", "2D", "2H", "2S", "3C", "3D", "3H", "3S", "4C", "4D", "4H", "4S", "5C", "5D", "5H", "5S", "6C", "6D", "6H", "6S", "7C", "7D", "7H", "7S", "8C", "8D", "8H", "8S", "9C", "9D", "9H", "9S", "TC", "TD", "TH", "TS", "JC", "JD", "JH", "JS", "QC", "QD", "QH", "QS", "KC", "KD", "KH", "KS"}
        );

void TestCardsShuffle::shuffle_seed1()
{
    CardList have = KpatShuffle::shuffled<Card>(input, 1);
    CardList want(
        {"6H", "2H", "9C", "6S", "TC", "8C", "3D", "6C", "QS", "8D", "8S", "6D", "7D", "JH", "2C", "8H", "TH", "4S", "TD", "3S", "7S", "4D", "AC", "4H", "QH", "TS", "5C", "4C", "3C", "AH", "AS", "JS", "QD", "9D", "KS", "2S", "3H", "KH", "QC", "AD", "5S", "9S", "KC", "KD", "5H", "7C", "7H", "5D", "JC", "9H", "2D", "JD"}
    );
    QCOMPARE(have, want);
}

void TestCardsShuffle::shuffle_seed24()
{
    CardList have = KpatShuffle::shuffled<Card>(input, 24);
    CardList want(
        {"AS", "3D", "QD", "2H", "9D", "JD", "7C", "8D", "6D", "KS", "4H", "4S", "8S", "8H", "KC", "TD", "JH", "3S", "3H", "QS", "4D", "AD", "TS", "TC", "5C", "9H", "AC", "8C", "7D", "6S", "KH", "TH", "JC", "6H", "3C", "9C", "6C", "5S", "JS", "KD", "2S", "9S", "QH", "2C", "7S", "AH", "7H", "2D", "5D", "QC", "5H", "4C"}
    );
    QCOMPARE(have, want);
}

QTEST_MAIN(TestCardsShuffle)
#include "shuffle_test.moc"
