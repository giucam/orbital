
#include <QObject>
#include <QtTest/QtTest>

#include "stringview.h"

using namespace Orbital;

class TstStringView : public QObject
{
    Q_OBJECT
private slots:
    void testSplit();
    void testCompare();
    void testContains();
    void testIsNullOrEmpty();
};

void TstStringView::testSplit()
{
    {
        StringView view("a:bb:ccc:dddd");
        QString result;
        view.split(':', [&result](StringView substr) {
            result += substr.toQString();
            result += QLatin1String("|");
            return false;
        });
        QCOMPARE(result, QLatin1String("a|bb|ccc|dddd|"));
    }
    {
        StringView view("a:bb:ccc:dddd::");
        QString result;
        view.split(':', [&result](StringView substr) {
            result += substr.toQString();
            result += QLatin1String("|");
            return false;
        });
        QCOMPARE(result, QLatin1String("a|bb|ccc|dddd|"));
    }
    {
        StringView view(":");
        QString result;
        view.split(':', [&result](StringView substr) {
            result += substr.toQString();
            result += QLatin1String("|");
            return false;
        });
        QCOMPARE(result, QLatin1String(""));
    }
    {
        StringView view("");
        QString result;
        view.split(':', [&result](StringView substr) {
            result += substr.toQString();
            result += QLatin1String("|");
            return false;
        });
        QCOMPARE(result, QLatin1String(""));
    }
    {
        StringView view("aaa");
        QString result;
        view.split(':', [&result](StringView substr) {
            result += substr.toQString();
            result += QLatin1String("|");
            return false;
        });
        QCOMPARE(result, QLatin1String("aaa|"));
    }
    {
        StringView view("::a::");
        QString result;
        view.split(':', [&result](StringView substr) {
            result += substr.toQString();
            result += QLatin1String("|");
            return false;
        });
        QCOMPARE(result, QLatin1String("a|"));
    }
    {
        StringView view("a↑b³b¼↑¹cc²");
        QString result;
        view.split(L'↑', [&result](StringView substr) {
            result += substr.toQString();
            result += QLatin1String("|");
            return false;
        });
        QCOMPARE(result, QString::fromUtf8("a|b³b¼|¹cc²|"));
    }
}

void TstStringView::testCompare()
{
    std::string stdStr("foobarñØ↑");
    QVERIFY(StringView(stdStr) == stdStr);

    const char *cStr = "foobar@ł€¶ŧ←←↓→øþłµ¢“ñ“æ";
    QVERIFY(StringView(cStr) == cStr);
}

void TstStringView::testContains()
{
    StringView view("foo↑bar");
    QVERIFY(view.contains('r'));
    QVERIFY(!view.contains('p'));
    QVERIFY(view.contains(L'↑'));
    QVERIFY(!view.contains(L'→'));
}

void TstStringView::testIsNullOrEmpty()
{
    {
        StringView view;
        QVERIFY(view.isNull());
        QVERIFY(view.isEmpty());
    }

    {
        StringView view("default");
        QVERIFY(!view.isNull());
        QVERIFY(!view.isEmpty());
    }

    {
        StringView view("");
        QVERIFY(!view.isNull());
        QVERIFY(view.isEmpty());
    }

    {
        std::string src;
        StringView view(src);
        QVERIFY(!view.isNull());
        QVERIFY(view.isEmpty());
    }
}

QTEST_MAIN(TstStringView)
#include "tst_stringview.moc"
