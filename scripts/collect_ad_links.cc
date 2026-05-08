#define LUNA_TESTING
#include "../src/main.cpp"

#include <fstream>

struct NetworkLogger : QWebEngineUrlRequestInterceptor {
    std::ofstream log_file;

    NetworkLogger() {
        log_file.open("results.txt");
        log_file << "=== Network Request Log Started ===" << std::endl;
    }

    ~NetworkLogger() {
        if (log_file.is_open()) {
            log_file << "=== Network Request Log Ended ===" << std::endl;
        }
    }

    void interceptRequest(QWebEngineUrlRequestInfo &info) override {
        const auto url = info.requestUrl().toString().toStdString();

        if (log_file.is_open()) {
            log_file << url << std::endl;
        }
    }
};

int main(int argc, char *argv[]) {
    register_luna_scheme();
    QApplication app(argc, argv);

    LunaBrowserOptions opts;
    opts.disable_adblocker = true;
    opts.urls.push_back("https://adblocktester.pages.dev/");

    LunaAdBlocker adblocker(std::filesystem::path(get_app_data_base()) / "adblocker");

    const auto profile_base = std::filesystem::path(get_app_data_base()) / "default";
    if (!std::filesystem::exists(profile_base)) {
        std::filesystem::create_directories(profile_base);
    }

    LunaBrowserProfile profile(profile_base, const_cast<char*>("collect-ad-links"), adblocker, true);

    auto *logger = new NetworkLogger();
    profile.web_engine_profile->setUrlRequestInterceptor(logger);

    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [logger, &profile]() {
        profile.web_engine_profile->setUrlRequestInterceptor(nullptr);
        delete logger;
    });

    LunaBrowser browser(opts, profile, adblocker);
    app.installEventFilter(&browser);
    browser.prepare();
    browser.show();

    const int ret = app.exec();

    for (int i = browser.tabs->count() - 1; i >= 0; --i) {
        auto *w = browser.tabs->widget(i);
        browser.tabs->removeTab(i);
        delete w;
    }
    browser.tabs_count = 0;

    profile.destroy();
    adblocker.flush();

    return ret;
}
