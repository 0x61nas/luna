#define LUNA_TESTING
#include "../src/main.cc"
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <chrono>

#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_RESET   "\033[0m"

struct {
    size_t total = 0;
    size_t failed = 0;
} luna_test_stats;

#define LUNA_TEST_ASSERT(cond) \
    do { \
        luna_test_stats.total++; \
        const bool __cond_eval_result = (cond); \
        if (!__cond_eval_result) { \
            fprintf(stderr, ANSI_RED "[  FAIL  ]" ANSI_RESET " %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            luna_failed_tests += 1; \
            luna_test_stats.failed++; \
        } else { \
            printf(ANSI_GREEN "[  PASS  ]" ANSI_RESET " %s\n", #cond); \
        } \
    } while (0)

struct TestCase {
    const char* name;
    size_t (*func)();
    const char* desc;
    bool skip_all;
};

size_t test_easylist_samples();
size_t test_str2u64();
size_t test_cache();
size_t test_get_hiding_rules_for_domain();
size_t test_load_all_filter_lists();
size_t test_block_request_combinations();
size_t test_youtube_ads();
#if __has_include("real_network_ads.h")
#include "real_network_ads.h"
size_t test_real_network_ads();
#endif

size_t test_str2u64() {
    size_t luna_failed_tests = 0;
    LUNA_TEST_ASSERT(str2u64("0") == 0);
    LUNA_TEST_ASSERT(str2u64("12345") == 12345);
    LUNA_TEST_ASSERT(str2u64("18446744073709551615") == UINT64_MAX);
    LUNA_TEST_ASSERT(str2u64("love") == THE_ZERO);
    return luna_failed_tests;
}

size_t test_easylist_samples() {
    size_t luna_failed_tests = 0;
    LunaAdBlocker ab(std::filesystem::path("not_used"));
    ab.parse_list_file("tests/easylist.txt");

    LUNA_TEST_ASSERT(ab.block_request("https://thatsillyman.win") == false);
    LUNA_TEST_ASSERT(ab.block_request("https://googleads.g.doubleclick.net/pagead/id") == true);

    return luna_failed_tests;
}

size_t test_cache() {
    size_t luna_failed_tests = 0;
    LunaAdBlocker ab(std::filesystem::path("not_used"));
    ab.parse_list_file("tests/easylist.txt");
    ab.clear_cache();

    printf("\n  " ANSI_YELLOW "▸ Test 1:" ANSI_RESET " Cache miss on first request\n");
    LUNA_TEST_ASSERT(ab.cache_size() == 0);
    bool result1 = ab.block_request("https://googleads.g.doubleclick.net/pagead/id");
    LUNA_TEST_ASSERT(result1 == true);
    LUNA_TEST_ASSERT(ab.cache_size() == 1);

    printf("\n  " ANSI_YELLOW "▸ Test 2:" ANSI_RESET " Cache hit on second request\n");
    bool result2 = ab.block_request("https://googleads.g.doubleclick.net/pagead/id");
    LUNA_TEST_ASSERT(result2 == true);
    LUNA_TEST_ASSERT(ab.cache_size() == 1);

    printf("\n  " ANSI_YELLOW "▸ Test 3:" ANSI_RESET " Different URL should miss cache\n");
    bool result3 = ab.block_request("https://thatsillyman.win");
    LUNA_TEST_ASSERT(result3 == false);
    LUNA_TEST_ASSERT(ab.cache_size() == 2);

    printf("\n  " ANSI_YELLOW "▸ Test 4:" ANSI_RESET " Cache with resource type and document domain\n");
    bool result4 = ab.block_request("https://googleads.g.doubleclick.net/pagead/id", 1 << 0, "example.com");
    LUNA_TEST_ASSERT(result4 == true);
    LUNA_TEST_ASSERT(ab.cache_size() == 3);

    printf("\n  " ANSI_YELLOW "▸ Test 5:" ANSI_RESET " Flush clears cache and stops thread\n");
    ab.flush();
    LUNA_TEST_ASSERT(ab.cache_size() == 0);

    return luna_failed_tests;
}

size_t test_get_hiding_rules_for_domain() {
    size_t luna_failed_tests = 0;

    printf("\n  " ANSI_YELLOW "▸ Test 1:" ANSI_RESET " No content rules returns empty array\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used1"));
        std::string r = ab.get_hiding_rules_for_domain("example.com");
        LUNA_TEST_ASSERT(r == "[]");
    }

    printf("\n  " ANSI_YELLOW "▸ Test 2:" ANSI_RESET " Global rules apply to any domain\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used2"));
        LunaAdBlockerRule rule;
        rule.type = LunaAdBlockerRuleType::ContentHideRule;
        rule.selector = ".ad";
        ab.content_rules.push_back(std::move(rule));
        std::string r1 = ab.get_hiding_rules_for_domain("example.com");
        LUNA_TEST_ASSERT(r1.find(".ad") != std::string::npos);
        std::string r2 = ab.get_hiding_rules_for_domain("other.com");
        LUNA_TEST_ASSERT(r2.find(".ad") != std::string::npos);
    }

    printf("\n  " ANSI_YELLOW "▸ Test 3:" ANSI_RESET " Domain-specific rules only for matching domain\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used3"));
        LunaAdBlockerRule rule;
        rule.type = LunaAdBlockerRuleType::ContentHideRule;
        rule.selector = "#ad-banner";
        rule.options.domains.push_back("example.com");
        ab.content_rules.push_back(std::move(rule));
        std::string r1 = ab.get_hiding_rules_for_domain("example.com");
        LUNA_TEST_ASSERT(r1.find("#ad-banner") != std::string::npos);
        std::string r2 = ab.get_hiding_rules_for_domain("other.com");
        LUNA_TEST_ASSERT(r2 == "[]");
    }

    printf("\n  " ANSI_YELLOW "▸ Test 4:" ANSI_RESET " Subdomain matches parent domain rule\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used4"));
        LunaAdBlockerRule rule;
        rule.type = LunaAdBlockerRuleType::ContentHideRule;
        rule.selector = ".subdomain-ad";
        rule.options.domains.push_back("example.com");
        ab.content_rules.push_back(std::move(rule));
        std::string r1 = ab.get_hiding_rules_for_domain("sub.example.com");
        LUNA_TEST_ASSERT(r1.find(".subdomain-ad") != std::string::npos);
        std::string r2 = ab.get_hiding_rules_for_domain("example.org");
        LUNA_TEST_ASSERT(r2 == "[]");
    }

    printf("\n  " ANSI_YELLOW "▸ Test 5:" ANSI_RESET " Exclude domains work correctly\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used5"));
        LunaAdBlockerRule rule;
        rule.type = LunaAdBlockerRuleType::ContentHideRule;
        rule.selector = "#excluded-ad";
        rule.options.domains.push_back("example.com");
        rule.options.exclude_domains.push_back("mail.example.com");
        ab.content_rules.push_back(std::move(rule));
        std::string r1 = ab.get_hiding_rules_for_domain("example.com");
        LUNA_TEST_ASSERT(r1.find("#excluded-ad") != std::string::npos);
        std::string r2 = ab.get_hiding_rules_for_domain("mail.example.com");
        LUNA_TEST_ASSERT(r2 == "[]");
    }

    printf("\n  " ANSI_YELLOW "▸ Test 6:" ANSI_RESET " Empty selector is skipped\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used6"));
        LunaAdBlockerRule rule1;
        rule1.type = LunaAdBlockerRuleType::ContentHideRule;
        rule1.selector = "";
        ab.content_rules.push_back(std::move(rule1));
        LunaAdBlockerRule rule2;
        rule2.type = LunaAdBlockerRuleType::ContentHideRule;
        rule2.selector = ".real-ad";
        ab.content_rules.push_back(std::move(rule2));
        std::string r = ab.get_hiding_rules_for_domain("example.com");
        LUNA_TEST_ASSERT(r.find(".real-ad") != std::string::npos);
        LUNA_TEST_ASSERT(r.find("\"selector\":\"\"") == std::string::npos);
    }

    printf("\n  " ANSI_YELLOW "▸ Test 7:" ANSI_RESET " JSON format is valid\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used7"));
        LunaAdBlockerRule rule1;
        rule1.type = LunaAdBlockerRuleType::ContentHideRule;
        rule1.selector = "#ad1";
        ab.content_rules.push_back(std::move(rule1));
        LunaAdBlockerRule rule2;
        rule2.type = LunaAdBlockerRuleType::ContentHideRule;
        rule2.selector = ".ad2";
        ab.content_rules.push_back(std::move(rule2));
        std::string r = ab.get_hiding_rules_for_domain("example.com");
        LUNA_TEST_ASSERT(r[0] == '[');
        LUNA_TEST_ASSERT(r.back() == ']');
        LUNA_TEST_ASSERT(r.find("{\"selector\":\"#ad1\"}") != std::string::npos);
        LUNA_TEST_ASSERT(r.find("{\"selector\":\".ad2\"}") != std::string::npos);
    }

    printf("\n  " ANSI_YELLOW "▸ Test 8:" ANSI_RESET " From easylist file - global rules present\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used8"));
        ab.parse_list_file("tests/easylist.txt");
        std::string r = ab.get_hiding_rules_for_domain("example.com");
        LUNA_TEST_ASSERT(!r.empty());
        LUNA_TEST_ASSERT(r[0] == '[');
        LUNA_TEST_ASSERT(r.back() == ']');
    }

    printf("\n  " ANSI_YELLOW "▸ Test 9:" ANSI_RESET " easylist - youtube domain filtering\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used9"));
        ab.parse_list_file("tests/easylist.txt");

        std::string r_youtube = ab.get_hiding_rules_for_domain("youtube.com");
        LUNA_TEST_ASSERT(!r_youtube.empty());
        LUNA_TEST_ASSERT(r_youtube.find("#shopping-timely-shelf") != std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("#sticker-layer") != std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("ytd-rich-item-renderer:has(> #content > ytd-ad-slot-renderer)") != std::string::npos);

        LUNA_TEST_ASSERT(r_youtube.find("shreddit-ad-post") == std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("reddit.com") == std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("moviekhhd") == std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("reddit") == std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("moviekhhd") == std::string::npos);
    }

    printf("\n  " ANSI_YELLOW "▸ Test 10:" ANSI_RESET " easylist - reddit domain filtering\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used10"));
        ab.parse_list_file("tests/easylist.txt");

        std::string r_reddit = ab.get_hiding_rules_for_domain("reddit.com");
        LUNA_TEST_ASSERT(!r_reddit.empty());
        LUNA_TEST_ASSERT(r_reddit.find("shreddit-ad-post") != std::string::npos);
        LUNA_TEST_ASSERT(r_reddit.find("#shopping-timely-shelf") == std::string::npos);
        LUNA_TEST_ASSERT(r_reddit.find("#sticker-layer") == std::string::npos);
        LUNA_TEST_ASSERT(r_reddit.find("data-before-content") != std::string::npos);
        LUNA_TEST_ASSERT(r_reddit.find("data-faceplate-tracking-context") != std::string::npos);
    }

    printf("\n  " ANSI_YELLOW "▸ Test 11:" ANSI_RESET " Multi-domain content rule parsing\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used11"));
        auto rule = LunaAdBlockerRule::parse("domain1.com,domain2.com##.multi-ad");
        LUNA_TEST_ASSERT(rule.type == LunaAdBlockerRuleType::ContentHideRule);
        LUNA_TEST_ASSERT(rule.selector == ".multi-ad");
        LUNA_TEST_ASSERT(rule.options.domains.size() == 2);
        LUNA_TEST_ASSERT(rule.options.domains[0] == "domain1.com");
        LUNA_TEST_ASSERT(rule.options.domains[1] == "domain2.com");

        ab.content_rules.push_back(std::move(rule));

        std::string r1 = ab.get_hiding_rules_for_domain("domain1.com");
        LUNA_TEST_ASSERT(r1.find(".multi-ad") != std::string::npos);

        std::string r2 = ab.get_hiding_rules_for_domain("domain2.com");
        LUNA_TEST_ASSERT(r2.find(".multi-ad") != std::string::npos);

        std::string r3 = ab.get_hiding_rules_for_domain("other.com");
        LUNA_TEST_ASSERT(r3 == "[]");
    }

    printf("\n  " ANSI_YELLOW "▸ Test 12:" ANSI_RESET " Real multi-domain content rule from easylist\n");
    {
        auto rule = LunaAdBlockerRule::parse("calculatorsoup.com,thetvdb.com###Bottom");
        LUNA_TEST_ASSERT(rule.type == LunaAdBlockerRuleType::ContentHideRule);
        LUNA_TEST_ASSERT(rule.selector == "#Bottom");
        LUNA_TEST_ASSERT(rule.options.domains.size() == 2);
        LUNA_TEST_ASSERT(rule.options.domains[0] == "calculatorsoup.com");
        LUNA_TEST_ASSERT(rule.options.domains[1] == "thetvdb.com");
    }

    return luna_failed_tests;
}

size_t test_load_all_filter_lists() {
    size_t luna_failed_tests = 0;

    printf("\n  " ANSI_YELLOW "▸ Per-file filter counts" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/easylist.txt");
        LUNA_TEST_ASSERT(ab.network_rules.size() == 63350);
        LUNA_TEST_ASSERT(ab.network_exception_rules.size() == 732);
        LUNA_TEST_ASSERT(ab.content_rules.size() == 23518);
    }
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/easyprivacy.txt");
        LUNA_TEST_ASSERT(ab.network_rules.size() == 54572);
        LUNA_TEST_ASSERT(ab.network_exception_rules.size() == 818);
        LUNA_TEST_ASSERT(ab.content_rules.size() == 33);
    }
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/uboFilters.txt");
        LUNA_TEST_ASSERT(ab.network_rules.size() == 1065);
        LUNA_TEST_ASSERT(ab.network_exception_rules.size() == 762);
        LUNA_TEST_ASSERT(ab.content_rules.size() == 4323);
    }
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/yt-shorts.txt");
        LUNA_TEST_ASSERT(ab.network_rules.size() == 0);
        LUNA_TEST_ASSERT(ab.network_exception_rules.size() == 0);
        LUNA_TEST_ASSERT(ab.content_rules.size() == 15);
    }
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/unbreak.txt");
        LUNA_TEST_ASSERT(ab.network_rules.size() == 767);
        LUNA_TEST_ASSERT(ab.network_exception_rules.size() == 1253);
        LUNA_TEST_ASSERT(ab.content_rules.size() == 456);
    }
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/quick-fixes.txt");
        LUNA_TEST_ASSERT(ab.network_rules.size() == 39);
        LUNA_TEST_ASSERT(ab.network_exception_rules.size() == 13);
        LUNA_TEST_ASSERT(ab.content_rules.size() == 169);
    }

    printf("\n  " ANSI_YELLOW "▸ Combined filter counts" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/easylist.txt");
        ab.parse_list_file("tests/easyprivacy.txt");
        ab.parse_list_file("tests/uboFilters.txt");
        ab.parse_list_file("tests/yt-shorts.txt");
        ab.parse_list_file("tests/unbreak.txt");
        ab.parse_list_file("tests/quick-fixes.txt");

        LUNA_TEST_ASSERT(ab.network_rules.size() == 119793);
        LUNA_TEST_ASSERT(ab.network_exception_rules.size() == 3578);
        LUNA_TEST_ASSERT(ab.content_rules.size() == 28514);
    }

    printf("\n  " ANSI_YELLOW "▸ Page URLs (should not block)" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/easylist.txt");
        ab.parse_list_file("tests/easyprivacy.txt");
        ab.parse_list_file("tests/uboFilters.txt");
        ab.parse_list_file("tests/yt-shorts.txt");
        ab.parse_list_file("tests/unbreak.txt");
        ab.parse_list_file("tests/quick-fixes.txt");

        LUNA_TEST_ASSERT(ab.block_request("https://youtu.be/SeMXa5lBGYc") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://thatsillyman.win") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://app.codecrafters.io/users/anas-elgarhy") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://www.google.com") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://github.com") == false);
    }

    printf("\n  " ANSI_YELLOW "▸ Ad/tracker URLs (should block)" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/easylist.txt");
        ab.parse_list_file("tests/easyprivacy.txt");
        ab.parse_list_file("tests/uboFilters.txt");
        ab.parse_list_file("tests/yt-shorts.txt");
        ab.parse_list_file("tests/unbreak.txt");
        ab.parse_list_file("tests/quick-fixes.txt");

        LUNA_TEST_ASSERT(ab.block_request("https://www.googletagmanager.com/gtag/js?id=G-T24VL5516K") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://pubads.g.doubleclick.net/gampad/adx") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://ad.doubleclick.net/ddm/track") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://www.google-analytics.com/analytics.js") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://connect.facebook.net/en_US/fbevents.js") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://securepubads.g.doubleclick.net/gampad/ads") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://googleads.g.doubleclick.net/pagead/id") == true);
    }

    printf("\n  " ANSI_YELLOW "▸ Content hiding rules" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/easylist.txt");
        ab.parse_list_file("tests/easyprivacy.txt");
        ab.parse_list_file("tests/uboFilters.txt");
        ab.parse_list_file("tests/yt-shorts.txt");
        ab.parse_list_file("tests/unbreak.txt");
        ab.parse_list_file("tests/quick-fixes.txt");

        std::string r_youtube = ab.get_hiding_rules_for_domain("youtube.com");
        LUNA_TEST_ASSERT(!r_youtube.empty());
        LUNA_TEST_ASSERT(r_youtube[0] == '[');
        LUNA_TEST_ASSERT(r_youtube.back() == ']');
        LUNA_TEST_ASSERT(r_youtube.find("ytd-rich-item-renderer") != std::string::npos);

        std::string r_reddit = ab.get_hiding_rules_for_domain("reddit.com");
        LUNA_TEST_ASSERT(!r_reddit.empty());
        LUNA_TEST_ASSERT(r_reddit[0] == '[');
        LUNA_TEST_ASSERT(r_reddit.back() == ']');
        LUNA_TEST_ASSERT(r_reddit.find("shreddit-ad-post") != std::string::npos);

        std::string r_unknown = ab.get_hiding_rules_for_domain("somerandomdomain12345.xyz");
        LUNA_TEST_ASSERT(!r_unknown.empty());
        LUNA_TEST_ASSERT(r_unknown[0] == '[');
        LUNA_TEST_ASSERT(r_unknown.back() == ']');
        LUNA_TEST_ASSERT(r_unknown.find("ytd-rich-item-renderer") == std::string::npos);
        LUNA_TEST_ASSERT(r_unknown.find("shreddit-ad-post") == std::string::npos);
    }

    return luna_failed_tests;
}

size_t test_block_request_combinations() {
    size_t luna_failed_tests = 0;
    const char* lists[] = {
        "tests/easylist.txt", "tests/easyprivacy.txt", "tests/uboFilters.txt",
        "tests/yt-shorts.txt", "tests/unbreak.txt", "tests/quick-fixes.txt"
    };

    printf("\n  " ANSI_YELLOW "▸ luna: internal URLs (hardcoded exemption)" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used1"));
        for (auto f : lists) ab.parse_list_file(f);

        LUNA_TEST_ASSERT(ab.block_request("luna://settings") == false);
        LUNA_TEST_ASSERT(ab.block_request("luna://settings", 1 << 0) == false);
        LUNA_TEST_ASSERT(ab.block_request("luna://settings", 0, "example.com") == false);
        LUNA_TEST_ASSERT(ab.block_request("luna:newtab") == false);
        LUNA_TEST_ASSERT(ab.block_request("luna:newtab", 1 << 1, "youtube.com") == false);
    }

    printf("\n  " ANSI_YELLOW "▸ resource_type filtering ($image rule)" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used2"));
        for (auto f : lists) ab.parse_list_file(f);

        LUNA_TEST_ASSERT(ab.block_request("https://example.com/ad/image/banner.png") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://example.com/ad/image/banner.png", 1 << 0) == false);
        LUNA_TEST_ASSERT(ab.block_request("https://example.com/ad/image/banner.png", 1 << 1) == true);
        LUNA_TEST_ASSERT(ab.block_request("https://example.com/ad/image/banner.png", 1 << 2) == false);
        LUNA_TEST_ASSERT(ab.block_request("https://example.com/ad/image/banner.png", 1 << 5) == false);
    }

    printf("\n  " ANSI_YELLOW "▸ document_domain (third-party) filtering" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used3"));
        for (auto f : lists) ab.parse_list_file(f);

        LUNA_TEST_ASSERT(ab.block_request("https://thatsillyman.win") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://thatsillyman.win", 0, "thatsillyman.win") == false);
    }

    printf("\n  " ANSI_YELLOW "▸ Combined resource_type + document_domain" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used4"));
        for (auto f : lists) ab.parse_list_file(f);

        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 0, "dailyuploads.net") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 0, "freeshot.live") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 0) == false);
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 0, "google.com") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 2, "dailyuploads.net") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 1, "dailyuploads.net") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 0, "dailyuploads.net") == false);
    }

    printf("\n  " ANSI_YELLOW "▸ Page URLs with various argument combos" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used5"));
        for (auto f : lists) ab.parse_list_file(f);

        LUNA_TEST_ASSERT(ab.block_request("https://youtu.be/SeMXa5lBGYc") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://youtu.be/SeMXa5lBGYc", 1 << 0) == false);
        LUNA_TEST_ASSERT(ab.block_request("https://youtu.be/SeMXa5lBGYc", 0, "youtube.com") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://youtu.be/SeMXa5lBGYc", 1 << 1, "reddit.com") == false);

        LUNA_TEST_ASSERT(ab.block_request("https://thatsillyman.win") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://thatsillyman.win", 1 << 0) == false);
        LUNA_TEST_ASSERT(ab.block_request("https://thatsillyman.win", 0, "thatsillyman.win") == false);

        LUNA_TEST_ASSERT(ab.block_request("https://app.codecrafters.io/users/anas-elgarhy") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://app.codecrafters.io/users/anas-elgarhy", 1 << 0) == false);
        LUNA_TEST_ASSERT(ab.block_request("https://app.codecrafters.io/users/anas-elgarhy", 0, "codecrafters.io") == false);

        LUNA_TEST_ASSERT(ab.block_request("https://github.com") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://github.com", 1 << 1) == false);
        LUNA_TEST_ASSERT(ab.block_request("https://github.com", 0, "example.com") == false);
    }

    printf("\n  " ANSI_YELLOW "▸ Ad URLs with all argument combos" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used6"));
        for (auto f : lists) ab.parse_list_file(f);

        const char* ad_urls[] = {
            "https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js",
            "https://www.googletagmanager.com/gtag/js?id=G-T24VL5516K",
            "https://www.google-analytics.com/analytics.js",
            "https://ad.doubleclick.net/ddm/track",
            "https://pubads.g.doubleclick.net/gampad/adx",
            "https://securepubads.g.doubleclick.net/gampad/ads",
            "https://googleads.g.doubleclick.net/pagead/id",
            "https://connect.facebook.net/en_US/fbevents.js",
        };

        for (const char* url : ad_urls) {
            LUNA_TEST_ASSERT(ab.block_request(url) == true);
        }
        for (const char* url : ad_urls) {
            LUNA_TEST_ASSERT(ab.block_request(url, 1 << 0, "example.com") == true);
        }
        for (const char* url : ad_urls) {
            LUNA_TEST_ASSERT(ab.block_request(url, 1 << 1) == true);
        }
        for (const char* url : ad_urls) {
            LUNA_TEST_ASSERT(ab.block_request(url, 0, url) == true);
        }
    }

    return luna_failed_tests;
}

size_t test_youtube_ads() {
    size_t luna_failed_tests = 0;

    const char* lists[] = {
        "tests/easylist.txt", "tests/easyprivacy.txt", "tests/uboFilters.txt",
        "tests/yt-shorts.txt", "tests/unbreak.txt", "tests/quick-fixes.txt"
    };

    printf("\n  " ANSI_YELLOW "▸ YouTube ad URLs (should block)" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used_yt1"));
        for (auto f : lists) ab.parse_list_file(f);

        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/pagead/ad") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/youtubei/v1/player/ad_break") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/api/stats/ads?event=ad") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/api/stats/qoe?page&ns=yt&fexp=v1&event=streamingstats") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/ptracking?html5=1&video_id=abc&cpn=def&ei=ghi&ptk=youtube_foo&pltype=content") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/get_video_info?video_id=xxx&adunit=yyy", 0, "www.youtube.com") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/get_midroll_", 0, "www.youtube.com") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://m.youtube.com/get_midroll_", 0, "www.youtube.com") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://googleads.g.doubleclick.net/pagead/ads") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://pubads.g.doubleclick.net/gampad/ads") == true);
    }

    printf("\n  " ANSI_YELLOW "▸ YouTube content URLs (should NOT block)" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used_yt2"));
        for (auto f : lists) ab.parse_list_file(f);

        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/watch?v=dQw4w9WgXcQ") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://youtu.be/dQw4w9WgXcQ") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/youtubei/v1/player?key=AIzaSyA") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/get_video_info?video_id=xxx") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://i.ytimg.com/vi/xxx/maxresdefault.jpg") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://yt3.ggpht.com/ytc/xxx") == false);
    }

    printf("\n  " ANSI_YELLOW "▸ YouTube ad URLs with resource type and document domain" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used_yt3"));
        for (auto f : lists) ab.parse_list_file(f);

        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/get_video_info?video_id=xxx&adunit=yyy") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/get_video_info?video_id=xxx&adunit=yyy", 0, "www.youtube.com") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/get_video_info?video_id=xxx&adunit=yyy", 1 << 0, "www.youtube.com") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/get_video_info?video_id=xxx&adunit=yyy", 0, "google.com") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/get_video_info?video_id=xxx") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/get_video_info?video_id=xxx", 0, "www.youtube.com") == false);
    }

    printf("\n  " ANSI_YELLOW "▸ googlevideo.com ad requests" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used_yt4"));
        for (auto f : lists) ab.parse_list_file(f);

        LUNA_TEST_ASSERT(ab.block_request("https://r1---sn-abc.googlevideo.com/initplayback?source=youtube&c=TVHTML5&oad=1", 1 << 5, "www.youtube.com") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://r2---sn-xyz.googlevideo.com/initplayback?source=youtube&c=TVHTML5&oad=1", 1 << 5, "www.youtube.com") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://r1---sn-abc.googlevideo.com/initplayback?source=youtube&c=TVHTML5&oad=1") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://r1---sn-abc.googlevideo.com/initplayback?source=youtube&c=TVHTML5", 1 << 5, "www.youtube.com") == false);
    }

    printf("\n  " ANSI_YELLOW "▸ YouTube exception rules" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used_yt5"));
        for (auto f : lists) ab.parse_list_file(f);

        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/get_video_info?video_id=xxx&adunit=yyy", 0, "www.youtube.com") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://www.youtube.com/get_video_info?video_id=xxx&adunit=yyy&timedtext_editor=1", 0, "www.youtube.com") == true);
    }

    printf("\n  " ANSI_YELLOW "▸ YouTube content hiding rules" ANSI_RESET "\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used_yt6"));
        for (auto f : lists) ab.parse_list_file(f);

        std::string r_youtube = ab.get_hiding_rules_for_domain("youtube.com");
        LUNA_TEST_ASSERT(!r_youtube.empty());
        LUNA_TEST_ASSERT(r_youtube[0] == '[');
        LUNA_TEST_ASSERT(r_youtube.back() == ']');
        LUNA_TEST_ASSERT(r_youtube.find("ytd-rich-item-renderer") != std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("ytd-ad-slot-renderer") != std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("#shopping-timely-shelf") != std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("#player-ads") != std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("#masthead-ad") != std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("shreddit-ad-post") == std::string::npos);

        std::string r_reddit = ab.get_hiding_rules_for_domain("reddit.com");
        LUNA_TEST_ASSERT(r_reddit.find("ytd-rich-item-renderer") == std::string::npos);

        std::string r_unknown = ab.get_hiding_rules_for_domain("somerandomdomain12345.xyz");
        LUNA_TEST_ASSERT(r_unknown.find("ytd-rich-item-renderer") == std::string::npos);
    }

    return luna_failed_tests;
}

#if __has_include("real_network_ads.h")
size_t test_real_network_ads() {
    size_t luna_failed_tests = 0;
    const char* lists[] = {
        "tests/easylist.txt", "tests/easyprivacy.txt", "tests/uboFilters.txt",
        "tests/yt-shorts.txt", "tests/unbreak.txt", "tests/quick-fixes.txt",
        "tests/custom_ads.txt"
    };

    LunaAdBlocker ab(std::filesystem::path("not_used_real"));
    for (auto f : lists) ab.parse_list_file(f);

    const char *current_cat_id = "";
    const char *current_svc = "";
    size_t cat_total = 0, cat_failed = 0;
    size_t svc_total = 0, svc_failed = 0;

    for (size_t i = 0; AD_TESTS[i].url != nullptr; i++) {
        const auto &e = AD_TESTS[i];

        if (strcmp(e.category_id, current_cat_id) != 0) {
            if (i > 0) printf("  category: %zu/%zu passed\n", cat_total - cat_failed, cat_total);
            current_cat_id = e.category_id;
            current_svc = "";
            cat_total = 0; cat_failed = 0;
            printf("\n" ANSI_MAGENTA ">>> [%s] %s" ANSI_RESET "\n", e.category_id, e.category_name);
        }

        if (strcmp(e.service, current_svc) != 0) {
            if (svc_total > 0) printf("    service: %zu/%zu passed\n", svc_total - svc_failed, svc_total);
            current_svc = e.service;
            svc_total = 0; svc_failed = 0;
            printf("\n  " ANSI_CYAN "--- %s ---" ANSI_RESET "\n", e.service);
        }

        bool blocked = ab.block_request(e.url);
        if (blocked) {
            printf("    " ANSI_GREEN "[PASS]" ANSI_RESET " %s\n", e.url);
        } else {
            printf("    " ANSI_RED "[FAIL]" ANSI_RESET " %s  " ANSI_RED "<-- NOT BLOCKED" ANSI_RESET "\n", e.url);
            luna_failed_tests++;
            cat_failed++;
            svc_failed++;
        }
        cat_total++;
        svc_total++;
    }

    if (svc_total > 0) printf("    service: %zu/%zu passed\n", svc_total - svc_failed, svc_total);
    if (cat_total > 0) printf("  category: %zu/%zu passed\n", cat_total - cat_failed, cat_total);

    return luna_failed_tests;
}
#endif

static const TestCase tests[] = {
    {"str2u64",              test_str2u64,              "str2u64 conversions",                    false},
    {"easylist_samples",     test_easylist_samples,     "Real easylist samples",                 false},
    {"cache",                test_cache,                "Cache functionality",                    false},
    {"hiding_rules",         test_get_hiding_rules_for_domain, "Content hiding rules for domains", false},
    {"load_all_lists",       test_load_all_filter_lists,"All filter list counts and blocking",    false},
    {"block_request_combos", test_block_request_combinations, "block_request argument combos",    false},
    {"youtube_ads",          test_youtube_ads,          "YouTube ad blocking",                    true },
#if __has_include("real_network_ads.h")
    {"real_network_ads",     test_real_network_ads,     "Real network ad URLs",                   true },
#endif
};

int main(int argc, char** argv) {
    auto start = std::chrono::steady_clock::now();

    printf(ANSI_BOLD "\n  luna test suite\n" ANSI_RESET);
    printf(ANSI_BOLD "  %s" ANSI_RESET "\n\n", "════════════════════════════════════════");

    const size_t num_tests = sizeof(tests) / sizeof(tests[0]);
    size_t total_failed = 0;
    size_t total_assertions = 0;
    size_t total_passed = 0;
    size_t tests_run = 0;

    auto run_test = [&](const TestCase& t) {
        luna_test_stats = {0, 0};
        size_t failed = t.func();
        size_t passed = luna_test_stats.total - luna_test_stats.failed;
        printf("\n  " ANSI_BOLD "%s" ANSI_RESET "\n", "────────────────────────────────────");
        if (failed == 0) {
            printf("  " ANSI_GREEN "✓ %s:" ANSI_RESET " %zu passed\n", t.desc, passed);
        } else {
            printf("  " ANSI_RED "✗ %s:" ANSI_RESET " %zu/%zu passed, %zu failed\n", t.desc, passed, luna_test_stats.total, failed);
        }
        total_failed += failed;
        total_assertions += luna_test_stats.total;
        total_passed += passed;
        tests_run++;
    };

    if (argc > 1) {
        if (strcmp(argv[1], "--list") == 0) {
            printf("  Available tests:\n");
            for (size_t i = 0; i < num_tests; i++) {
                printf("    " ANSI_CYAN "%s" ANSI_RESET "  - %s%s\n",
                       tests[i].name, tests[i].desc,
                       tests[i].skip_all ? " (opt-in)" : "");
            }
            return 0;
        }
        const char* filter = argv[1];
        bool found = false;
        for (size_t i = 0; i < num_tests; i++) {
            if (strcmp(tests[i].name, filter) == 0) {
                run_test(tests[i]);
                found = true;
                break;
            }
        }
        if (!found) {
            printf(ANSI_YELLOW "  Unknown test \"%s\". Available tests:" ANSI_RESET "\n", filter);
            for (size_t i = 0; i < num_tests; i++) {
                printf("    " ANSI_CYAN "%s" ANSI_RESET "  - %s\n", tests[i].name, tests[i].desc);
            }
            return 1;
        }
    } else {
        for (size_t i = 0; i < num_tests; i++) {
            if (!tests[i].skip_all) {
                run_test(tests[i]);
                printf("\n");
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    printf(ANSI_BOLD "  %s" ANSI_RESET "\n", "════════════════════════════════════════");
    if (total_failed == 0) {
        printf("  " ANSI_GREEN ANSI_BOLD "  ✓ ALL TESTS PASSED" ANSI_RESET "\n");
    } else {
        printf("  " ANSI_RED ANSI_BOLD "  ✗ %zu TEST(S) FAILED" ANSI_RESET "\n", total_failed);
    }
    printf("  %zu test(s), %zu assertion(s), %zu failed, %zu passed\n",
           tests_run, total_assertions, total_failed, total_passed);
    printf("  completed in %lldms\n" ANSI_RESET, (long long)ms);

    return (int)total_failed;
}
