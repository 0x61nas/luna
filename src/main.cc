// things...
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
#include <QUrlQuery>
#include <QKeyEvent>
#include <QTabBar>
#include <QLabel>
#include <QProgressBar>
#include <QSplitter>
#include <QHBoxLayout>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineUrlScheme>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineSettings>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineFindTextResult>
#include <QBuffer>
#include <QCompleter>
#include <QStringListModel>
#include <QListView>
#include <QTimer>
#include <print>
#include <string>
#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <curl/curl.h>
#include <fstream>
#include <unordered_map>
#include <regex>
#include <memory>
#include <array>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>

#define LUNA_FAIL(fmt, ...) \
    do { \
        std::print(fmt "\n", __VA_ARGS__); \
        std::exit(69); \
    } while (0)

#define LUNA_LOG(fmt, ...) \
    do { \
        std::print(fmt "\n", __VA_ARGS__); \
    } while (0)
#ifdef LUNA_DEBUG_BUILD
#define LUNA_DEBUG(fmt, ...) LUNA_LOG(fmt, __VA_ARGS__)
#else
#define LUNA_DEBUG(fmt, ...) do { (void)fmt; } while (0)
#endif // LUNA_DEBUG_BUILD

#ifdef LUNA_DEBUG_BUILD
struct AllocationMetrics {
    uint32_t total_allocated = 0;
    uint32_t total_freed = 0;

    uint32_t current_usage() { return total_allocated - total_freed; }
};
static AllocationMetrics allocation_metrics;
void* operator new(size_t size) {
    allocation_metrics.total_allocated += size;
    return malloc(size);
}
void operator delete(void* mem) noexcept {
    free(mem);
}
void operator delete(void* mem, size_t size) noexcept {
    allocation_metrics.total_freed += size;
    free(mem);
}
#endif // LUNA_DEBUG_BUILD

const char* LUNA_VERSION = "v1.0";
const char* THIS_BROWSER_NAME = "luna-browser";
// const char* DEFAULT_PAGE_URL = "https://start.duckduckgo.com";
// const char* DEFAULT_PAGE_URL = "https://thatsillyman.win";
constexpr const char* LUNA_PREFEX = "luna"; // the prefex used for spiceal domains e.g. luna:newtab.
const char* LUNA_NEW_TAB_URL = "luna:newtab";
const char* DEFAULT_PAGE_URL = LUNA_NEW_TAB_URL;
const char* THE_DEFAULT_SEARCH_ENGINE = "https://duckduckgo.com/?q=";
const char* LUNA_NEW_TAB_PAGE_PATH = "newtab.html";
constexpr const size_t TAB_RESTORE_MAX_COUNT = 10;
constexpr const size_t THE_ZERO = 69^69; // just as it should be.
constexpr const char* SESSION_FILE_HEADER = "Luna Browser Session";
constexpr const char* SESSION_FILE_VERSION_STR = "VERSION: v1";
constexpr const int SESSION_FILE_VERSION = 1;

// globals :3
static const char NEW_TAB_EMBEDDED[] = {
    #embed "../assets/newtab.html"
    , 0
};
char* new_tab_raw = const_cast<char*>(NEW_TAB_EMBEDDED);
static const char THE_F_MODE_JS_SCRIPT_EMBEDDED[] = {
    #embed "../assets/fmode.js"
    , 0
};
uint64_t total_blocked_ads = 0;

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

static uint64_t str2u64(std::string_view s) {
    uint64_t result = 0;
    for (char c : s) {
        if (c >= '0' && c <= '9') {
            result = result * 10 + (c - '0');
        } else if (result > 0) {
            break;
        }
    }
    return result;
}

inline bool is_separator(char c) {
    return !std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_' && c != '.' && c != '%';
}

bool glob_match(std::string_view text, std::string_view pattern) {
    size_t t = 0, p = 0;
    size_t star_t = text.length(), star_p = pattern.length();
    
    while (t < text.length()) {
        if (p < pattern.length() && pattern[p] == '*') {
            star_p = p++;
            star_t = t;
        } else if (p < pattern.length() && pattern[p] == '^') {
            if (is_separator(text[t])) {
                t++; p++;
                continue;
            } else if (star_p != pattern.length()) {
                p = star_p + 1;
                t = ++star_t;
                continue;
            } else {
                return false;
            }
        } else if (p < pattern.length() && text[t] == pattern[p]) {
            t++; p++;
        } else if (star_p != pattern.length()) {
            p = star_p + 1;
            t = ++star_t;
        } else {
            return false;
        }
    }
    
    while (p < pattern.length()) {
        if (pattern[p] == '*') p++;
        else if (pattern[p] == '^' && t == text.length()) p++;
        else break;
    }
    return p == pattern.length();
}

static size_t curl_write_cb(void* ptr, size_t size, size_t nmemb, void* userdata) {
    std::string* out = static_cast<std::string*>(userdata);
    out->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

static bool check_valid_url(const QString &str) {
    // Simple check: has dot and no spaces, or has protocol
    const QChar *data = str.data();
    size_t len = str.length();
    bool has_dot = false;
    for (size_t i = 0; i < len; i++) {
        if (data[i] == '.') has_dot = true;
        if (data[i] == ' ') return false;
    }
    if (has_dot) return true;
    return (str.startsWith("http://") || str.startsWith("https://") || str.startsWith("luna:"));
}

typedef enum {
    UnknownCommand,
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
    QLabel *load_time;
    QLabel *keystr;
    QProgressBar *progress;

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

        this->load_time = new QLabel("", this);
        this->load_time->setTextFormat(Qt::PlainText);
        this->load_time->setStyleSheet("color: #ebdbb2; padding: 2px;");
        
        this->keystr = new QLabel("", this);
        this->keystr->setTextFormat(Qt::PlainText);
        this->keystr->setStyleSheet("color: #ebdbb2; padding: 2px;");

        this->progress = new QProgressBar(this);
        this->progress->setRange(0, 100);
        this->progress->setValue(0);
        this->progress->setTextVisible(false);
        this->progress->setMaximumHeight(10);
        this->progress->setStyleSheet("QProgressBar::chunk { background: #98971a; }");
        
        layout->addWidget(this->txt);
        layout->addWidget(this->url, 1);
        layout->addWidget(this->progress);
        layout->addWidget(this->position);
        layout->addWidget(this->tab_index);
        layout->addWidget(this->load_time);
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
            case BrowserMode::SearchMode: this->txt->setText(" SEARCH "); break;
        }
    }

    void set_url(const QString url) {
        this->url->setText(url);
    }

    void set_progress(const size_t p) {
        this->progress->setValue(p);
        this->progress->setVisible(p > 0 && p < 100);
    }

    void set_tab_index(const size_t current, const size_t total) {
        this->tab_index->setText(QString("[%1/%2]").arg(current + 1).arg(total));
    }

    void set_position(const QString &text) {
        this->position->setText(text);
    }

    void set_load_time(const uint64_t elapsed) {
        QString text;
        if (elapsed < 1000) {
            text = QString("%1ms").arg(elapsed);
        } else if (elapsed < 60000) {
            text = QString("%1sec").arg(elapsed / 1000.0, 0, 'f', 2);
        } else {
            text = QString("%1min").arg(elapsed / 60000.0, 0, 'f', 2);
        }
        this->load_time->setText(text);
    }
};

struct LunaBrowserElapsedTimer {
    uint64_t start_timestamp = THE_ZERO;
    uint64_t end_timestamp = THE_ZERO;

    void start() {
        this->end_timestamp = THE_ZERO;
        this->start_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
                ).count();
    }

    void stop() {
        this->end_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
                ).count();
    }

    uint64_t elapsed() {
        if (this->end_timestamp == THE_ZERO) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
                ).count() - this->start_timestamp;
        }
        return this->end_timestamp - this->start_timestamp;
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
    struct PendingViewState {
        QString url;
        QPointF scroll_position;
        QString search_term;
        QString title;
    };
    TabBodyStateTag tag;
    union TabBodyStateValue {
        QSplitter *splitter;
        QWebEngineView *view;
    } val;
    LunaBrowserElapsedTimer load_timer;
    QString search_term;
    std::vector<PendingViewState> pending_views;
    bool has_pending_load() const { return !pending_views.empty(); }
    // QWebEngineFindTextResult *find_result;

    TabBody(QWebEngineView *v) {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0,0,0,0);
        this->setLayout(layout);
        layout->addWidget(v);

        this->tag = TabBodyStateTag::SingleViewTagBodyState;
        this->val.view = v;
    }

    bool split(const TabBodySplitDirection direction, QWebEngineView *v) {
        const auto orientation = direction == SplitHorizontallyDirection ? Qt::Horizontal : Qt::Vertical;
        auto *splitter = new QSplitter(orientation, this);

        if (this->tag == TabBodyStateTag::SingleViewTagBodyState) {
            assert(this->val.view);
            this->layout()->removeWidget(this->val.view);
            this->val.view->setParent(splitter);
            splitter->addWidget(val.view);
        } else if (this->tag == TabBodyStateTag::SplitedTagBodyState) {
            assert(this->val.splitter);
            if (this->val.splitter->orientation() != orientation) {
                this->val.splitter->setOrientation(orientation); // if the orientation is different maybe all the user needs is to change the direction
                return true;
            }
            return false; // we only support two views per tab for now
        }

        v->setParent(splitter);
        splitter->addWidget(v);

        if (this->tag == TabBodyStateTag::SingleViewTagBodyState) {
            this->tag = TabBodyStateTag::SplitedTagBodyState;
            this->val.splitter = splitter;
            this->layout()->addWidget(splitter);
            int total = direction == SplitHorizontallyDirection ? this->width() : this->height();
            splitter->setSizes({total / 2, total / 2});
        }
        v->setFocus();
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
        auto *splitter = this->val.splitter;
        auto w = this->active_veiw();

        // Find the survivor (the view NOT being removed).
        // Must be done before removing anything, since QSplitter indices shift
        // when a widget is reparented away.
        QWebEngineView *view = nullptr;
        for (int i = 0; i < splitter->count(); ++i) {
            auto *wi = qobject_cast<QWebEngineView*>(splitter->widget(i));
            if (wi && wi != w) {
                view = wi;
                break;
            }
        }
        assert(view != nullptr);

        w->setParent(nullptr);
        w->deleteLater();
        view->setParent(this);
        this->layout()->removeWidget(splitter);
        this->layout()->addWidget(view);
        delete splitter;
        this->val.view = view;
        this->tag = TabBodyStateTag::SingleViewTagBodyState;
        return false; // NOTE(anas): for now we only support one level of splitting
    }

};

struct TabRestoreState {
    QString url;
    QPointF scroll_position;
    QString search_term;
    size_t tab_index;

    bool is_valid() const {
        return !url.isEmpty();
    }
};

static QWebEngineScript makeFModeScript() {
    QWebEngineScript s;
    s.setName("fmode");
    s.setInjectionPoint(QWebEngineScript::DocumentReady);
    s.setRunsOnSubFrames(true);
    s.setWorldId(QWebEngineScript::MainWorld);
    s.setSourceCode(THE_F_MODE_JS_SCRIPT_EMBEDDED);
    return s;
}

typedef enum {
    NetworkBlockRule,
    NetworkExceptionRule,
    ContentHideRule,
} LunaAdBlockerRuleType;

struct LunaAdBlockerRuleOptions {
    uint32_t resource_mask = 0;
    std::vector<std::string> domains;
    std::vector<std::string> exclude_domains;
    std::vector<std::string> from_domains;
    std::vector<std::string> to_domains;
    bool third_party = false;
    bool not_third_party = false;
    bool match_case = false;
    bool domain_anchor = false;
    bool cname_only = false;
    bool generichide = false;
    bool specifichide = false;
    bool has_unrecognized = false;
};

struct LunaAdBlockerRule {
    LunaAdBlockerRuleType type;
    std::string pattern;
    std::string selector;
    bool exact_start = false;
    bool exact_end = false;
    bool domain_anchor = false;
    bool is_regex = false;
    std::unique_ptr<std::regex> regex_pattern;
    LunaAdBlockerRuleOptions options;
#ifdef LUNA_TESTING
    std::string original_rule;
#endif // LUNA_TESTING

    // Based on: https://adblockplus.org/filter-cheatsheet#exceptions
    static LunaAdBlockerRule parse(std::string_view line) {
        LunaAdBlockerRule rule{};
#ifdef LUNA_TESTING
        rule.original_rule = line;
#endif // LUNA_TESTING
        rule.type = LunaAdBlockerRuleType::NetworkBlockRule;

        size_t i = 0;
        const size_t n = line.size();

        // @@ exception
        if (n >= 2 && line[0] == '@' && line[1] == '@') {
            rule.type = LunaAdBlockerRuleType::NetworkExceptionRule;
            i += 2;
        }

        // scan once
        size_t content_pos = std::string_view::npos;
        size_t options_pos = std::string_view::npos;

        for (size_t j = i; j < n; ++j) {
            if (j + 1 < n && line[j] == '#' && line[j + 1] == '#') {
                content_pos = j;
                break;
            }
            if (line[j] == '$') {
                options_pos = j;
                break;
            }
        }

        // content rule
        if (content_pos != std::string_view::npos) {
            rule.type = LunaAdBlockerRuleType::ContentHideRule;
            auto domain_part = line.substr(i, content_pos - i);
            rule.pattern = domain_part;
            rule.selector = line.substr(content_pos + 2);

            // Parse domain restrictions from content rule prefix (comma-separated, supports ~exclude)
            if (!domain_part.empty()) {
                size_t pos = 0;
                while (pos < domain_part.size()) {
                    size_t comma = domain_part.find(',', pos);
                    size_t end = (comma == std::string_view::npos) ? domain_part.size() : comma;
                    auto tok = domain_part.substr(pos, end - pos);
                    if (!tok.empty()) {
                        if (tok[0] == '~') {
                            rule.options.exclude_domains.push_back(std::string(tok.substr(1)));
                        } else {
                            rule.options.domains.push_back(std::string(tok));
                        }
                    }
                    pos = (comma == std::string_view::npos) ? domain_part.size() : comma + 1;
                }
            }

            return rule;
        }

        size_t end = (options_pos != std::string_view::npos) ? options_pos : n;

        if (options_pos != std::string_view::npos) {
            auto opts = line.substr(options_pos + 1);
            parse_options(opts, rule.options);
        }

        // Check if it's a regex rule (pattern part starts and ends with /)
        if (i < end && line[i] == '/' && end > i + 1 && line[end - 1] == '/') {
            rule.is_regex = true;
            std::string regex_str(line.substr(i + 1, end - i - 2));
            std::regex::flag_type flags = std::regex::ECMAScript;
            if (!rule.options.match_case) {
                flags |= std::regex::icase;
            }
            try {
                rule.regex_pattern = std::make_unique<std::regex>(regex_str, flags);
            } catch (const std::regex_error& e) {
                rule.is_regex = false; // if the regex is bad just IGNORE it
            }
            return rule;
        }

        // anchors
        if (end - i >= 2 && line[i] == '|' && line[i + 1] == '|') {
            rule.domain_anchor = true;
            i += 2;
        } else if (i < end && line[i] == '|') {
            rule.exact_start = true;
            ++i;
        }

        if (end > i && line[end - 1] == '|') {
            rule.exact_end = true;
            --end;
        }

        // pattern
        std::string normalized(line.substr(i, end - i));

        if (!rule.exact_end)
            normalized += '*';

        if (!rule.exact_start && !rule.domain_anchor)
            normalized = "*" + normalized;

        rule.pattern = std::move(normalized);
        return rule;
    }

    static void parse_options(std::string_view opts, LunaAdBlockerRuleOptions& options) {
        size_t i = 0;
        while (i < opts.size()) {
            size_t j = i;
            while (j < opts.size() && opts[j] != ',') ++j;

            std::string_view tok = opts.substr(i, j - i);
            
            // Check for key=value patterns
            size_t eq_pos = tok.find('=');
            if (eq_pos != std::string_view::npos) {
                std::string_view key = tok.substr(0, eq_pos);
                std::string_view val = tok.substr(eq_pos + 1);
                
                if (key == "domain") {
                    parse_domain_list(val, options.domains, options.exclude_domains);
                } else if (key == "from") {
                    parse_domain_list(val, options.from_domains, options.exclude_domains);
                } else if (key == "to") {
                    parse_domain_list(val, options.to_domains, options.exclude_domains);
                }
                i = j + 1;
                continue;
            }

            bool neg = false;
            if (!tok.empty() && tok[0] == '~') {
                neg = true;
                tok.remove_prefix(1);
            }

            uint32_t bit = 0;

            if (tok == "script") bit = 1 << 0;
            else if (tok == "image") bit = 1 << 1;
            else if (tok == "stylesheet") bit = 1 << 2;
            else if (tok == "object") bit = 1 << 3;
            else if (tok == "subdocument") bit = 1 << 4;
            else if (tok == "xmlhttprequest" || tok == "xhr") bit = 1 << 5;
            else if (tok == "websocket") bit = 1 << 6;
            else if (tok == "webrtc") bit = 1 << 7;
            else if (tok == "popup") bit = 1 << 8;
            else if (tok == "ping") bit = 1 << 9;
            else if (tok == "font") bit = 1 << 10;
            else if (tok == "media") bit = 1 << 11;
            else if (tok == "document") bit = 1 << 12;
            else if (tok == "third-party" || tok == "3p") {
                if (neg) options.not_third_party = true;
                else options.third_party = true;
            } else if (tok == "match-case") {
                options.match_case = true;
            } else if (tok == "domain") {
                // handled above with key=value
            } else if (tok == "cname") {
                options.cname_only = true;
            } else if (tok == "generichide") {
                options.generichide = true;
            } else if (tok == "specifichide") {
                options.specifichide = true;
            } else if (tok == "redirect" || tok == "redirect-rule" ||
                       tok == "important" || tok == "badfilter" ||
                       tok == "denyallow" || tok == "header" ||
                       tok == "method" || tok == "all" ||
                       tok == "popunder" || tok == "empty" ||
                       tok == "mp4" || tok == "doc" ||
                       tok == "other" || tok == "frame" ||
                       tok == "inline-script" || tok == "csp" ||
                       tok == "prefetch" || tok == "jsonprune" ||
                       tok == "minify" || tok == "replace" ||
                       tok == "noop") {
                // Known but non-functional options - skip without marking unrecognized
            } else {
                options.has_unrecognized = true;
            }

            if (bit) {
                if (neg) options.resource_mask &= ~bit;
                else options.resource_mask |= bit;
            }

            i = j + 1;
        }
    }

    static void parse_domain_list(std::string_view val, std::vector<std::string>& include, std::vector<std::string>& exclude) {
        size_t i = 0;
        while (i < val.size()) {
            size_t j = i;
            while (j < val.size() && val[j] != '|') ++j;
            
            std::string_view domain = val.substr(i, j - i);
            if (!domain.empty()) {
                if (domain[0] == '~') {
                    exclude.push_back(std::string(domain.substr(1)));
                } else {
                    include.push_back(std::string(domain));
                }
            }
            i = j + 1;
        }
    }

    bool match(std::string_view url, uint32_t resource_type = 0, std::string_view document_domain = "") {
        // Skip rules with options we don't support (they wouldn't match correctly)
        if (this->options.has_unrecognized) return false;
        if (this->options.cname_only) return false;
        if (this->options.generichide || this->options.specifichide) return false;

        // Check resource type
        if (this->options.resource_mask != 0) {
            if (resource_type == 0) return false; // for tests
            if (!(this->options.resource_mask & resource_type)) {
                return false;
            }
        }

        // Check third-party
        if (this->options.third_party || this->options.not_third_party) {
            if (document_domain.empty()) return false;
            bool is_third_party = !is_same_domain(url, document_domain);
            if (this->options.third_party && !is_third_party) return false;
            if (this->options.not_third_party && is_third_party) return false;
        }

        // Check domain restrictions
        if (!check_domain_restrictions(document_domain, url)) {
            return false;
        }

        // Match URL based on pattern type
        if (this->is_regex && this->regex_pattern) {
            std::string url_str(url);
            try {
                return std::regex_search(url_str, *this->regex_pattern);
            } catch (...) {
                return false;
            }
        }

        if (this->domain_anchor) {
            std::string_view target = url;
            size_t scheme_end = url.find("://");
            if (scheme_end != std::string_view::npos) {
                target.remove_prefix(scheme_end + 3);
            }
            size_t path_start = target.find_first_of("/:#?");
            if (path_start != std::string_view::npos) {
                target = target.substr(0, path_start);
            }
            return glob_match(target, this->pattern) || glob_match(target, "*." + this->pattern);
        }
        return glob_match(url, this->pattern);
    }

    static bool is_same_domain(std::string_view url, std::string_view domain) {
        size_t scheme_end = url.find("://");
        std::string_view host = url;
        if (scheme_end != std::string_view::npos) {
            host.remove_prefix(scheme_end + 3);
        }
        size_t path_start = host.find_first_of("/:#?");
        if (path_start != std::string_view::npos) {
            host = host.substr(0, path_start);
        }
        
        // Check if host ends with domain
        if (host.size() < domain.size()) return false;
        auto pos = host.rfind(domain);
        if (pos == std::string_view::npos) return false;
        if (pos + domain.size() != host.size()) return false;
        if (pos > 0 && host[pos - 1] != '.') return false;
        return true;
    }

    bool check_domain_restrictions(std::string_view document_domain, std::string_view request_url = "") {
        // Check from domains (document domain)
        if (!this->options.from_domains.empty()) {
            bool matched = false;
            for (const auto& d : this->options.from_domains) {
                if (is_same_domain(document_domain, d)) {
                    matched = true;
                    break;
                }
            }
            if (!matched) return false;
        }

        // Check exclude domains (applies to document domain)
        for (const auto& d : this->options.exclude_domains) {
            if (is_same_domain(document_domain, d)) {
                return false;
            }
        }

        // Check include domains (generic domain option, applies to document domain)
        if (!this->options.domains.empty()) {
            bool matched = false;
            for (const auto& d : this->options.domains) {
                if (is_same_domain(document_domain, d)) {
                    matched = true;
                    break;
                }
            }
            if (!matched) return false;
        }

        // Check to domains (request URL domain)
        if (!this->options.to_domains.empty() && !request_url.empty()) {
            bool matched = false;
            for (const auto& d : this->options.to_domains) {
                if (is_same_domain(request_url, d)) {
                    matched = true;
                    break;
                }
            }
            if (!matched) return false;
        }

        return true;
    }

};

struct LunaAdBlockerList {
    std::string name;
    std::string url;
};

struct LunaAdBlocker {
    std::vector<LunaAdBlockerRule> network_rules;
    std::vector<LunaAdBlockerRule> network_exception_rules;
    std::vector<LunaAdBlockerRule> content_rules;

    std::vector<LunaAdBlockerList> lists;
    std::filesystem::path lists_dir;
    std::filesystem::path timestamps_db;
    std::unordered_map<std::string, uint64_t> timestamps;

    // Cache for block_request results with TTL
    struct CacheEntry {
        bool result;
        std::chrono::steady_clock::time_point expires_at;
    };
    std::unordered_map<std::string, CacheEntry> cache;
    std::mutex cache_mutex;

    // Simple cache for get_hiding_rules_for_domain
    std::unordered_map<std::string, std::string> hide_cache_data;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> hide_cache_expiry;
    std::thread cleanup_thread;
    std::atomic<bool> cleanup_running{false};
    std::condition_variable cleanup_cv;
    std::mutex cleanup_mutex;
    static constexpr std::chrono::minutes CACHE_TTL{5}; // 5 minutes TTL

    LunaAdBlocker(const std::filesystem::path base_path) {
        this->lists_dir = base_path;
        this->timestamps_db = base_path / "timestamps_db";
        curl_global_init(CURL_GLOBAL_DEFAULT);
        this->cleanup_running = true;
        this->cleanup_thread = std::thread(&LunaAdBlocker::cache_cleanup_loop, this);
    }
    
    ~LunaAdBlocker() {
        this->flush();
        curl_global_cleanup();
    }

    void flush() {
        // Signal the cleanup thread to exit
        bool was_running = this->cleanup_running.exchange(false);
        if (was_running) {
            this->cleanup_cv.notify_all();
        }
        // Clear the cache (before joining thread to avoid race)
        {
            std::lock_guard<std::mutex> lock(this->cache_mutex);
            this->cache.clear();
            this->hide_cache_data.clear();
            this->hide_cache_expiry.clear();
        }
        // Now join the thread
        if (was_running && this->cleanup_thread.joinable()) {
            this->cleanup_thread.join();
        }
    }

    static std::string escape_json(const std::string& s) {
        std::string r;
        r.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '"': r += "\\\""; break;
                case '\\': r += "\\\\"; break;
                case '\n': r += "\\n"; break;
                case '\r': r += "\\r"; break;
                case '\t': r += "\\t"; break;
                default: r += c;
            }
        }
        return r;
    }

    std::string get_hiding_rules_for_domain(const std::string& domain) {
        {
            std::lock_guard<std::mutex> lock(this->cache_mutex);
            auto it = this->hide_cache_expiry.find(domain);
            if (it != this->hide_cache_expiry.end() && it->second > std::chrono::steady_clock::now()) {
                return this->hide_cache_data[domain];
            }
        }

        std::string result = "[";
        bool first = true;
        for (const auto& rule : this->content_rules) {
            if (rule.selector.empty()) continue;

            bool apply = false;

            if (rule.options.domains.empty()) {
                apply = true;
            } else {
                for (const auto& d : rule.options.domains) {
                    if (domain == d || domain.ends_with("." + d)) {
                        apply = true;
                        break;
                    }
                }
                if (apply) {
                    for (const auto& d : rule.options.exclude_domains) {
                        if (domain == d || domain.ends_with("." + d)) {
                            apply = false;
                            break;
                        }
                    }
                }
            }

            if (apply) {
                if (!first) result += ",";
                first = false;
                result += "{\"selector\":\"" + escape_json(rule.selector) + "\"}";
            }
        }
        result += "]";

        {
            std::lock_guard<std::mutex> lock(this->cache_mutex);
            this->hide_cache_data[domain] = result;
            this->hide_cache_expiry[domain] = std::chrono::steady_clock::now() + CACHE_TTL;
        }

        return result;
    }

#ifdef LUNA_TESTING
    size_t cache_size() {
        std::lock_guard<std::mutex> lock(this->cache_mutex);
        return this->cache.size();
    }

    void clear_cache() {
        std::lock_guard<std::mutex> lock(this->cache_mutex);
        this->cache.clear();
    }
#endif // LUNA_TESTING

    void cache_cleanup_loop() {
        std::unique_lock<std::mutex> lock(this->cleanup_mutex);
        while (this->cleanup_running) {
            // Wait for 50ms or until signaled to exit
            this->cleanup_cv.wait_for(lock, std::chrono::milliseconds(50), [this]() {
                return !this->cleanup_running;
            });
            
            if (!this->cleanup_running) break;
            
            std::lock_guard<std::mutex> cache_lock(this->cache_mutex);
            const auto now = std::chrono::steady_clock::now();
            for (auto it = this->cache.begin(); it != this->cache.end(); ) {
                if (it->second.expires_at <= now) {
                    it = this->cache.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    void setup(std::initializer_list<const char*> lists) {
        if (!std::filesystem::exists(this->lists_dir)) {
            if (!std::filesystem::create_directories(this->lists_dir)) LUNA_FAIL("The adblocker lists directory can't be created at `{}`, please cheack your permissions", this->lists_dir.c_str());
        }
        if (std::filesystem::exists(this->timestamps_db)) {
            auto *file = fopen(this->timestamps_db.c_str(), "r");
            if (!file) {
                LUNA_LOG("Cant open {} to load the latest update timestamps, please check your permissions", this->timestamps_db.c_str());
            } else {
                char* buf = nullptr;
                size_t n = 0;
                ssize_t bytes_read;
                while ((bytes_read = getline(&buf, &n, file)) != -1) {
                    std::string line(buf);

                    if (!line.empty() && line.back() == '\n') line.pop_back();
                    if (!line.empty() && line.back() == '\r') line.pop_back();

                    const auto pos = line.find(':');
                    if (pos == std::string::npos) continue;

                    const auto key = line.substr(0, pos);
                    if (!this->timestamps.contains(key) && line.size() > pos + 1) {
                        const auto val = str2u64(std::string_view(line.c_str() + pos + 1));
                        this->timestamps.emplace(key, val);
                    }
                }
                free(buf);
                fclose(file);
            }
        }
        for (const auto list : lists) {
            std::string_view url_sv(list);
            std::string name(url_sv.substr(url_sv.find_last_of('/') + 1));
            if (name.empty()) name = "list_" + std::to_string(std::hash<std::string_view>{}(url_sv));
            this->add_list(url_sv, name);
        }
    }

    bool add_list(std::string_view url, std::string_view name) {
        for (const auto& list : this->lists) {
            if (list.url == url) {
                LUNA_LOG("{} arleady exists in the adblocker lists, maybe try update instead", url);
                return false;
            }
            if (list.name == name) {
                LUNA_LOG("List name `{}` already in use, try a different name, tha name has to be unique", name);
                return false;
            }
        }
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        const auto filter_path = this->lists_dir / name;

        auto write_timestamp = [this](std::string_view key, uint64_t ts) {
            std::vector<std::string> lines;
            if (std::filesystem::exists(this->timestamps_db)) {
                std::ifstream db(this->timestamps_db);
                std::string line;
                while (std::getline(db, line)) {
                    if (!line.empty()) {
                        if (line.back() == '\n') line.pop_back();
                        if (line.back() == '\r') line.pop_back();
                        lines.push_back(line);
                    }
                }
            }

            bool found = false;
            for (auto& l : lines) {
                auto pos = l.find(':');
                if (pos != std::string::npos && l.substr(0, pos) == key) {
                    l = std::string(key) + ":" + std::to_string(ts);
                    found = true;
                    break;
                }
            }
            if (!found) lines.push_back(std::string(key) + ":" + std::to_string(ts));

            std::ofstream out(this->timestamps_db, std::ios::trunc);
            for (auto& l : lines) out << l << "\n";
        };

        auto read_timestamp = [this](std::string_view key) -> uint64_t {
            if (!std::filesystem::exists(this->timestamps_db)) return 0;
            std::ifstream db(this->timestamps_db);
            std::string line;
            while (std::getline(db, line)) {
                if (!line.empty()) {
                    if (line.back() == '\n') line.pop_back();
                    if (line.back() == '\r') line.pop_back();
                    auto pos = line.find(':');
                    if (pos != std::string::npos && line.substr(0, pos) == key) {
                        return str2u64(line.substr(pos + 1));
                    }
                }
            }
            return 0;
        };

        uint16_t expires_days = 10;
        if (std::filesystem::exists(filter_path)) {
            const size_t MAX_HEADER = 19;
            std::ifstream f(filter_path);
            std::string line;
            size_t i = 0;
            while (std::getline(f, line) && i < MAX_HEADER) {
                if (line.rfind("! Expires", 0) == 0) {
                    auto pos = line.find(':');
                    if (pos != std::string::npos) {
                        std::string rest = line.substr(pos + 1);
                        std::string num_str;
                        for (const auto ch : rest) {
                            if (std::isdigit(ch)) {
                                num_str += ch;
                            } else if (!num_str.empty()) {
                                break;
                            }
                        }
                        if (!num_str.empty()) {
                            expires_days = static_cast<uint16_t>(str2u64(num_str));
                        }
                    }
                    break;
                }
                ++i;
            }
            uint64_t last_ts = read_timestamp(name);
            uint64_t expires_sec = expires_days * 86400;

            if (now - last_ts >= expires_sec) {
                if (!download_list(url, filter_path)) return false;
                write_timestamp(name, now);
            }
        } else {
            if (!download_list(url, filter_path)) return false;
            write_timestamp(name, now);
        }
        this->lists.emplace_back(LunaAdBlockerList{std::string(name), std::string(url)});
        this->parse_list_file(filter_path);
        return true;
    }

    void update_lists() {
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        auto write_timestamp = [this](std::string_view key, uint64_t ts) {
            std::vector<std::string> lines;
            if (std::filesystem::exists(this->timestamps_db)) {
                std::ifstream db(this->timestamps_db);
                std::string line;
                while (std::getline(db, line)) {
                    if (!line.empty()) {
                        if (line.back() == '\n') line.pop_back();
                        if (line.back() == '\r') line.pop_back();
                        lines.push_back(line);
                    }
                }
            }

            bool found = false;
            for (auto& l : lines) {
                auto pos = l.find(':');
                if (pos != std::string::npos && l.substr(0, pos) == key) {
                    l = std::string(key) + ":" + std::to_string(ts);
                    found = true;
                    break;
                }
            }
            if (!found) lines.push_back(std::string(key) + ":" + std::to_string(ts));

            std::ofstream out(this->timestamps_db, std::ios::trunc);
            for (auto& l : lines) out << l << "\n";
        };

        auto read_timestamp = [this](std::string_view key) -> uint64_t {
            if (!std::filesystem::exists(this->timestamps_db)) return 0;
            std::ifstream db(this->timestamps_db);
            std::string line;
            while (std::getline(db, line)) {
                if (!line.empty()) {
                    if (line.back() == '\n') line.pop_back();
                    if (line.back() == '\r') line.pop_back();
                    auto pos = line.find(':');
                    if (pos != std::string::npos && line.substr(0, pos) == key) {
                        return str2u64(line.substr(pos + 1));
                    }
                }
            }
            return 0;
        };

        for (auto& list : this->lists) {
            const auto filter_path = this->lists_dir / list.name;
            uint16_t expires_days = 10;

            if (std::filesystem::exists(filter_path)) {
                const size_t MAX_HEADER = 19;
                std::ifstream f(filter_path);
                std::string line;
                size_t i = 0;
                while (std::getline(f, line) && i < MAX_HEADER) {
                    if (line.rfind("! Expires", 0) == 0) {
                        auto pos = line.find(':');
                        if (pos != std::string::npos) {
                            std::string rest = line.substr(pos + 1);
                            std::string num_str;
                            for (const auto ch : rest) {
                                if (std::isdigit(ch)) {
                                    num_str += ch;
                                } else if (!num_str.empty()) {
                                    break;
                                }
                            }
                            if (!num_str.empty()) {
                                expires_days = static_cast<uint16_t>(str2u64(num_str));
                            }
                        }
                        break;
                    }
                    ++i;
                }
                uint64_t last_ts = read_timestamp(list.name);
                uint64_t expires_sec = expires_days * 86400;

                if (now - last_ts >= expires_sec) {
                    LUNA_LOG("Updating adblock list: {}", list.name);
                    this->network_rules.clear();
                    this->network_exception_rules.clear();
                    this->content_rules.clear();

                    if (download_list(list.url, filter_path)) {
                        write_timestamp(list.name, now);
                        this->parse_list_file(filter_path);
                    }
                }
            }
        }
    }

    static bool download_list(std::string_view url_sv, const std::filesystem::path& out_path) {
        LUNA_LOG("LunaAdBlocker: start downloading {} at {}", url_sv, out_path.c_str());
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string buffer;
        std::string url_str(url_sv);

        curl_easy_setopt(curl, CURLOPT_URL, url_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) return false;

        std::ofstream out(out_path, std::ios::binary);
        out.write(buffer.data(), buffer.size());
        out.close();

        return true;
    }

    void parse_list_file(const std::filesystem::path& path) {
        std::ifstream f(path);
        std::string line;

        bool in_supported_block = true;  // Track if we're in a conditional block we support
        std::vector<bool> conditional_stack;

        while (std::getline(f, line)) {
            // Handle conditional blocks
            if (line.rfind("!#if ", 0) == 0) {
                std::string condition = line.substr(4);  // Skip "!#if"
                // Trim whitespace
                condition.erase(0, condition.find_first_not_of(" \t"));
                condition.erase(condition.find_last_not_of(" \t") + 1);
                
                bool supported = false;
                // Check if this is a condition we want to include
                if (condition == "ext_luna" || condition == "env_luna") {
                    supported = true;
                }
                // Skip ext_ubol, env_safari, etc.
                
                conditional_stack.push_back(in_supported_block);
                in_supported_block = in_supported_block && supported;
                continue;
            }
            
            if (line.rfind("!#endif", 0) == 0) {
                if (!conditional_stack.empty()) {
                    in_supported_block = conditional_stack.back();
                    conditional_stack.pop_back();
                }
                continue;
            }

            // Skip if we're in an unsupported conditional block
            if (!in_supported_block) continue;

            // Skip comments
            if (line.empty() || line[0] == '!') continue;

            auto rule = LunaAdBlockerRule::parse(line);

            switch (rule.type) {
                case LunaAdBlockerRuleType::NetworkBlockRule:
                    network_rules.push_back(std::move(rule));
                    break;
                case LunaAdBlockerRuleType::NetworkExceptionRule:
                    network_exception_rules.push_back(std::move(rule));
                    break;
                case LunaAdBlockerRuleType::ContentHideRule:
                    content_rules.push_back(std::move(rule));
                    break;
            }
        }
    }

    bool block_request(std::string_view url, uint32_t resource_type = 0, std::string_view document_domain = "") {
        // Exempt internal luna:* URLs from ad blocking
        if (url.rfind("luna:", 0) == 0) {
            return false;
        }

        // Build cache key
        std::string cache_key = std::string(url) + ":" + std::to_string(resource_type) + ":" + std::string(document_domain);

        // Check cache
        {
            std::lock_guard<std::mutex> lock(this->cache_mutex);
            const auto it = this->cache.find(cache_key);
            if (it != this->cache.end()) {
                return it->second.result; // short-circut and skip the havy lookups
            }
        }

        // Compute result
        bool result = false;
        bool found = false;
        for (auto& exc : this->network_exception_rules) {
            if (exc.match(url, resource_type, document_domain)) {
#ifdef LUNA_TESTING
                LUNA_DEBUG("Ignored by rule: {}", exc.original_rule);
#endif // LUNA_TESTING
               result = false;
               found = true;
               break;
            }
        }
        if (!found) {
            for (auto& blk : this->network_rules) {
                if (blk.match(url, resource_type, document_domain)) {
#ifdef LUNA_TESTING
                    LUNA_DEBUG("Blocked by rule: {}", blk.original_rule);
#endif // LUNA_TESTING
                   result = true;
                   break;
                }
            }
        }

        // Store in cache, for the next time :D
        {
            std::lock_guard<std::mutex> lock(this->cache_mutex);
            auto expires_at = std::chrono::steady_clock::now() + CACHE_TTL;
            this->cache[cache_key] = CacheEntry{result, expires_at};
        }
        return result;
    }

};

struct NetworkAdBlocker: QWebEngineUrlRequestInterceptor {
    LunaAdBlocker &adblocker;

    NetworkAdBlocker(LunaAdBlocker &adblocker) : adblocker(adblocker) {
    }

    void interceptRequest(QWebEngineUrlRequestInfo &info) override {
        uint32_t resource_type = 0;
        switch (info.resourceType()) {
            case QWebEngineUrlRequestInfo::ResourceTypeScript:
                resource_type = 1 << 0;
                break;
            case QWebEngineUrlRequestInfo::ResourceTypeImage:
                resource_type = 1 << 1;
                break;
            case QWebEngineUrlRequestInfo::ResourceTypeStylesheet:
                resource_type = 1 << 2;
                break;
            case QWebEngineUrlRequestInfo::ResourceTypeObject:
                resource_type = 1 << 3;
                break;
            case QWebEngineUrlRequestInfo::ResourceTypeSubFrame:
                resource_type = 1 << 4;
                break;
            case QWebEngineUrlRequestInfo::ResourceTypeXhr:
                resource_type = 1 << 5;
                break;
            case QWebEngineUrlRequestInfo::ResourceTypeWebSocket:
                resource_type = 1 << 6;
                break;
            default:
                break;
        }
        const auto url = info.requestUrl().toString().toStdString();
        const auto first_party = info.firstPartyUrl().toString().toStdString();

        // Extract document domain from first-party URL
        std::string_view document_domain;
        size_t scheme_end = first_party.find("://");
        if (scheme_end != std::string::npos) {
            std::string_view host(first_party);
            host.remove_prefix(scheme_end + 3);
            size_t path_start = host.find_first_of("/:#?");
            if (path_start != std::string_view::npos) {
                document_domain = host.substr(0, path_start);
            } else {
                document_domain = host;
            }
        }

        const auto block = this->adblocker.block_request(url, resource_type, document_domain);
        if (block) {
            total_blocked_ads++;
            LUNA_DEBUG("LunaAdBlocker: blocked [{}]: {}", total_blocked_ads, url);
            info.block(true);
        }
    }
};

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
        QWebEngineUrlScheme::LocalAccessAllowed |
        QWebEngineUrlScheme::ViewSourceAllowed |
        QWebEngineUrlScheme::ContentSecurityPolicyIgnored |
        QWebEngineUrlScheme::FetchApiAllowed
    );
    QWebEngineUrlScheme::registerScheme(scheme);
}

struct LunaBrowserHistory {
    std::filesystem::path file_path;
    std::vector<std::string> entries;
    size_t new_start = THE_ZERO;

    LunaBrowserHistory() = default;
    LunaBrowserHistory(const std::filesystem::path history_file_path) {
        this->file_path = history_file_path;
    }

    void load() {
        const auto capacity = 4096;
        if (std::filesystem::exists(this->file_path)) {
            const auto file_size = std::filesystem::file_size(this->file_path);
            if (file_size > capacity ) this->entries.reserve(file_size / 2);
            else this->entries.reserve(capacity);
            auto *file = fopen(this->file_path.c_str(), "r");
            if (!file) {
                LUNA_LOG("Cant open {} to load the history, please check your permissions", this->file_path.c_str());
                return;
            }
            char* buf = nullptr;
            size_t n = 0;
            while (getline(&buf, &n, file) != -1) {
                entries.emplace_back(buf);
                free(buf); // dont be like the chrome browser
                buf = nullptr;
                n = 0;
            }
            free(buf);
            fclose(file);
            this->new_start = entries.size();
        } else {
            const auto parent_path = this->file_path.parent_path();
            if (!std::filesystem::exists(parent_path)) {
                if (!std::filesystem::create_directories(parent_path)) LUNA_FAIL("The history file can't be created at `{}`, please cheack your permissions", parent_path.c_str());
            }
            this->entries.reserve(capacity);
        }
    }

    bool save() {
        auto *file = fopen(this->file_path.c_str(), "a");
        if (!file) {
            LUNA_LOG("Cant open {} to update the history, please check your permissions", this->file_path.c_str());
            return false;
        }
        const auto entries_size = this->entries.size();
        for (size_t i = this->new_start; i < entries_size; i++) {
            std::fprintf(file, "%s\n", this->entries[i].c_str());
        }
        fclose(file);
        this->new_start = entries_size;
        return true;
    }

    void append(const QUrl url) {
        auto byte_arr = url.toEncoded();
        this->entries.emplace_back(byte_arr.constData(), byte_arr.size());
    }

    void add_search(const char* query) {
        this->entries.emplace_back(query);
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
    std::filesystem::path profile_path;
    LunaBrowserHistory history;
    LunaBrowserBookmarks bookmarks;
    QWebEngineProfile *web_engine_profile;
    QWebEngineUrlRequestInterceptor *interceptor = nullptr;

    LunaBrowserProfile(std::filesystem::path profile_base, char* name, LunaAdBlocker& adblocker, bool disable_adblocker = false) : name(name), profile_path(profile_base) {
        LunaBrowserHistory history(profile_base / "history");
        this->history = history;
        LunaBrowserBookmarks bookmarks(profile_base / "bookmarks");
        this->bookmarks = bookmarks;

        auto *web_engine_profile = new QWebEngineProfile(QString(name));
        web_engine_profile->scripts()->insert(makeFModeScript());
        
        web_engine_profile->setCachePath((profile_base / "cache").c_str());
        web_engine_profile->installUrlSchemeHandler(LUNA_PREFEX, new LunaBrowserSchemeHandler());
        if (!disable_adblocker) {
            this->interceptor = new NetworkAdBlocker(adblocker);
            web_engine_profile->setUrlRequestInterceptor(this->interceptor);
        }
        this->web_engine_profile = web_engine_profile;
    }

    void destroy() {
        if (this->web_engine_profile) {
            this->web_engine_profile->setUrlRequestInterceptor(nullptr);
            delete this->interceptor;
            this->interceptor = nullptr;
            delete this->web_engine_profile;
            this->web_engine_profile = nullptr;
        }
    }
};

struct LunaBrowserOptions {
    bool disable_adblocker = false;
    bool disable_cache = false;
    bool no_js = false;
    bool force_darkmode = false;
    bool frameless = false;
    bool private_window = false;
    std::string session_name;
    std::vector<std::string> urls;
    std::vector<std::string> commands;
};

struct LunaBrowser: QMainWindow {
    BrowserMode mode = BrowserMode::NormalMode;
    QTabWidget *tabs;
    size_t tabs_count = THE_ZERO;
    StatusBar* status_bar;
    LunaBrowserOptions opts;
    LunaBrowserProfile profile;
    LunaAdBlocker& adblocker;
    QLineEdit *user_input; // command/search
    QCompleter *completer;
    QStringListModel *completer_model;
    QListView *completer_popup;
    QStringList base_commands;
    std::array<TabRestoreState, TAB_RESTORE_MAX_COUNT> recent_restore_states{};
    size_t restore_count = THE_ZERO; // number of entries in the queue

    QString last_error;
    QTimer *error_timer;
    QLabel *error_label;
    QStringList command_history;
    int history_index;
    QString saved_input;

    void update_mode(const BrowserMode m) {
        this->mode = m;
        this->status_bar->update_mode(m);
        // QApplication::processEvents(); // in case we neede to change ui from a none ui thread

        if (m == BrowserMode::CommandMode) {
            this->error_label->setVisible(false);
            this->error_timer->stop();
            this->user_input->setVisible(true);
            this->user_input->setFocus();
            this->user_input->setCursorPosition(1);
            this->history_index = -1;
            this->saved_input.clear();
            this->completer_model->setStringList(this->base_commands);
            QTimer::singleShot(0, this, [this]() {
                this->completer->complete();
            });
        } else if (m == BrowserMode::SearchMode) {
            this->error_label->setVisible(false);
            this->error_timer->stop();
            this->user_input->setVisible(true);
            this->user_input->setFocus();
            this->user_input->clear();
            this->user_input->setPlaceholderText("Search...");
        } else {
            this->user_input->setVisible(false);
            this->user_input->clear();
        }
    }

    void update_tab_title(TabBody *tab_body) {
        if (!tab_body) return;
        auto *active = tab_body->active_veiw();
        if (!active) return;
        int tab_idx = this->tabs->indexOf(tab_body);
        if (tab_idx == -1) return;
        QString title = active->title();
        if (title.isEmpty()) title = active->url().toString();
        if (title.isEmpty()) title = "Luna";
        if (tab_body->tag == TabBodyStateTag::SplitedTagBodyState && tab_body->val.splitter) {
            int active_idx = tab_body->val.splitter->indexOf(active);
            if (active_idx >= 0) {
                const auto other_idx = (active_idx == THE_ZERO) ? 1 : THE_ZERO;
                title = QString("%1|%2: %3").arg(other_idx + 1).arg(active_idx + 1).arg(title);
            }
        } else {
            title = QString("%1: %2").arg(tab_idx + 1).arg(title);
        }
        this->tabs->setTabText(tab_idx, title);
    }

    LunaBrowser(const LunaBrowserOptions opts, const LunaBrowserProfile profile, LunaAdBlocker& adblocker): opts(opts), profile(profile), adblocker(adblocker) {
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

        this->user_input = new QLineEdit(this);
        this->user_input->setVisible(false);
        this->user_input->setStyleSheet(
            "QLineEdit {"
            "  background: #282828;"
            "  color: #ebdbb2;"
            "  border: none;"
            "  padding: 4px;"
            "}"
        );

        this->completer_model = new QStringListModel(this);
        this->base_commands = QStringList({
            "o", "open", "tabopen", "back", "forward", "reload", "stop",
            "w", "quit", "q", "wq", "undo", "redo", "yank", "paste",
            "tabnew", "tabclose", "tabnext", "tabprev",
            "scroll", "scrollpage", "search", "nohlsearch",
            "bookmark-add", "bookmark-del", "bookmark-list",
            "history", "download", "adblock-enable", "adblock-disable",
            "set", "bind", "unbind", "help",
            "x", "session-save",
        });
        this->completer_model->setStringList(this->base_commands);

        this->completer = new QCompleter(this->completer_model, this);
        this->completer->setCaseSensitivity(Qt::CaseInsensitive);
        this->completer->setCompletionMode(QCompleter::PopupCompletion);

        this->completer_popup = new QListView();
        this->completer_popup->setStyleSheet(
            "QListView {"
            "  background: #282828;"
            "  color: #ebdbb2;"
            "  border: none;"
            "  outline: none;"
            "}"
            "QListView::item:selected {"
            "  background: #3c3836;"
            "  color: #ebdbb2;"
            "}"
        );
        this->completer->setPopup(this->completer_popup);

        this->user_input->setCompleter(this->completer);
        this->user_input->installEventFilter(this);

        this->error_label = new QLabel(this);
        this->error_label->setStyleSheet("background: #cc241d; color: #ebdbb2; padding: 4px; font-weight: bold;");
        this->error_label->setVisible(false);
        this->error_label->setMinimumHeight(24);

        this->error_timer = new QTimer(this);
        this->error_timer->setSingleShot(true);
        QObject::connect(this->error_timer, &QTimer::timeout, this, [this]() {
            this->error_label->setVisible(false);
        });

        this->history_index = -1;
        this->saved_input.clear();

        QObject::connect(this->user_input, &QLineEdit::returnPressed, this, [this]() {
            QString text = this->user_input->text();
            if (this->mode == BrowserMode::SearchMode) {
                // Perform search in current tab
                if (auto *tab_body = this->active_tab()) {
                    if (auto *v = tab_body->active_veiw()) {
                        tab_body->search_term = text;
                        v->page()->findText(text, QWebEnginePage::FindFlags(), [](const QWebEngineFindTextResult &) {
                            // tab_body->find_result = &result;
                            // this->status_bar->set_search_result(QString("(%1/%2)")
                                // .arg(result.activeMatch())
                                // .arg(result.numberOfMatches()));
                        });
                    }
                }
                this->user_input->setVisible(false);
                this->update_mode(BrowserMode::NormalMode);
            } else {
                if (!text.isEmpty()) {
                    this->command_history.append(text);
                }
                this->handle_input_command(text);
            }
        });

        // QObject::connect(this->user_input, &QLineEdit::textChanged, this, [this](const QString &text) {
            // this->update_completions(text);
        // });

        vbox->addWidget(this->tabs, 1);
        vbox->addWidget(this->user_input, 0);
        vbox->addWidget(this->error_label, 0);
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
        QObject::connect(this->tabs, &QTabWidget::currentChanged, [&](int idx) {
            auto *tab_body = dynamic_cast<TabBody*>(this->tabs->currentWidget());
            if (tab_body) {
                if (tab_body->has_pending_load()) {
                    this->activate_pending_tab(tab_body);
                }
                auto *v = tab_body->active_veiw();
                if (v) {
                    this->status_bar->set_url(v->url().toString());
                    this->status_bar->set_tab_index(idx, this->tabs->count());
                    QObject::connect(v, &QWebEngineView::urlChanged, [this](const QUrl &u) {
                        this->status_bar->set_url(u.toString());
                    });
                }
                // this->update_tab_title(tab_body);
            }
        });


        QObject::connect(this->tabs, &QTabWidget::tabCloseRequested, [&](int i){
            this->close_tab(i);
        });
        // Keep smiling ^_^
    }

    void prepare() {
        this->profile.history.load();
    }

    void show() {
        LUNA_LOG("frameless: {}", this->opts.frameless);
        this->tabs->tabBar()->setVisible(!this->opts.frameless);
        this->status_bar->setVisible(!this->opts.frameless);
        if (this->opts.urls.empty()) {
            // Create the default tab
            this->new_tab(this->profile.web_engine_profile); 
        } else {
            for (std::string& url: this->opts.urls) {
                this->new_tab(this->profile.web_engine_profile, url.c_str()); 
            }
            this->opts.urls.clear(); // we are not gonna use any value of this vector after this point.
            this->opts.urls.shrink_to_fit();
        }
        QMainWindow::show();
    }

    TabBody* new_tab(QWebEngineProfile* profile, const char* tab_url = DEFAULT_PAGE_URL, const bool instantly_switch = true, const bool defer_load = false) {
        LUNA_DEBUG("Creating a new tab for `{}`", tab_url);
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
        if (!defer_load && std::strcmp(tab_url, LUNA_NEW_TAB_URL) == 0) {
            // TODO(anas): provide the recents list
        }
        auto tab_body = new TabBody(web_engine_view);
        const auto url = QUrl(tab_url);
        const size_t idx = this->tabs->addTab(tab_body, "New Tab");
        if (!defer_load) tab_body->load_timer.start();
        if (instantly_switch) {
            this->tabs->setCurrentIndex(idx);
        }
        // when the page dose load update the tab text
        QObject::connect(web_engine_view, &QWebEngineView::titleChanged, [tab_body, this](const QString &) {
            // std::print("{}: {}\n", this->tabs->indexOf(parent), t.toStdString());
            this->update_tab_title(tab_body);
        });
        QObject::connect(web_engine_view->page(), &QWebEnginePage::newWindowRequested, [profile, this](QWebEngineNewWindowRequest &request) {
            if (!request.isUserInitiated()) {
                // TODO(anas): cheack the allow list, or ask the user first
            }
            switch(request.destination()) {
                case QWebEngineNewWindowRequest::InNewWindow: {
                    // Create a new browser window
                    LunaBrowser* new_browser = new LunaBrowser(this->opts, this->profile, this->adblocker);
                    new_browser->setAttribute(Qt::WA_DeleteOnClose);
                    // Set the requested URL to load in the new window
                    QString requested_url = request.requestedUrl().toString();
                    new_browser->opts.urls.push_back(requested_url.toStdString());
                    // Prepare and show the new browser
                    new_browser->prepare();
                    new_browser->show();
                    // Notify the request about the page to open in
                    if (new_browser->tabs->count() > 0) {
                        TabBody* first_tab = dynamic_cast<TabBody*>(new_browser->tabs->widget(0));
                        if (first_tab) {
                            if (QWebEngineView* view = first_tab->active_veiw()) {
                                request.openIn(view->page());
                            }
                        }
                    }
                } break;
                case QWebEngineNewWindowRequest::InNewTab: {
                    auto *tb= this->new_tab(profile);
                    if (auto* v = tb->active_veiw()) request.openIn(v->page());
                } break;
                case QWebEngineNewWindowRequest::InNewDialog: {
                    // Minimal window without tab bar, toolbar, or URL bar
                    QMainWindow* dialog_window = new QMainWindow();
                    dialog_window->setAttribute(Qt::WA_DeleteOnClose);
                    dialog_window->setWindowTitle("Luna Dialog");
                    
                    QWebEngineView* dialog_view = new QWebEngineView(profile, dialog_window);
                    if (this->opts.no_js) {
                        dialog_view->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
                    }
                    if (this->opts.force_darkmode) {
                        dialog_view->settings()->setAttribute(QWebEngineSettings::ForceDarkMode, true);
                    }
                    
                    dialog_window->setCentralWidget(dialog_view);
                    QUrl requested_url = request.requestedUrl();
                    dialog_view->load(requested_url.isEmpty() ? QUrl(DEFAULT_PAGE_URL) : requested_url);
                    dialog_window->show();
                    
                    request.openIn(dialog_view->page());
                } break;
                case QWebEngineNewWindowRequest::InNewBackgroundTab: {
                    // New tab in the same window, without switching to it
                    auto *tb = this->new_tab(profile, DEFAULT_PAGE_URL, false);
                    if (QWebEngineView* view = tb->active_veiw()) {
                        request.openIn(view->page());
                    }
                } break;
            }
        });
        QObject::connect(web_engine_view, &QWebEngineView::urlChanged, this, [this](const QUrl u) {
            if (u.scheme() != LUNA_PREFEX) this->profile.history.append(u);
        });
        QObject::connect(web_engine_view, &QWebEngineView::loadStarted, this, [web_engine_view, this]() {
            auto *parent = web_engine_view->parentWidget();
            while (parent && !dynamic_cast<TabBody*>(parent)) parent = parent->parentWidget();
            auto *tab_body = static_cast<TabBody*>(parent);
            tab_body->load_timer.start();
        });
        QObject::connect(web_engine_view, &QWebEngineView::loadFinished, this, [web_engine_view, this]() {
            auto *parent = web_engine_view->parentWidget();
            while (parent && !dynamic_cast<TabBody*>(parent)) parent = parent->parentWidget();
            auto *tab_body = static_cast<TabBody*>(parent);
            tab_body->load_timer.stop();
        });
        QObject::connect(web_engine_view, &QWebEngineView::loadProgress, this, [web_engine_view, this](const size_t progress) {
            auto *parent = web_engine_view->parentWidget();
            while (parent && !dynamic_cast<TabBody*>(parent)) parent = parent->parentWidget();
            auto *tab_body = static_cast<TabBody*>(parent);
            const auto current_idx = tab_body ? this->tabs->indexOf(tab_body) : -1;
            if (current_idx == this->tabs->currentIndex() && tab_body) {
                const uint64_t elapsed = tab_body->load_timer.elapsed();
                this->status_bar->set_load_time(elapsed);
                this->status_bar->set_progress(progress);
            }
        });
        QObject::connect(web_engine_view->page(), &QWebEnginePage::scrollPositionChanged, this, [web_engine_view, this](const QPointF &) {
            auto *parent = web_engine_view->parentWidget();
            while (parent && !dynamic_cast<TabBody*>(parent)) parent = parent->parentWidget();
            auto *tab_body = static_cast<TabBody*>(parent);
            const auto current_idx = tab_body ? this->tabs->indexOf(tab_body) : -1;
            if (current_idx == this->tabs->currentIndex() && tab_body) {
                web_engine_view->page()->runJavaScript("Math.round((window.scrollY / (document.body.scrollHeight - window.innerHeight)) * 100)", [this](const QVariant &val) {
                    const int pct = val.toInt();
                    if (pct <= 0) this->status_bar->set_position("[top]");
                    else this->status_bar->set_position(QString("[%1%]").arg(pct));
                });
            }
        });
        this->tabs_count += 1;

        // Inject domain-specific content hide rules
        if (!this->opts.disable_adblocker) {
            QString domain = url.host();
            if (!domain.isEmpty()) {
                std::string rules_json = this->adblocker.get_hiding_rules_for_domain(domain.toStdString());
                if (rules_json != "[]") {
                    QWebEngineScript s;
                    s.setName("content-hide");
                    s.setInjectionPoint(QWebEngineScript::DocumentReady);
                    s.setRunsOnSubFrames(true);
                    s.setWorldId(QWebEngineScript::MainWorld);
                    std::string js = R"JS(
                        (function(){
                            var rules = )JS" + rules_json + R"JS(;
                            var css = "";
                            for (var i = 0; i < rules.length; i++) {
                                css += rules[i].selector + " { display: none !important; }\n";
                            }
                            if (css) {
                                var style = document.createElement("style");
                                style.textContent = css;
                                if (document.head) document.head.appendChild(style);
                            }
                        })();
                        )JS";
                    s.setSourceCode(QString::fromStdString(js));
                    web_engine_view->page()->scripts().insert(s);
                }
            }
        }

        // load the url
        if (!defer_load) {
            web_engine_view->load(url);
        }
        return tab_body;
    }

    void close_tab(const size_t idx) {
        if (this->tabs_count == 1) this->new_tab(this->profile.web_engine_profile, DEFAULT_PAGE_URL, false);
        QWidget *w = this->tabs->widget(idx);
        // Save tab state for restoring
        if (auto *tab_body = dynamic_cast<TabBody*>(w)) {
            if (auto *v = tab_body->active_veiw()) {
                TabRestoreState state;
                state.url = v->url().toString();
                state.scroll_position = v->page()->scrollPosition();
                state.tab_index = idx;
                state.search_term = tab_body->search_term;
                if (state.is_valid()) {
                    if (this->restore_count < TAB_RESTORE_MAX_COUNT) {
                        // Shift all existing entries right to make room at index 0
                        for (size_t i = this->restore_count; i > 0; i--) {
                            this->recent_restore_states[i] = this->recent_restore_states[i - 1];
                        }
                        this->restore_count++;
                    } else {
                        // Array is full, shift right dropping the oldest at the end
                        for (size_t i = TAB_RESTORE_MAX_COUNT - 1; i > 0; i--) {
                            this->recent_restore_states[i] = this->recent_restore_states[i - 1];
                        }
                    }
                    this->recent_restore_states[THE_ZERO] = state;
                }
            }
        }
        this->tabs->removeTab(idx);
        this->tabs_count -= 1;
        delete w; // dont eat memory :)
    }

    void close_selected_veiw(const size_t idx) {
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
        LUNA_DEBUG("keyPressEvent: {}\n", QKeySequence(mods | key).toString().toStdString());
        if (this->mode == BrowserMode::NormieMode) {
            if (mods & Qt::ControlModifier & Qt::ShiftModifier) {
                if (key == Qt::Key_Escape) {
                    this->update_mode(BrowserMode::NormalMode);
                    return true; // ayoo
                }
            }
            LUNA_LOG("{}", "NormieMode is enabled: forward all keys to the webpage\n");
            return false;
        }

        auto *v = this->active_tab()->active_veiw();

        if (mods & Qt::ControlModifier) {
            bool handled = false;
            switch (key) {
                case Qt::Key_T: {
                    if ((mods & Qt::ShiftModifier) && !(mods & (Qt::AltModifier | Qt::MetaModifier))) {
                        // ctrl+shift+t: restore latest closed tab
                        this->reopen_latest_tab();
                    } else {
                        // ctrl+t: new tab
                        this->new_tab(this->profile.web_engine_profile);
                    }
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
                case Qt::Key_Slash: this->update_mode(BrowserMode::SearchMode); break;
                case Qt::Key_N: {
                    // N: next search result, Shift+N: previous search result
                    if (auto *tab_body = this->active_tab()) {
                        if (!tab_body->search_term.isEmpty()) {
                            QWebEnginePage::FindFlags flags;
                            if (mods & Qt::ShiftModifier) {
                                flags = QWebEnginePage::FindBackward;
                            } else {
                                flags = QWebEnginePage::FindFlags();
                            }
                            tab_body->active_veiw()->page()->findText(tab_body->search_term, flags, [](const QWebEngineFindTextResult &) {
                                // tab_body->find_result = &result;
                                // this->status_bar->set_search_result(QString("(%1/%2)")
                                    // .arg(result.activeMatch())
                                    // .arg(result.numberOfMatches()));
                            });
                        }
                    }
                } break;
                case Qt::Key_F: {
                    this->update_mode(BrowserMode::CaretMode);
                    v->page()->runJavaScript("window.__fmode_start && window.__fmode_start();");
                } break;
                case Qt::Key_S: {
                    auto* tb = dynamic_cast<TabBody*>(this->tabs->currentWidget());
                    if(tb) {
                        auto *nv = new QWebEngineView(this->profile.web_engine_profile);
                        nv->load(QUrl(DEFAULT_PAGE_URL));
                        if (!tb->split(SplitHorizontallyDirection, nv)) { this->show_error("Target tab is already in split mode!"); break; }
                        QObject::connect(nv, &QWebEngineView::titleChanged, [nv, this](const QString &) {
                            auto *parent = nv->parentWidget();
                            while (parent && !dynamic_cast<TabBody*>(parent)) parent = parent->parentWidget();
                            if (parent) this->update_tab_title(static_cast<TabBody*>(parent));
                        });
                        this->status_bar->set_url(nv->url().toString());
                        QObject::connect(nv, &QWebEngineView::urlChanged, [this](const QUrl &u) {
                            this->status_bar->set_url(u.toString());
                        });
                    }
                } break;
                case Qt::Key_V: {
                    auto* tb = dynamic_cast<TabBody*>(this->tabs->currentWidget());
                    if(tb) {
                        auto *nv = new QWebEngineView(this->profile.web_engine_profile);
                        nv->load(QUrl(DEFAULT_PAGE_URL));
                        if(!tb->split(SplitVerticallyDirection, nv)) { this->show_error("Target tab is already in split mode!"); break; }
                        QObject::connect(nv, &QWebEngineView::titleChanged, [nv, this](const QString &) {
                            auto *parent = nv->parentWidget();
                            while (parent && !dynamic_cast<TabBody*>(parent)) parent = parent->parentWidget();
                            if (parent) this->update_tab_title(static_cast<TabBody*>(parent));
                        });
                        this->status_bar->set_url(nv->url().toString());
                        QObject::connect(nv, &QWebEngineView::urlChanged, [this](const QUrl &u) {
                            this->status_bar->set_url(u.toString());
                        });
                    }
                } break;
                case Qt::Key_Colon:
                case Qt::Key_Semicolon: this->update_mode(BrowserMode::CommandMode); break;
                case Qt::Key_Escape: {
                    if (this->mode == BrowserMode::SearchMode) {
                        this->user_input->clear();
                        this->user_input->setVisible(false);
                        this->update_mode(BrowserMode::NormalMode);
                    } else if (this->error_label->isVisible()) {
                        this->error_label->setVisible(false);
                        this->error_timer->stop();
                    }
                } break;
            }
            return true; // we handled that key event
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

            // Handle Tab completion when input is visible and focused
            if (obj == this->user_input && this->user_input->isVisible()) {
                if (e->key() == Qt::Key_Tab) {
                    if (this->completer->popup()->isVisible()) {
                        QModelIndex idx = this->completer->popup()->currentIndex();
                        if (idx.isValid()) {
                            this->user_input->setText(idx.data().toString());
                            this->completer->popup()->hide();
                        }
                    } else {
                        this->completer->complete();
                    }
                    return true;
                }
                if (e->key() == Qt::Key_Escape) {
                    this->user_input->clear();
                    this->user_input->setVisible(false);
                    this->update_mode(BrowserMode::NormalMode);
                    return true;
                }
                if (e->key() == Qt::Key_Up) {
                    if (!this->command_history.isEmpty()) {
                        if (this->history_index == -1) {
                            this->saved_input = this->user_input->text();
                            this->history_index = this->command_history.size() - 1;
                        } else if (this->history_index > 0) {
                            this->history_index--;
                        }
                        if (this->history_index >= 0) {
                            this->user_input->setText(this->command_history[this->history_index]);
                            this->user_input->setCursorPosition(this->user_input->text().length());
                        }
                    }
                    return true;
                }
                if (e->key() == Qt::Key_Down) {
                    if (this->history_index != -1) {
                        if (this->history_index < this->command_history.size() - 1) {
                            this->history_index++;
                            this->user_input->setText(this->command_history[this->history_index]);
                        } else {
                            this->history_index = -1;
                            this->user_input->setText(this->saved_input);
                        }
                        this->user_input->setCursorPosition(this->user_input->text().length());
                    }
                    return true;
                }
                return false; // let the input handle other keys
            }

            return this->handle_key_press_event(e);
        } else if (event->type() == QEvent::FocusIn) {
            // std::print("focus has changed in\n");
            this->on_focus();
            return false;
        } else if (event->type() == QEvent::FocusOut) {
            // std::print("focus has changed out\n");
            return false;
        }

        return QObject::eventFilter(obj, event);
    }

    void closeEvent(QCloseEvent *) override { 
        // flush the new entries in our history
        if (!this->opts.private_window) {
            this->profile.history.save();
        }
    }

    void on_focus() {
        {
            auto *tab_body = this->active_tab();
            if (!tab_body) return;
            auto *v = tab_body->active_veiw();
            if (!v) return;
            this->update_tab_title(tab_body);
            // update the status bar
            this->status_bar->url->setText(v->url().toString());
            const uint64_t elapsed = tab_body->load_timer.elapsed();
            this->status_bar->set_load_time(elapsed);
        }
    }

    TabBody* active_tab() {
        auto *v = dynamic_cast<TabBody*>(this->tabs->currentWidget());
        return v;
    }

    void show_error(const char* msg) {
        std::print("{}\n", msg);
    }

    void show_error_notification() {
        if (this->last_error.isEmpty()) return;
        this->error_label->setText("Error: " + this->last_error);
        this->error_label->setVisible(true);
        this->error_timer->start(5000); // 5 seconds
    }

    void reopen_latest_tab() {
        if (this->restore_count == THE_ZERO) return;
        TabRestoreState state = this->recent_restore_states[THE_ZERO];
        // Shift remaining entries left (entry at index 1 becomes index 0, etc.)
        for (size_t i = 0; i < this->restore_count - 1; i++) {
            this->recent_restore_states[i] = this->recent_restore_states[i + 1];
        }
        this->restore_count--;
        const QString url = state.url;
        auto *tab_body = this->new_tab(this->profile.web_engine_profile, url.toStdString().c_str(), false);
        if (auto *v = tab_body->active_veiw()) {
            // Restore scroll position and search term after page loads
            QObject::connect(v, &QWebEngineView::loadFinished, this, [v, state, tab_body]() {
                // Restore scroll position
                v->page()->runJavaScript(QString("window.scrollTo(%1, %2);")
                    .arg(state.scroll_position.x())
                    .arg(state.scroll_position.y()));
                // Restore search term if present
                if (!state.search_term.isEmpty()) {
                    // LUNA_LOG("Perform search for {}", state.search_term.toStdString());
                    tab_body->search_term = state.search_term;
                    v->page()->findText(state.search_term, QWebEnginePage::FindFlags(), [](const QWebEngineFindTextResult &) {
                        // TODO(anas): are we intrested in the result.count?
                    });
                }
            });
        }
        // Switch to the restored tab
        size_t new_idx = this->tabs->count() - 1;
        this->tabs->setCurrentIndex(new_idx);
    }

    std::string save_session_to_file(const std::string &name) {
        std::string filename = name;
        const std::string ext = ".luna";
        if (filename.size() < ext.size() || filename.substr(filename.size() - ext.size()) != ext) {
            filename += ext;
        }

        const auto profile_path = std::filesystem::path(get_app_data_base()) / this->profile.name;
        const auto sessions_dir = profile_path / "sessions";
        if (!std::filesystem::exists(sessions_dir)) {
            std::filesystem::create_directories(sessions_dir);
        }
        const auto file_path = sessions_dir / filename;

        std::ofstream file(file_path);
        if (!file) {
            this->last_error = QString("Cannot create session file: %1").arg(QString::fromStdString(file_path.string()));
            return "";
        }

        file << SESSION_FILE_HEADER << "\n";
        file << SESSION_FILE_VERSION_STR << "\n";
        file << this->tabs->currentIndex() << "\n"; // TEST(anas): idx + 1 ?

        const int tab_count = this->tabs->count();
        for (int i = 0; i < tab_count; ++i) {
            auto *tab_body = dynamic_cast<TabBody*>(this->tabs->widget(i));
            if (!tab_body) continue;

            if (tab_body->tag == SingleViewTagBodyState) {
                auto *v = tab_body->active_veiw();
                if (!v) continue;
                QString url = v->url().toString();
                if (url.isEmpty() || url == LUNA_NEW_TAB_URL) continue;
                QPointF scroll_pos = v->page()->scrollPosition();
                QString search = tab_body->search_term;
                QString title = v->title();
                file << url.toStdString() << ","
                     << scroll_pos.x() << ","
                     << scroll_pos.y() << ","
                     << search.toStdString() << ","
                     << title.toStdString() << ",s\n";
            } else if (tab_body->tag == SplitedTagBodyState && tab_body->val.splitter) {
                QString split_state = tab_body->val.splitter->orientation() == Qt::Horizontal ? "h" : "v";
                for (int j = 0; j < tab_body->val.splitter->count(); ++j) {
                    auto *sv = qobject_cast<QWebEngineView*>(tab_body->val.splitter->widget(j));
                    if (!sv) continue;
                    QString url = sv->url().toString();
                    if (url.isEmpty() || url == LUNA_NEW_TAB_URL) continue;
                    QPointF scroll_pos = sv->page()->scrollPosition();
                    QString search = (j == 0) ? tab_body->search_term : QString();
                    QString title = sv->title();
                    file << url.toStdString() << ","
                         << scroll_pos.x() << ","
                         << scroll_pos.y() << ","
                         << search.toStdString() << ","
                         << title.toStdString() << ","
                         << split_state.toStdString() << "\n";
                }
            }
        }

        file.close();

        const auto last_session_path = sessions_dir / "last_session";
        std::ofstream last_file(last_session_path);
        if (last_file) {
            last_file << filename;
            last_file.close();
        }

        return file_path.string();
    }

    bool load_session_from_file(const std::string &name) {
        std::string session_name = name;
        const auto profile_path = this->profile.profile_path;
        const auto sessions_dir = profile_path / "sessions";
        if (session_name.empty()) {
            const auto last_path = sessions_dir / "last_session";
            if (std::filesystem::exists(last_path)) {
                std::ifstream f(last_path);
                std::getline(f, session_name);
            }
            if (session_name.empty()) {
                return true; // in this case the user hasent created any sessions for that profile yet
            }
        }

        std::string filename = session_name;
        const std::string ext = ".luna";
        if (filename.size() < ext.size() || filename.substr(filename.size() - ext.size()) != ext) {
            filename += ext;
        }

        const auto file_path = sessions_dir / filename;
        if (!std::filesystem::exists(file_path)) {
            this->last_error = "The session file dosen't exist";
            return false;
        }

        struct SessionViewEntry {
            QString url;
            QPointF scroll_pos;
            QString search_term;
            QString title;
            QString split_state;
        };
        std::vector<SessionViewEntry> entries;
        entries.reserve(10); // no one could have more than a 10 tabs opend at the same time, right?, right?
        size_t selected_tab_idx = 0;

        std::ifstream file(file_path);
        std::string line;
        bool is_valid = false;
        if (std::getline(file, line)) { // cheack the header
            is_valid = line == SESSION_FILE_HEADER;
        }
        if (std::getline(file, line)) { // the second line should be a valid scheme version
            is_valid = line == SESSION_FILE_VERSION_STR;
        }
        if (!is_valid) {
            this->last_error = "Invalid session file headers";
            return false;
        }
        if (std::getline(file, line)) { // the third line should keep the selected tab index
            selected_tab_idx = str2u64(line); // this function will never fail just like our love :D
        }
        int line_idx = 0;
        while (std::getline(file, line)) {
            if (line.empty()) { line_idx++; continue; } // skip empty lines
            line_idx++;

            std::vector<std::string> parts;
            size_t pos = 0;
            size_t found;
            while ((found = line.find(',', pos)) != std::string::npos) {
                parts.push_back(line.substr(pos, found - pos));
                pos = found + 1;
            }
            parts.push_back(line.substr(pos));

            if (parts.empty() || parts[0].empty()) continue;

            SessionViewEntry entry;
            entry.url = QString::fromStdString(parts[0]);
            if (parts.size() >= 2) entry.scroll_pos.setX(std::stod(parts[1]));
            if (parts.size() >= 3) entry.scroll_pos.setY(std::stod(parts[2]));
            if (parts.size() >= 4) entry.search_term = QString::fromStdString(parts[3]);
            if (parts.size() >= 6) {
                entry.title = QString::fromStdString(parts[4]);
                entry.split_state = QString::fromStdString(parts[5]);
            } else if (parts.size() >= 5) {
                entry.split_state = QString::fromStdString(parts[4]);
            }
            entries.push_back(std::move(entry));
        }
        file.close();

        if (entries.empty()) return true;

        for (int i = this->tabs->count() - 1; i >= 0; --i) {
            QWidget *w = this->tabs->widget(i);
            this->tabs->removeTab(i);
            this->tabs_count--;
            delete w;
        }
        this->tabs_count = 0;

        size_t view_idx = 0;
        size_t tab_idx = 0;
        while (view_idx < entries.size()) {
            bool is_active = (tab_idx == selected_tab_idx);
            auto &entry = entries[view_idx];
            QString split_state = entry.split_state.isEmpty() ? QString("s") : entry.split_state;

            if (split_state == "s") {
                auto *tab_body = this->new_tab(this->profile.web_engine_profile,
                    entry.url.toStdString().c_str(), false, !is_active);
                int tab_index = this->tabs->indexOf(tab_body);
                if (tab_index >= 0 && !entry.title.isEmpty()) {
                    this->tabs->setTabText(tab_index, entry.title);
                }
                if (auto *v = tab_body->active_veiw()) {
                    if (is_active) {
                        QPointF scroll_pos = entry.scroll_pos;
                        QString search_term = entry.search_term;
                        QObject::connect(v, &QWebEngineView::loadFinished, this, [v, scroll_pos, search_term, tab_body]() {
                            if (scroll_pos.x() != 0 || scroll_pos.y() != 0) {
                                v->page()->runJavaScript(QString("window.scrollTo(%1, %2);")
                                    .arg(scroll_pos.x())
                                    .arg(scroll_pos.y()));
                            }
                            if (!search_term.isEmpty()) {
                                tab_body->search_term = search_term;
                                v->page()->findText(search_term, QWebEnginePage::FindFlags(), [](const QWebEngineFindTextResult &) {});
                            }
                        });
                    } else {
                        tab_body->pending_views.push_back({entry.url, entry.scroll_pos, entry.search_term, entry.title});
                    }
                }
                view_idx++;
            } else if (split_state == "h" || split_state == "v") {
                Qt::Orientation orientation = (split_state == "h") ? Qt::Horizontal : Qt::Vertical;

                auto *v1 = new QWebEngineView(this->profile.web_engine_profile);
                if (this->opts.no_js) v1->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
                if (this->opts.force_darkmode) v1->settings()->setAttribute(QWebEngineSettings::ForceDarkMode, true);

                auto *tab_body = new TabBody(v1);
                tab_body->tag = SplitedTagBodyState;

                auto *splitter = new QSplitter(orientation, tab_body);
                tab_body->layout()->removeWidget(v1);
                v1->setParent(splitter);
                splitter->addWidget(v1);

                QWebEngineView *v2 = nullptr;
                if (view_idx + 1 < entries.size()) {
                    v2 = new QWebEngineView(this->profile.web_engine_profile);
                    if (this->opts.no_js) v2->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
                    if (this->opts.force_darkmode) v2->settings()->setAttribute(QWebEngineSettings::ForceDarkMode, true);
                    v2->setParent(splitter);
                    splitter->addWidget(v2);
                }

                tab_body->val.splitter = splitter;
                tab_body->layout()->addWidget(splitter);

                int split_tab_idx = this->tabs->addTab(tab_body, "New Tab");
                if (split_tab_idx >= 0 && !entry.title.isEmpty()) {
                    this->tabs->setTabText(split_tab_idx, entry.title);
                }
                this->tabs_count++;

                auto connect_view = [this, tab_body](QWebEngineView *sv) {
                    QObject::connect(sv, &QWebEngineView::titleChanged, [tab_body, this](const QString &) {
                        this->update_tab_title(tab_body);
                    });
                    QObject::connect(sv, &QWebEngineView::urlChanged, this, [this](const QUrl u) {
                        if (u.scheme() != LUNA_PREFEX) this->profile.history.append(u);
                    });
                    QObject::connect(sv, &QWebEngineView::loadStarted, this, [sv, this]() {
                        auto *parent = sv->parentWidget();
                        while (parent && !dynamic_cast<TabBody*>(parent)) parent = parent->parentWidget();
                        if (parent) static_cast<TabBody*>(parent)->load_timer.start();
                    });
                    QObject::connect(sv, &QWebEngineView::loadFinished, this, [sv, this]() {
                        auto *parent = sv->parentWidget();
                        while (parent && !dynamic_cast<TabBody*>(parent)) parent = parent->parentWidget();
                        if (parent) static_cast<TabBody*>(parent)->load_timer.stop();
                    });
                };

                if (v1) connect_view(v1);
                if (v2) connect_view(v2);

                if (is_active) {
                    auto load_view = [this](QWebEngineView *sv, const SessionViewEntry &e) {
                        sv->load(QUrl(e.url));
                        QPointF sp = e.scroll_pos;
                        QObject::connect(sv, &QWebEngineView::loadFinished, this, [sv, sp]() {
                            if (sp.x() != 0 || sp.y() != 0) {
                                sv->page()->runJavaScript(QString("window.scrollTo(%1, %2);").arg(sp.x()).arg(sp.y()));
                            }
                        });
                    };
                    load_view(v1, entry);
                    if (v2 && view_idx + 1 < entries.size()) {
                        load_view(v2, entries[view_idx + 1]);
                    }
                } else {
                    tab_body->pending_views.push_back({entry.url, entry.scroll_pos, entry.search_term, entry.title});
                    if (v2 && view_idx + 1 < entries.size()) {
                        auto &e2 = entries[view_idx + 1];
                        tab_body->pending_views.push_back({e2.url, e2.scroll_pos, e2.search_term, e2.title});
                    }
                }

                view_idx += (v2 ? 2 : 1);
            } else {
                view_idx++;
                continue;
            }
            tab_idx++;
        }

        if (this->tabs->count() > 0 && selected_tab_idx < (size_t)this->tabs->count()) {
            this->tabs->setCurrentIndex(selected_tab_idx);
        }
        return true; // loaded successfully
    }

    void activate_pending_tab(TabBody *tab_body) {
        if (!tab_body || !tab_body->has_pending_load()) return;

        auto load_view = [this](QWebEngineView *sv, const TabBody::PendingViewState &pv) {
            sv->load(QUrl(pv.url));
            const QPointF sp = pv.scroll_position;
            const QString st = pv.search_term;
            QObject::connect(sv, &QWebEngineView::loadFinished, this, [sv, sp, st]() {
                if (sp.x() != 0 || sp.y() != 0) {
                    sv->page()->runJavaScript(QString("window.scrollTo(%1, %2);").arg(sp.x()).arg(sp.y()));
                }
                if (!st.isEmpty()) {
                    sv->page()->findText(st, QWebEnginePage::FindFlags(), [](const QWebEngineFindTextResult &) {});
                }
            });
        };

        if (tab_body->tag == TabBodyStateTag::SingleViewTagBodyState) {
            auto *v = tab_body->active_veiw();
            if (v && !tab_body->pending_views.empty()) {
                load_view(v, tab_body->pending_views[0]);
                if (!tab_body->pending_views[0].search_term.isEmpty()) {
                    tab_body->search_term = tab_body->pending_views[0].search_term;
                }
            }
        } else if (tab_body->tag == TabBodyStateTag::SplitedTagBodyState && tab_body->val.splitter) {
            size_t pv_idx = 0;
            for (int i = 0; i < tab_body->val.splitter->count() && pv_idx < tab_body->pending_views.size(); ++i) {
                auto *sv = qobject_cast<QWebEngineView*>(tab_body->val.splitter->widget(i));
                if (!sv) continue;
                load_view(sv, tab_body->pending_views[pv_idx]);
                if (pv_idx == 0 && !tab_body->pending_views[pv_idx].search_term.isEmpty()) {
                    tab_body->search_term = tab_body->pending_views[pv_idx].search_term;
                }
                pv_idx++;
            }
        }

        tab_body->pending_views.clear();
    }

    bool run_cmd(const BrowserCommands command, const QString &args) {
        auto *v = this->active_tab() ? this->active_tab()->active_veiw() : nullptr;
        switch (command) {
            case BrowserCommands::QuitCommand:
                QApplication::quit();
                return true;
            case BrowserCommands::OpenCommand:
            case BrowserCommands::NewTabComamand: {
                QString url = args;
                if (!args.isEmpty()) {
                    const bool is_url = check_valid_url(args);
                    if (is_url) {
                        if (!args.startsWith("http://") && !args.startsWith("https://")) {
                            url = "http://" + url;
                        }
                    } else {
                        url = THE_DEFAULT_SEARCH_ENGINE + QUrl::toPercentEncoding(args);
                        this->profile.history.add_search(args.toStdString().c_str());
                    }
                } else {
                    url = DEFAULT_PAGE_URL;
                }
                this->new_tab(this->profile.web_engine_profile, url.toStdString().c_str());
                return true;
            }
            case BrowserCommands::ReloadCommand:
                if (v) { v->reload(); return true; }
                this->last_error = "No active view to reload";
                return false;
            case BrowserCommands::StopCommand:
                if (v) { v->stop(); return true; }
                this->last_error = "No active view to stop";
                return false;
            case BrowserCommands::SaveAndQuiteCommmand: {
                {
                    std::string session_name = args.isEmpty() ? "default" : args.toStdString();
                    this->save_session_to_file(session_name);
                }
                QApplication::quit();
                return true;
            }
            case BrowserCommands::SaveSessionCommand: {
                std::string session_name = args.isEmpty() ? "default" : args.toStdString();
                this->save_session_to_file(session_name);
                return true;
            }
            case BrowserCommands::CloseTabCommand:
                this->close_tab(this->tabs->currentIndex());
                return true;
            case BrowserCommands::UnknownCommand:
                this->last_error = "Unknown command";
                return false;
            default:
                this->last_error = "Command not implemented";
                return false;
        }
    }

    void handle_input_command(const QString &text) {
        QString cmd = text.trimmed();
        if (cmd.isEmpty()) {
            this->user_input->setVisible(false);
            this->update_mode(BrowserMode::NormalMode);
            return;
        }

        if (cmd.startsWith(":")) {
            cmd = cmd.mid(1); // skip the ':'
        }
        LUNA_DEBUG("Command: `{}`", cmd.toStdString());

        QString cmd_name;
        QString args;
        const auto space_idx = cmd.indexOf(' ');
        if (space_idx != -1) {
            cmd_name = cmd.left(space_idx).trimmed();
            args = cmd.mid(space_idx + 1).trimmed();
        } else {
            cmd_name = cmd.trimmed();
        }

        BrowserCommands browser_command = BrowserCommands::UnknownCommand;
        if (cmd_name == "q" || cmd_name == "quit") {
            browser_command = BrowserCommands::QuitCommand;
        } else if (cmd_name == "o" || cmd_name == "open") {
            browser_command = BrowserCommands::OpenCommand;
        } else if (cmd_name == "tabopen" || cmd_name == "tabnew") {
            browser_command = BrowserCommands::NewTabComamand;
        } else if (cmd_name == "r" || cmd_name == "reload") {
            browser_command = BrowserCommands::ReloadCommand;
        } else if (cmd_name == "stop") {
            browser_command = BrowserCommands::StopCommand;
        } else if (cmd_name == "close" || cmd_name == "tabclose") {
            browser_command = BrowserCommands::CloseTabCommand;
        } else if (cmd_name == "w" || cmd_name == "wq") {
            browser_command = BrowserCommands::SaveAndQuiteCommmand;
        } else if (cmd_name == "x" || cmd_name == "session-save") {
            browser_command = BrowserCommands::SaveSessionCommand;
        }

        bool success = this->run_cmd(browser_command, args);
        if (!success) {
            this->show_error_notification();
        }

        this->user_input->clear();
        this->user_input->setVisible(false);
        this->update_mode(BrowserMode::NormalMode);
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
        "  -s, --session <name>       Restore a specific saved session\n"
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

#ifndef LUNA_TESTING
int main(int argc, char *argv[]) {
    register_luna_scheme();   // MUST be first
    QApplication app(argc, argv);  // MUST be first Qt thing
    // LunaProfile profile;
    LunaBrowserOptions opts;
    opts.commands.reserve(argc - 1); // you have to be ready for the worst
    opts.urls.reserve(argc - 1); // we will free it immediately  after using it anyway
    char* profile_name = const_cast<char*>("default");
    // Frist of the firstest process the command line arguments.
    const auto bin = argv[0];
    for (int i = 1; i < argc; ++i) {
        std::string_view s(argv[i]);

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
        } else if (s == "-s" || s == "--session") {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                opts.session_name = argv[++i];
            }
        } else if (s == "--private-window") {
            opts.private_window = true;
        } else if (!s.empty() && s[0] == '+') {
            opts.commands.push_back(std::string(s.substr(1)));
        } else {
            opts.urls.push_back(std::string(s));
        }
    }

    const auto new_tab_path = std::filesystem::path(get_app_config_base()) / LUNA_NEW_TAB_PAGE_PATH;
    if (std::filesystem::exists(new_tab_path)) {
        const auto fsize = std::filesystem::file_size(new_tab_path);

        FILE *new_tab_f = std::fopen(new_tab_path.string().c_str(), "rb");
        if (new_tab_f) {
            new_tab_raw = new char[fsize + 1];
            std::fread(new_tab_raw, 1, fsize, new_tab_f);
            new_tab_raw[fsize] = '\0';
            std::fclose(new_tab_f);
        }
    }

    if (opts.private_window) {
        // TODO(anas): dummy profile?
    }
    const auto path = std::filesystem::path(get_app_data_base()) / profile_name;
    if (!std::filesystem::exists(path)) {
        LUNA_LOG("{}", "The provided profile dosen't exists, we will create it");
        if(!std::filesystem::create_directories(path)) LUNA_FAIL("{} can't be created, please cheack your permissions", path.c_str());
    }

    LunaAdBlocker adblocker(std::filesystem::path(get_app_data_base()) / "adblocker");
    if (!opts.disable_adblocker) {
        const auto easylist_url = "https://easylist.to/easylist/easylist.txt";
        const auto easyprivacy_url = "https://easylist.to/easylist/easyprivacy.txt";
        const auto ubo_filters_url = "https://raw.githubusercontent.com/uBlockOrigin/uAssets/refs/heads/master/filters/filters.txt";
        const auto remove_yt_shorts = "https://raw.githubusercontent.com/brave/adblock-lists/refs/heads/master/brave-lists/yt-shorts.txt";
        const auto unbreak_url = "https://raw.githubusercontent.com/uBlockOrigin/uAssets/refs/heads/master/filters/unbreak.txt";
        const auto quick_fixes_url = "https://raw.githubusercontent.com/uBlockOrigin/uAssets/refs/heads/master/filters/quick-fixes.txt";
        const auto adguard_adservers = "https://raw.githubusercontent.com/AdguardTeam/AdguardFilters/refs/heads/master/BaseFilter/sections/adservers.txt";
        const auto default_lists = {
            easylist_url,
            easyprivacy_url,
            remove_yt_shorts,
            ubo_filters_url,
            adguard_adservers,
            // "file:///home/anas/code/luna/test_rules.txt",
            unbreak_url,
            quick_fixes_url,
        };
        adblocker.setup(default_lists);
        LUNA_DEBUG("Adblocker initialized with: {} network_rules, {} network_exception_rules, {}, content_rules", adblocker.network_rules.size(), adblocker.network_exception_rules.size(), adblocker.content_rules.size());
#if 0
        // Debug: print loaded rules
        LUNA_LOG("{}", "=== Loaded Network Rules ===");
        for (const auto& rule : adblocker.network_rules) {
            LUNA_LOG("  [Network] pattern='{}' exact_start={} exact_end={} domain_anchor={}", rule.pattern, rule.exact_start, rule.exact_end, rule.domain_anchor);
        }
        LUNA_LOG("{}", "=== Loaded Network Exception Rules ===");
        for (const auto& rule : adblocker.network_exception_rules) {
            LUNA_LOG("  [Exception] pattern='{}' exact_start={} exact_end={} domain_anchor={}", rule.pattern, rule.exact_start, rule.exact_end, rule.domain_anchor);
        }
        LUNA_LOG("{}", "=== Loaded Content Rules ===");
        for (const auto& rule : adblocker.content_rules) {
            LUNA_LOG("  [Content] pattern='{}' selector='{}'", rule.pattern, rule.selector);
        }
#endif
    }

    LunaBrowserProfile profile(path, profile_name, adblocker, opts.disable_adblocker);
    LunaBrowser browser(opts, profile, adblocker);
    app.installEventFilter(&browser);
    browser.prepare();
    browser.show();

    if(!browser.load_session_from_file(opts.session_name)) {
        browser.show_error_notification();
    }

    const int ret = app.exec();

    // Destroy all tab views before destroying the profile,
    // so no QWebEnginePages reference the profile when it's deleted.
    for (int i = browser.tabs->count() - 1; i >= 0; --i) {
        auto *w = browser.tabs->widget(i);
        browser.tabs->removeTab(i);
        delete w;
    }
    browser.tabs_count = 0;

    // Destroy the web engine profile (and its interceptors) before adblocker goes out of scope
    profile.destroy();

#ifdef LUNA_DEBUG_BUILD
    LUNA_LOG("The total allocated memory over the browser runtime: {} bytes", allocation_metrics.total_allocated);
    LUNA_LOG("The total freed memory over the browser runtime: {} bytes", allocation_metrics.total_freed);
    LUNA_LOG("Current heap useage: {} bytes", allocation_metrics.current_usage());
#endif // LUNA_DEBUG_BUILD

    adblocker.flush();
    return ret;
}
#endif // LUNA_TESTING
