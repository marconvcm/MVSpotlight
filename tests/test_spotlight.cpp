#include <QtTest/QtTest>
#include "../src/providers/CalculatorProvider.h"
#include "../src/providers/SettingsProvider.h"
#include "../src/providers/ActionProvider.h"
#include "../src/services/UsageHistory.h"
#include "../src/services/DesktopEntryService.h"
#include "../src/core/SearchController.h"
#include "../src/lua/LuaEngine.h"
#include "../src/lua/LuaPermissions.h"
#include "../src/lua/LuaPluginManager.h"
#include "../src/services/ConfigService.h"
#include "../src/providers/DeveloperCommandProvider.h"
#include "../src/providers/FileProvider.h"
#include "../src/providers/AiProvider.h"
#include "../src/services/AiService.h"
#include "../src/services/MarkdownRenderer.h"

class TestMVSpotlight : public QObject
{
    Q_OBJECT

private slots:
    void testCalculatorValid();
    void testCalculatorInvalid();
    void testUsageHistoryFrecency();
    void testSettingsSearch();
    void testActionSearch();
    void testDesktopEntrySearch();
    void testSearchControllerSettingsDispatch();
    void testSearchDebounce();
    void testLuaEngineExecution();
    void testLuaEngineErrorCatching();
    void testLuaPermissions();
    void testLuaPluginManagerLoad();
    void testCurrencyPlugin();
    void testWeatherPlugin();
    void testConfigService();
    void testDeveloperCommandProvider();
    void testFileProviderWorkspaceFolder();
    void testDeveloperToolsWorkspaceSettings();
    void testAiModeDetection();
    void testAiProviderSearch();
    void testAiConfigService();
    void testMarkdownRendering();
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
    QString testId = QString("test:app:frecency_%1").arg(QDateTime::currentMSecsSinceEpoch());

    double initialBoost = history.frecencyBoost(testId);
    QCOMPARE(initialBoost, 0.0);
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

    auto resGnomeSettings = settings.search("gnome settings");
    QVERIFY(!resGnomeSettings.isEmpty());
    QCOMPARE(resGnomeSettings.first().id(), QString("settings:main"));
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

void TestMVSpotlight::testDesktopEntrySearch()
{
    DesktopEntryService::instance().scanApplications();
    auto results = DesktopEntryService::instance().search("gnome settings");
    bool foundSettings = false;
    for (const auto &r : results) {
        if (r.id().contains("Settings", Qt::CaseInsensitive)) {
            foundSettings = true;
            break;
        }
    }
    QVERIFY(foundSettings);
}

void TestMVSpotlight::testSearchControllerSettingsDispatch()
{
    SearchController controller;
    QVERIFY(controller.init());

    controller.setQuery("gnome settings");
    controller.flushSearch();
    QVERIFY(controller.resultCount() > 0);

    bool foundSettings = false;
    for (int i = 0; i < controller.resultCount(); ++i) {
        auto map = controller.model()->get(i);
        QString id = map.value("id").toString();
        if (id == "settings:main" || id.contains("Settings", Qt::CaseInsensitive)) {
            foundSettings = true;
            break;
        }
    }
    QVERIFY(foundSettings);
}

void TestMVSpotlight::testSearchDebounce()
{
    SearchController controller;
    QVERIFY(controller.init());

    // 1. Test rapid keystroke coalescing with debounce = 80ms
    ConfigService::instance().setSearchDebounceMs(80);

    // Initial state with empty query
    controller.setQuery("");

    // Type rapidly - search is debounced
    controller.setQuery("xyznonexistent");
    QVERIFY(controller.isSearching());

    // Flush manually (e.g. user pressing Enter or arrow key)
    controller.flushSearch();
    QCOMPARE(controller.resultCount(), 0);

    // 2. Test natural timer expiration with QTRY_VERIFY_WITH_TIMEOUT
    controller.setQuery("wifi");
    QVERIFY(controller.isSearching());
    QTRY_VERIFY_WITH_TIMEOUT(controller.resultCount() > 0, 500);

    // 3. Test debounce = 0 (instant search)
    ConfigService::instance().setSearchDebounceMs(0);
    controller.setQuery("gnome settings");
    // With debounce = 0, sync search executes immediately without waiting
    QVERIFY(controller.resultCount() > 0);

    // 4. Test instant search when clearing query to empty
    controller.setQuery("");
    QCOMPARE(controller.query(), QString(""));

    // Restore standard default
    ConfigService::instance().setSearchDebounceMs(100);
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

void TestMVSpotlight::testWeatherPlugin()
{
    LuaPluginManager manager;
    QVERIFY(manager.init());

    // 1. "weather" keyword alone
    auto res1 = manager.searchAll("weather");
    QVERIFY(!res1.isEmpty());
    QVERIFY(res1.first().title().contains("Weather"));

    // 2. "weather tokyo"
    auto res2 = manager.searchAll("weather tokyo");
    QVERIFY(!res2.isEmpty());
    QVERIFY(res2.first().title().contains("Tokyo"));

    // 3. "clima sao paulo"
    auto res3 = manager.searchAll("clima sao paulo");
    QVERIFY(!res3.isEmpty());
    QVERIFY(res3.first().title().contains("Sao paulo") || res3.first().title().contains("Weather"));

    // 4. "london weather"
    auto res4 = manager.searchAll("london weather");
    QVERIFY(!res4.isEmpty());
    QVERIFY(res4.first().title().contains("London") || res4.first().title().contains("Weather"));
}

void TestMVSpotlight::testConfigService()
{
    ConfigService &cfg = ConfigService::instance();

    // Test appearance setters/getters
    cfg.setAccentColor("#10B981");
    QCOMPARE(cfg.accentColor(), QString("#10B981"));

    cfg.setCardWidth(750);
    QCOMPARE(cfg.cardWidth(), 750);

    cfg.setCornerRadius(20);
    QCOMPARE(cfg.cornerRadius(), 20);

    cfg.setSurfaceOpacity(0.92);
    QCOMPARE(cfg.surfaceOpacity(), 0.92);

    cfg.setThemeMode("dark");
    QCOMPARE(cfg.themeMode(), QString("dark"));

    cfg.setSearchDebounceMs(150);
    QCOMPARE(cfg.searchDebounceMs(), 150);
    cfg.setSearchDebounceMs(100); // restore default

    // Test plugin enable/disable
    cfg.setPluginEnabled("test.plugin", false);
    QCOMPARE(cfg.isPluginEnabled("test.plugin"), false);
    cfg.setPluginEnabled("test.plugin", true);
    QCOMPARE(cfg.isPluginEnabled("test.plugin"), true);

    // Test plugin custom settings
    cfg.setPluginSetting("test.plugin", "custom_key", "custom_val");
    QCOMPARE(cfg.getPluginSetting("test.plugin", "custom_key").toString(), QString("custom_val"));
    QCOMPARE(cfg.getPluginSetting("test.plugin", "missing_key", "default_val").toString(), QString("default_val"));
}

void TestMVSpotlight::testDeveloperCommandProvider()
{
    DeveloperCommandProvider dev;

    // Test preferences commands
    auto resPref = dev.search("preferences");
    QVERIFY(!resPref.isEmpty());
    QCOMPARE(resPref.first().action(), QString("open_preferences"));

    auto resSettings = dev.search("settings");
    QVERIFY(!resSettings.isEmpty());
    QCOMPARE(resSettings.first().action(), QString("open_preferences"));

    auto resPlugins = dev.search("plugins");
    QVERIFY(!resPlugins.isEmpty());
    QCOMPARE(resPlugins.first().action(), QString("open_preferences"));

    auto resReload = dev.search("reload");
    QVERIFY(!resReload.isEmpty());
    QCOMPARE(resReload.first().action(), QString("reload_plugins"));
}

void TestMVSpotlight::testFileProviderWorkspaceFolder()
{
    FileProvider provider;
    QSignalSpy spy(&provider, &FileProvider::resultsReady);

    provider.searchAsync(1, "workspace");
    QVERIFY(spy.wait(2000));

    QList<QVariant> arguments = spy.takeFirst();
    QList<SearchResult> results = arguments.at(1).value<QList<SearchResult>>();

    bool foundWorkspace = false;
    for (const auto &res : results) {
        if (res.title().compare("workspace", Qt::CaseInsensitive) == 0 && res.type() == "Folder") {
            foundWorkspace = true;
            QVERIFY(res.metadataValue("filePath").toString().endsWith("Workspace"));
            break;
        }
    }
    QVERIFY(foundWorkspace);
}

void TestMVSpotlight::testDeveloperToolsWorkspaceSettings()
{
    ConfigService &cfg = ConfigService::instance();
    cfg.setPluginSetting("org.mvspotlight.devtools", "workspace_path", "~/CustomWorkspace");
    QCOMPARE(cfg.getPluginSetting("org.mvspotlight.devtools", "workspace_path").toString(), QString("~/CustomWorkspace"));

    // Reset back
    cfg.setPluginSetting("org.mvspotlight.devtools", "workspace_path", "~/Workspace");
}

void TestMVSpotlight::testAiModeDetection()
{
    SearchController controller;
    QCOMPARE(controller.isAiMode(), false);

    controller.setQuery("firefox");
    QCOMPARE(controller.isAiMode(), false);

    controller.setQuery(">");
    QCOMPARE(controller.isAiMode(), true);

    controller.setQuery("> explain quantum computing");
    QCOMPARE(controller.isAiMode(), true);

    controller.setQuery("   > what is the weather");
    QCOMPARE(controller.isAiMode(), true);

    controller.setQuery("terminal > logs");
    QCOMPARE(controller.isAiMode(), false);
}

void TestMVSpotlight::testAiProviderSearch()
{
    AiProvider ai;
    auto resEmpty = ai.search("firefox");
    QVERIFY(resEmpty.isEmpty());

    // Without API key, should prompt user to configure in Preferences
    ConfigService::instance().setAiApiKey("");
    ConfigService::instance().setAiProvider("gemini");
    auto resPrompt = ai.search("> explain rust borrow checker");
    QVERIFY(!resPrompt.isEmpty());
    QCOMPARE(resPrompt.first().action(), QString("open_preferences"));

    // With API key configured, should yield ask_ai action for both > prefix and normal queries
    ConfigService::instance().setAiApiKey("test-key-12345");
    auto resQuery = ai.search("> explain rust borrow checker");
    QVERIFY(!resQuery.isEmpty());
    QCOMPARE(resQuery.first().type(), QString("AI"));
    QCOMPARE(resQuery.first().action(), QString("ask_ai"));
    QVERIFY(resQuery.first().title().contains("explain rust borrow checker"));

    auto resNormal = ai.search("what your meaning?");
    QVERIFY(!resNormal.isEmpty());
    QCOMPARE(resNormal.first().type(), QString("AI"));
    QCOMPARE(resNormal.first().action(), QString("ask_ai"));

    // Verify SearchController integration
    SearchController controller;
    controller.init();

    controller.setQuery("> what your meaning?");
    controller.flushSearch();
    QVERIFY(controller.resultCount() > 0);
    QCOMPARE(controller.currentResult()["type"].toString(), QString("AI"));

    controller.setQuery("what your meaning?");
    controller.flushSearch();
    QVERIFY(controller.resultCount() > 0);
    QCOMPARE(controller.currentResult()["type"].toString(), QString("AI"));

    // Clean up
    ConfigService::instance().setAiApiKey("");
}

void TestMVSpotlight::testAiConfigService()
{
    ConfigService &cfg = ConfigService::instance();
    QString origProvider = cfg.aiProvider();
    QString origModel = cfg.aiModel();
    QString origColor = cfg.aiAccentColor();

    cfg.setAiProvider("openai");
    QCOMPARE(cfg.aiProvider(), QString("openai"));
    QCOMPARE(cfg.aiModel(), QString("gpt-4o-mini"));

    cfg.setAiProvider("claude");
    QCOMPARE(cfg.aiProvider(), QString("claude"));
    QCOMPARE(cfg.aiModel(), QString("claude-3-5-sonnet-20241022"));

    cfg.setAiProvider("ollama");
    QCOMPARE(cfg.aiProvider(), QString("ollama"));
    QCOMPARE(cfg.aiModel(), QString("llama3.2"));

    cfg.setAiAccentColor("#FF007F");
    QCOMPARE(cfg.aiAccentColor(), QString("#FF007F"));

    cfg.setAiTemperature(1.2);
    QCOMPARE(cfg.aiTemperature(), 1.2);

    cfg.setAiMaxTokens(2048);
    QCOMPARE(cfg.aiMaxTokens(), 2048);

    // Restore original values
    cfg.setAiProvider(origProvider);
    cfg.setAiModel(origModel);
    cfg.setAiAccentColor(origColor);
}

void TestMVSpotlight::testMarkdownRendering()
{
    QString md = "# Title Heading\n\n"
                 "This is **bold** text and *italic* text.\n\n"
                 "```python\n"
                 "def add(a, b):\n"
                 "    return a + b\n"
                 "```\n\n"
                 "* Item Alpha\n"
                 "* Item Beta\n\n"
                 "[Google Search](https://google.com)";

    QString htmlDark = MarkdownRenderer::toHtml(md, true, "#8A2BE2");
    QVERIFY(!htmlDark.isEmpty());
    QVERIFY(htmlDark.contains("<h1"));
    QVERIFY(htmlDark.contains("Title Heading"));
    QVERIFY(htmlDark.contains("<pre"));
    QVERIFY(htmlDark.contains("def add(a, b):"));
    QVERIFY(htmlDark.contains("Item Alpha"));
    QVERIFY(htmlDark.contains("https://google.com"));
    QVERIFY(htmlDark.contains("color: #8A2BE2"));

    QString htmlLight = MarkdownRenderer::toHtml(md, false, "#3584E4");
    QVERIFY(!htmlLight.isEmpty());
    QVERIFY(htmlLight.contains("color: #3584E4"));

    // Verify AiService wrapper
    QString rendered = AiService::instance().renderMarkdown(md, true, "#8A2BE2");
    QCOMPARE(rendered, htmlDark);
}

QTEST_MAIN(TestMVSpotlight)
#include "test_spotlight.moc"
