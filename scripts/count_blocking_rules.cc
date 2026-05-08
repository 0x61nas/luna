#define LUNA_TESTING
#include "../src/main.cc"
#include <cstdio>

int main() {
    // easylist.txt
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/easylist.txt");
        printf("easylist: network=%zu exceptions=%zu content=%zu\n",
            ab.network_rules.size(), ab.network_exception_rules.size(), ab.content_rules.size());
    }
    // easyprivacy.txt
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/easyprivacy.txt");
        printf("easyprivacy: network=%zu exceptions=%zu content=%zu\n",
            ab.network_rules.size(), ab.network_exception_rules.size(), ab.content_rules.size());
    }
    // uboFilters.txt
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/uboFilters.txt");
        printf("uboFilters: network=%zu exceptions=%zu content=%zu\n",
            ab.network_rules.size(), ab.network_exception_rules.size(), ab.content_rules.size());
    }
    // yt-shorts.txt
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/yt-shorts.txt");
        printf("yt-shorts: network=%zu exceptions=%zu content=%zu\n",
            ab.network_rules.size(), ab.network_exception_rules.size(), ab.content_rules.size());
    }
    // unbreak.txt
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/unbreak.txt");
        printf("unbreak: network=%zu exceptions=%zu content=%zu\n",
            ab.network_rules.size(), ab.network_exception_rules.size(), ab.content_rules.size());
    }
    // quick-fixes.txt
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/quick-fixes.txt");
        printf("quick-fixes: network=%zu exceptions=%zu content=%zu\n",
            ab.network_rules.size(), ab.network_exception_rules.size(), ab.content_rules.size());
    }
    // custom_ads.txt
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/custom_ads.txt");
        printf("quick-fixes: network=%zu exceptions=%zu content=%zu\n",
            ab.network_rules.size(), ab.network_exception_rules.size(), ab.content_rules.size());
    }
    // Combined
    {
        LunaAdBlocker ab(std::filesystem::path("not_used"));
        ab.parse_list_file("tests/easylist.txt");
        ab.parse_list_file("tests/easyprivacy.txt");
        ab.parse_list_file("tests/uboFilters.txt");
        ab.parse_list_file("tests/yt-shorts.txt");
        ab.parse_list_file("tests/unbreak.txt");
        ab.parse_list_file("tests/quick-fixes.txt");
        ab.parse_list_file("tests/custom_ads.txt");
        printf("\ncombined: network=%zu exceptions=%zu content=%zu\n",
            ab.network_rules.size(), ab.network_exception_rules.size(), ab.content_rules.size());
    }
    return 0;
}
