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
