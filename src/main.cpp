#include <QApplication>
#include <QMainWindow>
#include <QLineEdit>
#include <QToolBar>
#include <QAction>
#include <QWebEngineView>
#include <QWebEngineProfile>
#include <QWebEngineNewWindowRequest>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QUrl>
#include <QKeyEvent>
#include <QTabBar>
#include <QLabel>
#include <QSplitter>
#include <QHBoxLayout>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineUrlScheme>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineSettings>
#include <QBuffer>
#include <print>
#include <string>
#include <cstdlib>
#include <filesystem>

#define LUNA_FAIL(fmt, ...) \
    do { \
        std::print(fmt "\n", __VA_ARGS__); \
        std::exit(69); \
    } while (0)

#define LUNA_LOG(fmt, ...) \
    do { \
        std::print(fmt "\n", __VA_ARGS__); \
    } while (0)

const char* LUNA_VERSION = "v1.0";
const char* THIS_BROWSER_NAME = "luna";
// const char* DEFAULT_PAGE_URL = "https://start.duckduckgo.com";
// const char* DEFAULT_PAGE_URL = "https://thatsillyman.win";
constexpr const char* LUNA_PREFEX = "luna"; // the prefex used for spiceal domains e.g. luna:newtab.
const char* LUNA_NEW_TAB_URL = "luna:newtab";
const char* DEFAULT_PAGE_URL = LUNA_NEW_TAB_URL;
const char* LUNA_NEW_TAB_PAGE_PATH = "newtab.html";

// globals :3
char* new_tab_raw;

#if defined(_WIN32)
#define PATH_SEP '\\'
#define PATH_SEP_STR "\\"
#else
#define PATH_SEP '/'
#define PATH_SEP_STR "/"
#endif

const char* get_app_data_base() {
    static std::string path;
    if (!path.empty()) return path.c_str();

#if defined(_WIN32)
    const char* appdata = std::getenv("APPDATA");
    path = std::string(appdata ? appdata : ".") + PATH_SEP_STR + THIS_BROWSER_NAME;
#else //unix
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg) {
        path = std::string(xdg) + PATH_SEP_STR + THIS_BROWSER_NAME;
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            path = std::string(home) + "/.local/share/" + THIS_BROWSER_NAME;
        } else {
            path = std::string("./") + THIS_BROWSER_NAME;
        }
    }
#endif
    return path.c_str();
}

const char* get_app_config_base() {
    static std::string path;
    if (!path.empty()) return path.c_str();

#if defined(_WIN32)
    const char* config = std::getenv("APPDATA");
    path = std::string(config ? config : ".") + PATH_SEP_STR + THIS_BROWSER_NAME;
#else //unix
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg) {
        path = std::string(xdg) + PATH_SEP_STR + THIS_BROWSER_NAME;
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            path = std::string(home) + "/.config/" + THIS_BROWSER_NAME;
        } else {
            path = std::string("./") + THIS_BROWSER_NAME;
        }
    }
#endif
    return path.c_str();
}

const char* get_app_cache_base() {
    static std::string path;
    if (!path.empty()) return path.c_str();

#if defined(_WIN32)
    const char* cache = std::getenv("LOCALAPPDATA");
    path = std::string(cache ? cache : ".") + PATH_SEP_STR + THIS_BROWSER_NAME;
#else //unix
    const char* xdg = std::getenv("XDG_CACHE_HOME");
    if (xdg) {
        path = std::string(xdg) + PATH_SEP_STR + THIS_BROWSER_NAME;
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            path = std::string(home) + "/.cache/" + THIS_BROWSER_NAME;
        } else {
            path = std::string("./") + THIS_BROWSER_NAME;
        }
    }
#endif
    return path.c_str();
}

typedef enum {
    NewTabComamand,
    NewPrivateTabCommand,
    CloseTabCommand,
    CloseAllPrivateTabs,
    HistoryCommand, // show an interactive history view
    TabMuteToggleCommand,
    WebsiteJSEnableCommand, // Enable JS for X website
    WebsiteJSDisableCommand, // Disable JS for X website
    BookmarkCommand, // Add a url to the bookmarks list
    BookmarksCommand, // List and manage the bookmarks list
    BookmarkDeleteCommand,
    AdBlockEnableCommand,
    AdBlockEnableEverywhareCommand,
    AdBlockDisableCommand,
    AdBlockDisableEverywhareCommand,
    AdBlockStatsCommand,
    AdBolckWhiteListAddCommand,
    QuitCommand,
    SaveAndQuiteCommmand,
    SaveSessionCommand,
    SessionsCommand, // List and manage the sessions
    SessionDeleteCommand,
    OpenCommand, // open a url in a new tab
    ReloadCommand,
    ReloadHardCommand, // reload without using the cash
    StopCommand, // stop loading in current/[idx] tab
    PinTabCammand, // Pin the current/[idx] tab
    HintCommand, // Start hinting (a.k.a. the f mode)
    ScrollDownCommand,
    ScrollUpCommand,
    SplitHorizontally, // split the current tab horizontally
    SplitVertically, // split the current tab vertically
    ClearCashCommand, // Cleaar the current/[id] profile cash from disk and memory
    ClearCookiesCommand, // Clear the cookies of the current/[idx]/[url] website
} BrowserCommands;

typedef enum {
    NormalMode,
    InsertMode,
    CaretMode, // f
    CaretSelecionnMode, // shift + v
    CommandMode,
    NormieMode,
    SearchMode,
} BrowserMode;

struct BrowserUrl {
    std::string raw;

    QUrl as_qurl() {
        return QUrl(this->raw.c_str());
    }
};


struct StatusBar: QWidget {
    QLabel *cmd;
    QLabel *txt;
    QLabel *url;
    QLabel *position; // [top] -> [100%]
    QLabel *tab_index; // the selected tab index from the total [2/3]
    QLabel *keystr;

    StatusBar(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(4, 2, 4, 2);
        
        this->txt = new QLabel(" NORMAL ", this);
        this->txt->setTextFormat(Qt::PlainText);
        this->txt->setStyleSheet("background: #a89984; color: #282828; font-weight: bold; padding: 2px;");
        
        this->url = new QLabel("u:newtab", this);
        this->url->setTextFormat(Qt::PlainText);
        this->url->setStyleSheet("color: #ebdbb2; padding: 2px;");

        this->position = new QLabel("[top]", this);
        this->position->setTextFormat(Qt::PlainText);
        this->position->setStyleSheet("color: #ebdbb2; padding: 2px;");

        this->tab_index = new QLabel("[1/1]", this);
        this->tab_index->setTextFormat(Qt::PlainText);
        this->tab_index->setStyleSheet("color: #ebdbb2; padding: 2px;");
        
        this->keystr = new QLabel("", this);
        this->keystr->setTextFormat(Qt::PlainText);
        this->keystr->setStyleSheet("color: #ebdbb2; padding: 2px;");
        
        layout->addWidget(this->txt);
        layout->addWidget(this->url, 1);
        layout->addWidget(this->position);
        layout->addWidget(this->tab_index);
        layout->addWidget(this->keystr);
        
        this->setStyleSheet("background: #282828;");
    }

    void update_mode(const BrowserMode m) {
        switch(m) {
            case BrowserMode::NormalMode: this->txt->setText(" NORMAL "); break;
            case BrowserMode::InsertMode: this->txt->setText(" INSERT "); break;
            case BrowserMode::CaretMode: this->txt->setText(" CARET "); break;
            case BrowserMode::CaretSelecionnMode: this->txt->setText(" SEL "); break;
            case BrowserMode::NormieMode: this->txt->setText(" IGNORE "); break;
            case BrowserMode::CommandMode: this->txt->setText(nullptr); break;
        }
    }

    void set_url(const QString url) {
        this->url->setText(url);
    }
};

typedef enum {
    SplitHorizontallyDirection,
    SplitVerticallyDirection,
} TabBodySplitDirection;

typedef enum {
    SplitedTagBodyState,
    SingleViewTagBodyState,
} TabBodyStateTag;

struct TabBody: QWidget {
    TabBodyStateTag tag;
    union TabBodyStateValue {
        QSplitter *splitter;
        QWebEngineView *view;
    } val;

    TabBody(QWebEngineView *v) {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0,0,0,0);
        this->setLayout(layout);
        layout->addWidget(v);

        this->tag = TabBodyStateTag::SingleViewTagBodyState;
        this->val.view = v;
    }

    bool split(const TabBodySplitDirection direction, QWebEngineView *v) {
        auto *splitter = new QSplitter(
            direction == SplitHorizontallyDirection ? Qt::Horizontal : Qt::Vertical,
            this
        );

        if (this->tag == TabBodyStateTag::SingleViewTagBodyState) {
            assert(this->val.view);
            this->layout()->removeWidget(this->val.view);
            this->val.view->setParent(splitter);
            // this->val.view->installEventFilter(this); // to handle the FocusIn/Out events in the level of the view
            splitter->addWidget(val.view);
        } else if (this->tag == TabBodyStateTag::SplitedTagBodyState) {
            assert(this->val.splitter);
            // splitter->setParent(this->val.splitter);
            // const auto OLD_IDX = this->val.splitter->indexOf(this->active_veiw());
            // const auto old = this->val.splitter->replaceWidget(OLD_IDX, splitter);
            // splitter->addWidget(old);
            return false; // we only support two views per tab for now
        }

        v->setParent(splitter);
        // v->installEventFilter(this); // to handle the FocusIn/Out events in the level of the view
        splitter->addWidget(v);

        if (this->tag == TabBodyStateTag::SingleViewTagBodyState) {
            this->tag = TabBodyStateTag::SplitedTagBodyState;
            this->val.splitter = splitter;
            this->layout()->addWidget(splitter);
        }
        return true;
    }

    BrowserUrl url() {
       auto url = BrowserUrl {std::string()};
       auto *v = active_veiw();
       if (v) url.raw = v->url().toString().toStdString();
       return url;
    }

    QWebEngineView* active_veiw() {
        if (this->tag == TabBodyStateTag::SingleViewTagBodyState) {
            return this->val.view;
        } else if (this->tag == TabBodyStateTag::SplitedTagBodyState && this->val.splitter) {
            for (int i = 0; i < this->val.splitter->count(); ++i) {
                auto *widget = this->val.splitter->widget(i);
                if (widget->hasFocus()) return qobject_cast<QWebEngineView*>(widget);
            }
            return qobject_cast<QWebEngineView*>(this->val.splitter->widget(0));
        }
        return nullptr;
    }

    bool remove_active_veiw() {
        assert(this->tag == TabBodyStateTag::SplitedTagBodyState);
        auto w = this->active_veiw();
        w->setParent(nullptr);
        w->deleteLater();
        return this->val.splitter->count() == 0;
    }

};

static QWebEngineScript makeFModeScript() {
    QWebEngineScript s;
    s.setName("fmode");
    s.setInjectionPoint(QWebEngineScript::DocumentReady);
    s.setRunsOnSubFrames(true);
    s.setWorldId(QWebEngineScript::MainWorld);

    // this is the dumbest implementation possable for the fmode by the Clancker
    s.setSourceCode(R"JS(
        (function(){
            if (window.__fmode_installed) return;
            window.__fmode_installed = true;

            const chars = "asdfghjklqwertyuiopzxcvbnm";

            function gen2(){
                let s="";
                for(let i=0;i<2;i++) s+=chars[Math.floor(Math.random()*chars.length)];
                return s;
            }

            function visible(el){
                const r = el.getBoundingClientRect();
                return r.width>0 && r.height>0 &&
                       r.bottom>0 && r.right>0 &&
                       r.top < window.innerHeight &&
                       r.left < window.innerWidth;
            }

            function getTargets(){
                return Array.from(document.querySelectorAll(
                    "a,button,input,[onclick],[role=button]"
                )).filter(visible);
            }


            function cleanup(){
                window.__fmode_labels.forEach(l=>l.remove());
                window.__fmode_active=false;
                window.__fmode_buf="";
                document.removeEventListener("keydown", handler, true);
            }

            function start(){
                if (window.__fmode_active) return;
                window.__fmode_active = true;

                const els = getTargets();
                window.__fmode_map = Object.create(null);
                window.__fmode_labels = [];
                window.__fmode_buf = "";

                els.forEach(el=>{
                    let key;
                    do { key = gen2(); } while (window.__fmode_map[key]);

                    const r = el.getBoundingClientRect();
                    const d = document.createElement("div");
                    d.textContent = key;
                    d.style.position="fixed";
                    d.style.left = r.left + "px";
                    d.style.top  = r.top  + "px";
                    d.style.background="yellow";
                    d.style.color="black";
                    d.style.fontSize="12px";
                    d.style.padding="2px";
                    d.style.zIndex=2147483647;

                    document.body.appendChild(d);
                    window.__fmode_map[key] = el;
                    window.__fmode_labels.push(d);
                });


                function handler(e){
                    if(!window.__fmode_active) return;

                    if(e.key.length === 1){
                        window.__fmode_buf += e.key.toLowerCase();
                        if(window.__fmode_buf.length === 2){
                            const el = window.__fmode_map[window.__fmode_buf];
                            if(el) el.click();
                            cleanup();
                        }
                        e.preventDefault();
                    }
                }

                document.addEventListener("keydown", handler, true);
            }

            window.__fmode_start = start;
            window.__fmode_cleanup = cleanup;
        })();
    )JS");
    return s;
}

// struct FuckAllJavaScriptFilter: QWebEngineUrlRequestInterceptor { };

struct LunaBrowserSchemeHandler: QWebEngineUrlSchemeHandler {
    LunaBrowserSchemeHandler(QObject *p = nullptr) : QWebEngineUrlSchemeHandler(p) {}
    void requestStarted(QWebEngineUrlRequestJob *job) override {
        const QUrl url = job->requestUrl();
        QByteArray data;
        if (url.path() == "newtab") {
            // data = "<html><body><h1>New Tab</h1></body></html>";
            data = new_tab_raw;
        } else if (url.path() == "settings") {
            data = "<html><body><h1>Settings</h1></body></html>";
        } else {
            data = "<html><body><h1>404</h1></body></html>";
        }   

        auto *buffer = new QBuffer(job);
        buffer->setData(data);
        buffer->open(QIODevice::ReadOnly);
        job->reply("text/html", buffer);
    }
};

static void register_luna_scheme() {
    QWebEngineUrlScheme scheme(LUNA_PREFEX);
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
    scheme.setFlags(
        QWebEngineUrlScheme::SecureScheme |
        QWebEngineUrlScheme::LocalScheme |
        QWebEngineUrlScheme::LocalAccessAllowed
    );
    QWebEngineUrlScheme::registerScheme(scheme);
}

struct LunaBrowserHistory {
    std::filesystem::path file_path;

    LunaBrowserHistory() = default;
    LunaBrowserHistory(const std::filesystem::path history_file_path) {
        this->file_path = history_file_path;
    }
};

struct LunaBrowserBookmarks {
    std::filesystem::path file_path;

    LunaBrowserBookmarks() = default;
    LunaBrowserBookmarks(const std::filesystem::path bookmarks_file_path) {
        this->file_path = bookmarks_file_path;
    }
};

struct LunaBrowserProfile {
    char* name;
    LunaBrowserHistory history;
    LunaBrowserBookmarks bookmarks;
    QWebEngineProfile *web_engine_profile;

    LunaBrowserProfile(std::filesystem::path profile_base, char* name) {
        this->name = name;
        LunaBrowserHistory history(profile_base / "history");
        this->history = history;
        LunaBrowserBookmarks bookmarks(profile_base / "bookmarks");
        this->bookmarks = bookmarks;

        auto *web_engine_profile = new QWebEngineProfile(QString(name));
        web_engine_profile->scripts()->insert(makeFModeScript());
        web_engine_profile->setCachePath((profile_base / "cache").c_str());
        web_engine_profile->installUrlSchemeHandler(LUNA_PREFEX, new LunaBrowserSchemeHandler());
        this->web_engine_profile = web_engine_profile;
    }
};

struct LunaBrowserOptions {
    bool disable_adblocker = false;
    bool disable_cache = false;
    bool no_js = false;
    bool force_darkmode = false;
    bool frameless = false;
    bool private_window = false;
    std::vector<std::string> urls;
    std::vector<std::string> commands;
};

struct LunaBrowser: QMainWindow {
    BrowserMode mode = BrowserMode::NormalMode;
    QTabWidget *tabs;
    unsigned int tabs_count = 0;
    StatusBar* status_bar;
    LunaBrowserOptions opts;
    LunaBrowserProfile profile;

    void update_mode(const BrowserMode m) {
        this->mode = m;
        this->status_bar->update_mode(m);
        // QApplication::processEvents(); // in case we neede to change ui from a none ui thread
    }

    LunaBrowser(const LunaBrowserOptions opts, const LunaBrowserProfile profile): opts(opts), profile(profile) {
        this->setWindowTitle("Luna Browser");
        auto *central = new QWidget(this);
        auto *vbox = new QVBoxLayout(central);
        vbox->setContentsMargins(0,0,0,0);
        vbox->setSpacing(0);

        this->tabs = new QTabWidget();
        this->tabs->setTabsClosable(false); // to remove the [X] button
        this->tabs->setDocumentMode(true);
        this->tabs->tabBar()->setExpanding(true); // each tab want to take all the available space

        this->status_bar = new StatusBar();
        
        vbox->addWidget(this->tabs, 1);
        vbox->addWidget(this->status_bar, 0);
        this->setCentralWidget(central);


        this->tabs->setStyleSheet(R"(
            QTabWidget::pane { 
                border: none; 
            }
            QTabBar {
                background: #3c3836;
            }
            QTabBar::tab {
                background: #3c3836;
                color: #ebdbb2;
                padding: 2px 10px;
                border: none;
                min-width: 120px;
            }
            QTabBar::tab:selected {
                background: #3c3836;
                color: #ebdbb2;
                border-bottom: 2px solid #fe8019;
            }
        )");

        //
        QObject::connect(this->tabs, &QTabWidget::currentChanged, [&](int) {
            auto *v = dynamic_cast<TabBody*>(this->tabs->currentWidget())->active_veiw();
            if (v) {
                // this->urlbar->setText(v->url().toString());
                this->status_bar->set_url(v->url().toString());
                QObject::connect(v, &QWebEngineView::urlChanged, [&](const QUrl &u) {
                    // this->urlbar->setText(u.toString());
                    this->status_bar->set_url(u.toString());
                });
            }
        });

        QObject::connect(this->tabs, &QTabWidget::tabCloseRequested, [&](int i){
            this->close_tab(i);
        });
        // Keep smiling ^_^
    }

    void show() {
        LUNA_LOG("frameless: {}", this->opts.frameless);
        this->tabs->tabBar()->setVisible(!this->opts.frameless);
        this->status_bar->setVisible(!this->opts.frameless);
        if (this->opts.urls.empty()) {
            // Create the default tab
            const auto *web_view = this->new_tab(this->profile.web_engine_profile); 
        } else {
            for (std::string& url: this->opts.urls) {
                const auto *web_view = this->new_tab(this->profile.web_engine_profile, url.c_str()); 
            }
        }
        QMainWindow::show();
    }

    TabBody* new_tab(QWebEngineProfile* profile, const char* tab_url = DEFAULT_PAGE_URL, const bool instantly_switch = true) {
        LUNA_LOG("Creating a new tab for `{}`", tab_url);
        // auto *page = new QWebEnginePage()
        auto *web_engine_view = new QWebEngineView(profile);
        if (this->opts.no_js) {
            // TEST(anas): dose this blocks our scripts too?
            // if its maybe we want to go with the CSP + the `FuckAllJavaScriptFilter`
            web_engine_view->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
        }
        if (this->opts.force_darkmode) {
            web_engine_view->settings()->setAttribute(QWebEngineSettings::ForceDarkMode, true);
        }
        if (std::strcmp(tab_url, LUNA_NEW_TAB_URL) == 0) {
            // TODO(anas): provide the recents list
        }
        auto tab_body = new TabBody(web_engine_view);
        const auto url = QUrl(tab_url);
        const int idx = this->tabs->addTab(tab_body, "New Tab");
        if (instantly_switch) {
            this->tabs->setCurrentIndex(idx);
        }
        // when the page dose load update the tab text
        QObject::connect(web_engine_view, &QWebEngineView::titleChanged, [idx, this](const QString &t) {
            std::print("{}: {}\n", idx, t.toStdString());
            this->tabs->setTabText(idx, t);
        });
        QObject::connect(web_engine_view->page(), &QWebEnginePage::newWindowRequested, [profile, this](QWebEngineNewWindowRequest &request) {
            if (!request.isUserInitiated()) {
                // TODO(anas): cheack the allow list, or ask the user first
            }
            switch(request.destination()) {
                case QWebEngineNewWindowRequest::InNewWindow: {
                    // TODO(anas): open new window
                } break;
                case QWebEngineNewWindowRequest::InNewTab: {
                    auto *tb= this->new_tab(profile);
                    if (auto* v = tb->active_veiw()) request.openIn(v->page());
                } break;
                case QWebEngineNewWindowRequest::InNewDialog: {
                    //TODO(anas): In a window without a tab bar,toolbar, or URL bar.
                } break;
                case QWebEngineNewWindowRequest::InNewBackgroundTab: {
                    // TODO(anas): In a tab of the same window, without hiding the currently visible web engine view.
                } break;
            }
        });
        // update the status bar when the tab finshes loading
        QObject::connect(web_engine_view, &QWebEngineView::loadFinished, this, [&]() {
                std::print("Page load has finshed: TODO update the status_bar and loadtime\n");
            // TODO
        });
        QObject::connect(web_engine_view, &QWebEngineView::loadProgress, this, [idx, this](const int progress) {
            // this->update_load_progress(idx, progress);
        });
        this->tabs_count += 1;
        // load the url
        web_engine_view->load(url);
        return tab_body;
    }

    void close_tab(const unsigned int idx) {
        if (this->tabs_count == 1) this->new_tab(this->profile.web_engine_profile, DEFAULT_PAGE_URL, false);
        QWidget *w = this->tabs->widget(idx);
        this->tabs->removeTab(idx);
        this->tabs_count -= 1;
        delete w;
    }

    void close_selected_veiw(const unsigned int idx) {
        auto at = this->active_tab();
        if (at->tag == TabBodyStateTag::SplitedTagBodyState) {
            bool empty = at->remove_active_veiw();
            if (empty) this->close_tab(idx);
        } else {
            this->close_tab(idx);
        }
    }

    bool handle_key_press_event(const QKeyEvent *e) {
        const auto key = e->key();
        const auto mods = e->modifiers();
        std::print("keyPressEvent: {}\n",
               QKeySequence(mods | key).toString().toStdString());
        // if (cmd->isVisible()) return;
        if (this->mode == BrowserMode::NormieMode) {
            if (mods & Qt::ControlModifier & Qt::ShiftModifier) {
                if (key == Qt::Key_Escape) {
                    this->update_mode(BrowserMode::NormalMode);
                    return true; // ayoo
                }
            }
            std::print("NormieMode is enabled: forward all keys to the webpage\n");
            return false;
        }

        auto *v = this->active_tab()->active_veiw();

        if (mods & Qt::ControlModifier) {
            bool handled = false;
            switch (key) {
                case Qt::Key_T: { // ctrl+t
                    this->new_tab(this->profile.web_engine_profile);
                    handled = true;
                } break;
                case Qt::Key_W: { // ctrl+w
                    this->close_tab(this->tabs->currentIndex());
                    handled = true;
                } break;
                case Qt::Key_Escape: {
                    if (mods & Qt::ShiftModifier) {
                        this->update_mode(BrowserMode::NormieMode);
                        handled = true;
                    }
                } break;
            }
            return handled;
        }

        if (this->mode == BrowserMode::NormalMode) {
            switch (key) {
                case Qt::Key_J: v->page()->runJavaScript("window.scrollBy(0,100);"); break;
                case Qt::Key_K: v->page()->runJavaScript("window.scrollBy(0,-100);"); break;
                case Qt::Key_H: v->back(); break;
                case Qt::Key_L: v->forward(); break;
                case Qt::Key_R: v->reload(); break;
                case Qt::Key_D: this->close_selected_veiw(this->tabs->currentIndex()); break; 
                case Qt::Key_I: this->update_mode(BrowserMode::InsertMode); break;
                case Qt::Key_U: this->reopen_latest_tab(); break;
                case Qt::Key_F: {
                    this->update_mode(BrowserMode::CaretMode);
                    v->page()->runJavaScript("window.__fmode_start && window.__fmode_start();");
                } break;
                case Qt::Key_S: {
                    auto* tb = dynamic_cast<TabBody*>(this->tabs->currentWidget());
                    if(tb) {
                        auto *nv = new QWebEngineView(this->profile.web_engine_profile);
                        nv->load(QUrl(DEFAULT_PAGE_URL));
                        if (!tb->split(SplitHorizontallyDirection, nv)) this->show_error("Target tab is already in split mode!");
                    }
                } break;
                case Qt::Key_V: {
                    auto* tb = dynamic_cast<TabBody*>(this->tabs->currentWidget());
                    if(tb) {
                        auto *nv = new QWebEngineView(this->profile.web_engine_profile);
                        nv->load(QUrl(DEFAULT_PAGE_URL));
                        if(!tb->split(SplitVerticallyDirection, nv)) this->show_error("Target tab is already in split mode!");
                    }
                } break;
                case Qt::Key_Colon:
                case Qt::Key_Semicolon: {
                    this->update_mode(BrowserMode::CommandMode);
                    // this->show_command_prompt();
                } break;
            }
            return true;
        } else {
            if (key == Qt::Key_Escape) {
                if (this->mode == BrowserMode::CaretMode) {
                    v->page()->runJavaScript("window.__fmode_cleanup && window.__fmode_cleanup();");
                }
                this->update_mode(BrowserMode::NormalMode);
                return true;
            }
        }

        // if (key == Qt::Key_F11) {
            // this->showFullScreen();
            // return true;
        // }

        return false;
    }

    // needed for `installEventFilter`
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::KeyPress) {
            auto *e = static_cast<QKeyEvent*>(event);
            return this->handle_key_press_event(e);
        } else if (event->type() == QEvent::FocusIn) {
            std::print("focus has changed in\n");
            this->on_focus();
            return false; 
        } else if (event->type() == QEvent::FocusOut) {
            std::print("focus has changed out\n");
            // Handle focus lost
            return false;
        }

        return QObject::eventFilter(obj, event);
    }

    void on_focus() {
        // update the status bar
        {
            auto *v = this->active_tab()->active_veiw();
            this->status_bar->url->setText(v->url().toString());
        }
    }

    TabBody* active_tab() {
        auto *v = dynamic_cast<TabBody*>(this->tabs->currentWidget());
        return v;
    }

    void show_error(const char* msg) {
        std::print("{}\n", msg);
    }

    void reopen_latest_tab() {

    }

    bool run_cmd(const BrowserCommands command, const char* args) {
        return false; // TODO
    }

};


void print_help(const char *bin) {
    std::print(
        "Usage:\n"
        "  {} [<options>] [<urls>] [+<command>]\n\n"

        "Options:\n"
        "  -h, --help                 Prints this help\n"
        "  -v, --version              Prints version/build info\n"
        "  --disable-adblocker        Disable the adblocker globally\n"
        "  --disable-cache            Disable the cache globally\n"
        "  --no-js                    Disable JavaScript globally\n"
        "  --force-darkmode           Force the dark mode on all web pages\n"
        "  --private-window           Open a private window\n"
        "  -p, --profile <profile>    Load a specific profile\n"
        "  --freamless                Open window with no tab/status bar\n\n"

        "Arguments:\n"
        "  <urls>                     One or more URLs to open\n"
        "  +<command>                 Execute command on startup\n\n"

        "Examples:\n"
        "  {} https://thatsillyman.win\n"
        "  {} -p default https://example.com +bookmark\n"
        "  {} --private-window https://example.com\n",
        bin, bin, bin, bin
    );
}

int main(int argc, char *argv[]) {
    register_luna_scheme();   // MUST be first
    QApplication app(argc, argv);  // MUST be first Qt thing
    // LunaProfile profile;
    LunaBrowserOptions opts;
    char* profile_name = const_cast<char*>("default");
    // Frist of the firstest process the command line arguments.
    const auto bin = argv[0];
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];

        if (s == "-h" || s == "--help") {
            print_help(bin);
            return 0;
        } else if (s == "-v" || s == "--version") {
            std::print("Luna Browser {}\n", LUNA_VERSION);
            return 0;
        }
        else if (s == "--disable-adblocker") opts.disable_adblocker = true;
        else if (s == "--disable-cache") opts.disable_cache = true;
        else if (s == "--no-js") opts.no_js = true;
        else if (s == "--force-darkmode") opts.force_darkmode = true;
        else if (s == "--freamless") opts.frameless = true;

        else if (s == "-p" || s == "--profile") {
            if (i + 1 >= argc) {
                LUNA_FAIL("{} is used but no name are provided", s);
            }
            const auto name = argv[++i];
            profile_name = name;
        } else if (s == "--private-window") {
            opts.private_window = true;
        } else if (!s.empty() && s[0] == '+') {
            opts.commands.push_back(s.substr(1));
        } else {
            opts.urls.push_back(s);
        }
    }

    const auto new_tab_path = std::filesystem::path(get_app_config_base()) / LUNA_NEW_TAB_PAGE_PATH;
    if (std::filesystem::exists(new_tab_path)) {
        const auto fsize = std::filesystem::file_size(new_tab_path);

        FILE *new_tab_f = std::fopen(new_tab_path.string().c_str(), "rb");
        if (new_tab_f) {
            std::fread(new_tab_raw, 1, fsize, new_tab_f);
            std::fclose(new_tab_f);
        }
    } else {
        new_tab_raw = const_cast<char*>(R"HTML(<!DOCTYPE html><html lang="en">
        <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>New Tab</title>
        <style>
        :root {
            --bg:#0a0b10;
            --bg-soft:#11131a;
            --fg:#e0e6ed;
            --muted:#9aa3ad;
            --accent:rgba(224,230,237,.15);
        }
        * { box-sizing:border-box; }
        body {
            margin:0;
            font-family:system-ui,-apple-system,sans-serif;
            background:radial-gradient(circle at top,#1a1c29,var(--bg)60%);
            color:var(--fg);
            display:grid;
            place-items:center;
            min-height:100vh;
        }
        .container {
            width:100%;
            max-width:520px;
            padding:2rem;
            text-align:center;
        }
        .moon {
            width:64px;
            height:64px;
            margin:0 auto 2rem;
            border-radius:50%;
            background:var(--fg);
            box-shadow:
                0 0 30px var(--accent),
                inset -10px -10px 18px rgba(0,0,0,.5),
                inset 4px 4px 8px rgba(255,255,255,.7);
        }
        .search {
            width:100%;
            padding:1rem 1.4rem;
            border-radius:999px;
            border:1px solid rgba(255,255,255,.08);
            background:var(--bg-soft);
            color:var(--fg);
            outline:0;
            font-size:1rem;
            transition:.2s;
        }
        .search::placeholder { color:var(--muted); }
        .search:focus {
            border-color:rgba(255,255,255,.25);
            box-shadow:0 0 15px var(--accent);
        }
        .grid {
            margin-top:2.5rem;
            display:grid;
            grid-template-columns:repeat(auto-fill,minmax(90px,1fr));
            gap:1rem;
        }
        .item {
            text-decoration:none;
            color:var(--fg);
            padding:.8rem;
            border-radius:12px;
            background:rgba(255,255,255,.02);
            transition:.15s;
        }
        .item:hover {
            background:rgba(255,255,255,.06);
            transform:translateY(-2px);
        }
        .icon {
            width:36px;
            height:36px;
            margin:0 auto .5rem;
            border-radius:50%;
            display:grid;
            place-items:center;
            background:rgba(255,255,255,.08);
            font-size:.9rem;
        }
        .title {
            font-size:.75rem;
            color:var(--muted);
            white-space:nowrap;
            overflow:hidden;
            text-overflow:ellipsis;
        }
        </style>
        </head>
        <body>
        <div class="container">
        <div class="moon"></div>
        <form action="https://duckduckgo.com/">
        <input class="search" name="q" placeholder="Search..." autofocus>
        </form>
        <div class="grid" id="grid"></div>
        </div>
        <script>
        const s=[
        ["Anas's Homepage","https://thatsillyman.win"],
        ["GitHub","https://github.com"],
        ["Kernel","https://kernel.org"],
        ["Arch","https://wiki.archlinux.org"],
        ["PostgreSQL","https://postgresql.org"],
        ["HN","https://news.ycombinator.com"],
        ["Reddit","https://reddit.com"]
        ],g=document.getElementById("grid");
        s.forEach(([t,u])=>{
        const a=document.createElement("a");
        a.href=u;
        a.className="item";
        a.innerHTML=`<div class="icon">${t[0]}</div><div class="title">${t}</div>`;
        g.appendChild(a);
        });
        </script>
        </body>
        </html>
        )HTML");
    }

    if (opts.private_window) {
        // TODO(anas): dummy profile?
    }
    const auto path = std::filesystem::path(get_app_data_base()) / profile_name;
    if (!std::filesystem::exists(path)) {
        LUNA_LOG("{}", "The provided profile dosen't exists, we will create it");
        if(!std::filesystem::create_directories(path)) LUNA_FAIL("{} can't be created, please cheack your permissions", path.c_str());
    }
    LunaBrowserProfile profile(path, profile_name);
    LunaBrowser browser(opts, profile);
    app.installEventFilter(&browser);
    browser.show();

    return app.exec();
}
