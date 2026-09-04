#include <QtTest/QtTest>
#include "../src/providers/CalculatorProvider.h"
#include "../src/providers/SettingsProvider.h"
#include "../src/providers/ActionProvider.h"
#include "../src/services/UsageHistory.h"
#include "../src/lua/LuaEngine.h"
#include "../src/lua/LuaPermissions.h"
#include "../src/lua/LuaPluginManager.h"

class TestMVSpotlight : public QObject
{
    Q_OBJECT

private slots:
    void testCalculatorValid();
    void testCalculatorInvalid();
    void testUsageHistoryFrecency();
    void testSettingsSearch();
    void testActionSearch();
    void testLuaEngineExecution();
    void testLuaEngineErrorCatching();
    void testLuaPermissions();
    void testLuaPluginManagerLoad();
    void testCurrencyPlugin();
};

void TestMVSpotlight::testCalculatorValid()
{
    CalculatorProvider calc;

    // 22 * 5 = 110
    auto res1 = calc.search("22 * 5");
    QVERIFY(!res1.isEmpty());
    QCOMPARE(res1.first().title(), QString("110"));

    // 1024 / 8 = 128
    auto res2 = calc.search("1024 / 8");
    QVERIFY(!res2.isEmpty());
    QCOMPARE(res2.first().title(), QString("128"));

    // (25 + 5) * 2 = 60
    auto res3 = calc.search("(25 + 5) * 2");
    QVERIFY(!res3.isEmpty());
    QCOMPARE(res3.first().title(), QString("60"));

    // sqrt(144) = 12
    auto res4 = calc.search("sqrt(144)");
    QVERIFY(!res4.isEmpty());
    QCOMPARE(res4.first().title(), QString("12"));
}

void TestMVSpotlight::testCalculatorInvalid()
{
    CalculatorProvider calc;
    // Words should not trigger calculator
    QVERIFY(calc.search("firefox").isEmpty());
    QVERIFY(calc.search("hello world").isEmpty());
    QVERIFY(calc.search("///").isEmpty());
}

void TestMVSpotlight::testUsageHistoryFrecency()
{
    UsageHistory &history = UsageHistory::instance();
    QString testId = "test:app:myapp";

    double initialBoost = history.frecencyBoost(testId);
    history.recordLaunch(testId);
    double boosted = history.frecencyBoost(testId);

    QVERIFY(boosted > initialBoost);
}

void TestMVSpotlight::testSettingsSearch()
{
    SettingsProvider settings;
    auto resWifi = settings.search("wifi");
    QVERIFY(!resWifi.isEmpty());
    QCOMPARE(resWifi.first().title(), QString("Wi-Fi"));

    auto resDisp = settings.search("display");
    QVERIFY(!resDisp.isEmpty());
}

void TestMVSpotlight::testActionSearch()
{
    ActionProvider actions;
    auto res = actions.search("lock");
    QVERIFY(!res.isEmpty());
    QCOMPARE(res.first().id(), QString("action:lock"));

    auto resTerm = actions.search("terminal");
    QVERIFY(!resTerm.isEmpty());
}

void TestMVSpotlight::testLuaEngineExecution()
{
    LuaEngine engine;
    QVERIFY(engine.init());

    QString error;
    bool ok = engine.executeString("local a = 10 + 20; return a", &error);
    QVERIFY2(ok, qPrintable(error));
}

void TestMVSpotlight::testLuaEngineErrorCatching()
{
    LuaEngine engine;
    QVERIFY(engine.init());

    QString error;
    // Syntax error
    bool okSyntax = engine.executeString("this is invalid lua code !@#$", &error);
    QVERIFY(!okSyntax);
    QVERIFY(!error.isEmpty());

    // Runtime error
    bool okRuntime = engine.executeString("error('Deliberate plugin error')", &error);
    QVERIFY(!okRuntime);
    QVERIFY(error.contains("Deliberate plugin error"));
}

void TestMVSpotlight::testLuaPermissions()
{
    QStringList perms = { "notifications", "clipboard.write" };
    LuaPermissions lp(perms);

    QVERIFY(lp.hasPermission(LuaPermissions::Notifications));
    QVERIFY(lp.hasPermission(LuaPermissions::ClipboardWrite));
    QVERIFY(!lp.hasPermission(LuaPermissions::ProcessExecute));
    QVERIFY(!lp.hasPermission(LuaPermissions::Network));
}

void TestMVSpotlight::testLuaPluginManagerLoad()
{
    LuaPluginManager manager;
    QVERIFY(manager.init());

    // Verify plugins loaded
    QVERIFY(manager.plugins().size() >= 5);

    // Search through UUID plugin
    auto uuidResults = manager.searchAll("uuid");
    QVERIFY(!uuidResults.isEmpty());
    QCOMPARE(uuidResults.first().type(), QString("Generator"));

    // Search through timestamp plugin
    auto timeResults = manager.searchAll("timestamp");
    QVERIFY(!timeResults.isEmpty());
    QCOMPARE(timeResults.first().type(), QString("Converter"));
}

void TestMVSpotlight::testCurrencyPlugin()
{
    LuaPluginManager manager;
    QVERIFY(manager.init());

    // 1. "299brl to usd"
    auto res1 = manager.searchAll("299brl to usd");
    QVERIFY(!res1.isEmpty());
    QVERIFY(res1.first().title().contains("USD"));
    QVERIFY(res1.first().subtitle().contains("299"));
    QVERIFY(res1.first().subtitle().contains("BRL"));

    // 2. "299 brl in usd"
    auto res2 = manager.searchAll("299 brl in usd");
    QVERIFY(!res2.isEmpty());
    QVERIFY(res2.first().title().contains("USD"));

    // 3. "299brl usd"
    auto res3 = manager.searchAll("299brl usd");
    QVERIFY(!res3.isEmpty());
    QVERIFY(res3.first().title().contains("USD"));

    // 4. "$100 to brl"
    auto res4 = manager.searchAll("$100 to brl");
    QVERIFY(!res4.isEmpty());
    QVERIFY(res4.first().title().contains("BRL"));

    // 5. "brl to usd"
    auto res5 = manager.searchAll("brl to usd");
    QVERIFY(!res5.isEmpty());
    QVERIFY(res5.first().title().contains("USD"));
}

QTEST_MAIN(TestMVSpotlight)
#include "test_spotlight.moc"
