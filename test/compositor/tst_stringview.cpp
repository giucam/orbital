
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
        QCOMPARE(result, QLatin1String(""));
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
}

void TstStringView::testCompare()
{
    std::string stdStr("foobarñØ↑");
    QVERIFY(StringView(stdStr) == stdStr);

    const char *cStr = "foobar@ł€¶ŧ←←↓→øþłµ¢“ñ“æ";
    QVERIFY(StringView(cStr) == cStr);
}

QTEST_MAIN(TstStringView)
#include "tst_stringview.moc"
