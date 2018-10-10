/*
 * shuffle_test.cpp
 * Copyright (C) 2018 Shlomi Fish <shlomif@cpan.org>
 *
 * Distributed under terms of the MIT license.
 */

#include <QTest>
#include "shuffle.h"

class TestQString: public QObject
{
    Q_OBJECT
private slots:
    void toUpper();
};

void TestQString::toUpper()
{
    QString str = "Hello";
    QCOMPARE(str.toUpper(), QString("HELLO"));
}

QTEST_MAIN(TestQString)
#include "shuffle_test.moc"
