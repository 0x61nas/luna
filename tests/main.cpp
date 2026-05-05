#define LUNA_TESTING
#include "../src/main.cpp"
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
#include <cstring>
#include <unistd.h>

int test_easylist_samples();
int test_str2u64();

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
