
#include <QObject>
#include <QtTest/QtTest>

#include "utils.h"

using namespace Orbital;

class TstMaybe : public QObject
{
    Q_OBJECT
private slots:
    void test();
};

void TstMaybe::test()
{
    Maybe<int> m;
    QVERIFY(!m);
    QVERIFY(!m.isSet());

    m = 0;
    QVERIFY(m);
    QVERIFY(m.isSet());
    QCOMPARE(m.value(), 0);

    auto m2 = m;
    m.reset();
    QVERIFY(m2);
    QCOMPARE(m2.value(), 0);
    QVERIFY(!m);
}

QTEST_MAIN(TstMaybe)
#include "tst_maybe.moc"
