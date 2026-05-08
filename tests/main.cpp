#define LUNA_TESTING
#include "../src/main.cpp"
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <thread>

int test_easylist_samples();
int test_str2u64();
int test_cache();
int test_get_hiding_rules_for_domain();
int test_load_all_filter_lists();
int test_block_request_combinations();
int test_youtube_ads();
#if __has_include("real_network_ads.h")
#include "real_network_ads.h"
int test_real_network_ads();
#endif

#define LUNA_TEST_ASSERT(cond) \
    do { \
        const bool __cond_eval_result = (cond); \
        if (!__cond_eval_result) { \
            fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            luna_failed_tests += 1; \
        } else { \
            printf("[PASS] %s\n", #cond); \
        } \
    } while (0)

int run_tests() {
    int failed_tests = 0;
    // Test 1: str2u64 basic conversions
    failed_tests += test_str2u64();

    // Test 2: Timestamp parsing (validates getline fix and parsing logic)
    printf("\nTesting timestamp parsing...\n");
    const char* test_db = "/tmp/luna_test_ts_db.txt";
    {
        std::ofstream db(test_db);
        db << "list1:1700000000\n";
        db << "list2:1700000001\n";
        db << "invalid_line\n";  // No colon, should be skipped
        db << "list3:\n";        // No value, should be skipped
    }

    printf("\nTesting real easylist samples...\n");
    failed_tests += test_easylist_samples();

    printf("\nTesting cache functionality...\n");
    failed_tests += test_cache();

    printf("\nTesting get hiding rules for domain...\n");
    failed_tests += test_get_hiding_rules_for_domain();

    printf("\nTesting all filter lists combined...\n");
    failed_tests += test_load_all_filter_lists();

    printf("\nTesting block_request argument combinations...\n");
    failed_tests += test_block_request_combinations();

    // printf("\nTesting YouTube ad blocking...\n");
    // failed_tests += test_youtube_ads();

#if __has_include("real_network_ads.h")
    printf("\nTesting real network ads...\n");
    failed_tests += test_real_network_ads();
#endif

    return failed_tests;
}

int test_easylist_samples() {
    int luna_failed_tests = 0;
    LunaAdBlocker ab(std::filesystem::path("not_used"));
    ab.parse_list_file("tests/easylist.txt");

    LUNA_TEST_ASSERT(ab.block_request("https://thatsillyman.win") == false);
    LUNA_TEST_ASSERT(ab.block_request("https://googleads.g.doubleclick.net/pagead/id") == true);

    return luna_failed_tests;
}

int test_load_all_filter_lists() {
    int luna_failed_tests = 0;

    // Test 1: Per-file filter counts
    printf("\n--- Per-file filter counts ---\n");
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

    // Test 2: Combined filter counts across all lists
    printf("\n--- Combined filter counts ---\n");
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

    // Test 3: Normal page URLs should NOT be blocked
    printf("\n--- Page URLs (should not block) ---\n");
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

    // Test 4: Known ad/tracker URLs SHOULD be blocked
    printf("\n--- Ad/tracker URLs (should block) ---\n");
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

    // Test 5: Content hiding rules from combined lists
    printf("\n--- Content hiding rules ---\n");
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

int test_block_request_combinations() {
    int luna_failed_tests = 0;
    const char* lists[] = {
        "tests/easylist.txt", "tests/easyprivacy.txt", "tests/uboFilters.txt",
        "tests/yt-shorts.txt", "tests/unbreak.txt", "tests/quick-fixes.txt"
    };

    // Test 1: luna: internal URLs are hardcoded to never block
    printf("\n--- luna: internal URLs (hardcoded exemption) ---\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used1"));
        for (auto f : lists) ab.parse_list_file(f);

        LUNA_TEST_ASSERT(ab.block_request("luna://settings") == false);
        LUNA_TEST_ASSERT(ab.block_request("luna://settings", 1 << 0) == false);
        LUNA_TEST_ASSERT(ab.block_request("luna://settings", 0, "example.com") == false);
        LUNA_TEST_ASSERT(ab.block_request("luna:newtab") == false);
        LUNA_TEST_ASSERT(ab.block_request("luna:newtab", 1 << 1, "youtube.com") == false);
    }

    // Test 2: resource_type filtering — same URL with different resource types
    printf("\n--- resource_type filtering ($image rule) ---\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used2"));
        for (auto f : lists) ab.parse_list_file(f);

        // /ad/image/*$image should only match when resource_type includes image
        LUNA_TEST_ASSERT(ab.block_request("https://example.com/ad/image/banner.png") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://example.com/ad/image/banner.png", 1 << 0) == false);
        LUNA_TEST_ASSERT(ab.block_request("https://example.com/ad/image/banner.png", 1 << 1) == true);
        LUNA_TEST_ASSERT(ab.block_request("https://example.com/ad/image/banner.png", 1 << 2) == false);
        LUNA_TEST_ASSERT(ab.block_request("https://example.com/ad/image/banner.png", 1 << 5) == false);
    }

    // Test 3: document_domain filtering — same URL, different document domains
    printf("\n--- document_domain (third-party) filtering ---\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used3"));
        for (auto f : lists) ab.parse_list_file(f);

        // thatsillyman.win is not an ad domain — not blocked as page load
        LUNA_TEST_ASSERT(ab.block_request("https://thatsillyman.win") == false);
        LUNA_TEST_ASSERT(ab.block_request("https://thatsillyman.win", 0, "thatsillyman.win") == false);
    }

    // Test 4: Combined resource_type + document_domain
    printf("\n--- Combined resource_type + document_domain ---\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used4"));
        for (auto f : lists) ab.parse_list_file(f);

        // ||bit.ly^$script,domain=dailyuploads.net|freeshot.live
        // nothing set → not blocked
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123") == false);
        // script + matching domain → blocked
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 0, "dailyuploads.net") == true);
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 0, "freeshot.live") == true);
        // script but no domain → not blocked (domain missing)
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 0) == false);
        // script + non-matching domain → not blocked (wrong domain)
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 0, "google.com") == false);
        // stylesheet + matching domain → not blocked (wrong resource type)
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 2, "dailyuploads.net") == false);
        // image + matching domain → not blocked (wrong resource type)
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 1 << 1, "dailyuploads.net") == false);
        // no resource type + matching domain → not blocked (no type specified)
        LUNA_TEST_ASSERT(ab.block_request("https://bit.ly/abc123", 0, "dailyuploads.net") == false);
    }

    // Test 5: Page URLs with various argument combos
    printf("\n--- Page URLs with various argument combos ---\n");
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

    // Test 6: Known ad URLs blocked with all argument combinations
    printf("\n--- Ad URLs with all argument combos ---\n");
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

int main() {
    printf("=== Luna AdBlocker Test Suite ===\n");
    int ret = run_tests();
    printf(ret == 0 ? "\nAll tests passed!\n" : "\nSome tests failed!\n");
    return ret;
}

int test_str2u64() {
    int luna_failed_tests = 0;
    printf("Testing str2u64...\n");
    LUNA_TEST_ASSERT(str2u64("0") == 0);
    LUNA_TEST_ASSERT(str2u64("12345") == 12345);
    LUNA_TEST_ASSERT(str2u64("18446744073709551615") == UINT64_MAX);
    return luna_failed_tests;
}

int test_cache() {
    int luna_failed_tests = 0;
    LunaAdBlocker ab(std::filesystem::path("not_used"));
    ab.parse_list_file("tests/easylist.txt");
    ab.clear_cache();

    // Test 1: Cache miss on first request
    printf("Test 1: Cache miss on first request...\n");
    LUNA_TEST_ASSERT(ab.cache_size() == 0);
    bool result1 = ab.block_request("https://googleads.g.doubleclick.net/pagead/id");
    LUNA_TEST_ASSERT(result1 == true);
    LUNA_TEST_ASSERT(ab.cache_size() == 1);

    // Test 2: Cache hit on second request
    printf("Test 2: Cache hit on second request...\n");
    bool result2 = ab.block_request("https://googleads.g.doubleclick.net/pagead/id");
    LUNA_TEST_ASSERT(result2 == true);
    LUNA_TEST_ASSERT(ab.cache_size() == 1);  // Still 1, not 2

    // Test 3: Different URL should miss cache
    printf("Test 3: Different URL should miss cache...\n");
    bool result3 = ab.block_request("https://thatsillyman.win");
    LUNA_TEST_ASSERT(result3 == false);
    LUNA_TEST_ASSERT(ab.cache_size() == 2);

    // Test 4: Test cache with resource type and document domain
    printf("Test 4: Cache with resource type and document domain...\n");
    bool result4 = ab.block_request("https://googleads.g.doubleclick.net/pagead/id", 1 << 0, "example.com");
    LUNA_TEST_ASSERT(result4 == true);
    LUNA_TEST_ASSERT(ab.cache_size() == 3);  // Different key due to different params

    // Test 5: Flush should clear cache and stop thread
    printf("Test 5: Flush clears cache and stops thread...\n");
    ab.flush();
    LUNA_TEST_ASSERT(ab.cache_size() == 0);

    return luna_failed_tests;
}

#if __has_include("real_network_ads.h")
int test_real_network_ads() {
    int luna_failed_tests = 0;
    const char* lists[] = {
        "tests/easylist.txt", "tests/easyprivacy.txt", "tests/uboFilters.txt",
        "tests/yt-shorts.txt", "tests/unbreak.txt", "tests/quick-fixes.txt"
    };

    LunaAdBlocker ab(std::filesystem::path("not_used_real"));
    for (auto f : lists) ab.parse_list_file(f);

    for (int i = 0; REAL_AD_LINKS[i] != nullptr; i++) {
        LUNA_TEST_ASSERT(ab.block_request(REAL_AD_LINKS[i]) == true);
    }

    return luna_failed_tests;
}
#endif

int test_get_hiding_rules_for_domain() {
    int luna_failed_tests = 0;

    // Test 1: No content rules returns empty array
    printf("Test 1: No content rules returns empty array...\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used1"));
        std::string r = ab.get_hiding_rules_for_domain("example.com");
        LUNA_TEST_ASSERT(r == "[]");
    }

    // Test 2: Global rules apply to any domain
    printf("Test 2: Global rules apply to any domain...\n");
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

    // Test 3: Domain-specific rules only for matching domain
    printf("Test 3: Domain-specific rules only for matching domain...\n");
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

    // Test 4: Subdomain matches parent domain rule
    printf("Test 4: Subdomain matches parent domain rule...\n");
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

    // Test 5: Exclude domains work correctly
    printf("Test 5: Exclude domains work correctly...\n");
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

    // Test 6: Empty selector is skipped
    printf("Test 6: Empty selector is skipped...\n");
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

    // Test 7: JSON format is valid
    printf("Test 7: JSON format is valid...\n");
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

    // Test 8: From easylist file — global rules present
    printf("Test 8: From easylist file — global rules present...\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used8"));
        ab.parse_list_file("tests/easylist.txt");
        std::string r = ab.get_hiding_rules_for_domain("example.com");
        LUNA_TEST_ASSERT(!r.empty());
        LUNA_TEST_ASSERT(r[0] == '[');
        LUNA_TEST_ASSERT(r.back() == ']');
    }

    // Test 9: easylist — youtube.com gets youtube-specific rules, NOT other domains' rules
    printf("Test 9: easylist — youtube domain filtering...\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used9"));
        ab.parse_list_file("tests/easylist.txt");

        std::string r_youtube = ab.get_hiding_rules_for_domain("youtube.com");
        LUNA_TEST_ASSERT(!r_youtube.empty());
        LUNA_TEST_ASSERT(r_youtube.find("#shopping-timely-shelf") != std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("#sticker-layer") != std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("ytd-rich-item-renderer:has(> #content > ytd-ad-slot-renderer)") != std::string::npos);

        // Must NOT include reddit-specific rules
        LUNA_TEST_ASSERT(r_youtube.find("shreddit-ad-post") == std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("reddit.com") == std::string::npos);

        // Must NOT include multi-domain rules for other sites like moviekhhd.biz
        LUNA_TEST_ASSERT(r_youtube.find("moviekhhd") == std::string::npos);

        // Result should not contain other domains' specific selectors
        LUNA_TEST_ASSERT(r_youtube.find("reddit") == std::string::npos);
        LUNA_TEST_ASSERT(r_youtube.find("moviekhhd") == std::string::npos);
    }

    // Test 10: easylist — reddit.com gets reddit rules, NOT youtube rules
    printf("Test 10: easylist — reddit domain filtering...\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used10"));
        ab.parse_list_file("tests/easylist.txt");

        std::string r_reddit = ab.get_hiding_rules_for_domain("reddit.com");
        LUNA_TEST_ASSERT(!r_reddit.empty());
        LUNA_TEST_ASSERT(r_reddit.find("shreddit-ad-post") != std::string::npos);
        LUNA_TEST_ASSERT(r_reddit.find("#shopping-timely-shelf") == std::string::npos);
        LUNA_TEST_ASSERT(r_reddit.find("#sticker-layer") == std::string::npos);
        // Check selectors with embedded quotes (escaped in JSON output)
        LUNA_TEST_ASSERT(r_reddit.find("data-before-content") != std::string::npos);
        LUNA_TEST_ASSERT(r_reddit.find("data-faceplate-tracking-context") != std::string::npos);
    }

    // Test 11: Multi-domain content rule parsing (comma-separated domains)
    printf("Test 11: Multi-domain content rule parsing...\n");
    {
        LunaAdBlocker ab(std::filesystem::path("not_used11"));
        // Simulate parsing a multi-domain content rule
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

        // Should NOT include it for other domains
        std::string r3 = ab.get_hiding_rules_for_domain("other.com");
        LUNA_TEST_ASSERT(r3 == "[]");
    }

    // Test 12: Parse real multi-domain content rule from easylist
    printf("Test 12: Real multi-domain content rule from easylist...\n");
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
