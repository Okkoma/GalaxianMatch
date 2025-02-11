#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Basic tests", "[core]") {
    SECTION("Simple check") {
        REQUIRE(1 + 1 == 2);
    }
    
    SECTION("Check with booleans") {
        bool condition = true;
        REQUIRE(condition);
    }
}

int addition(int a, int b) {
    return a + b;
}

TEST_CASE("Test of the addition function", "[math]") {
    REQUIRE(addition(2, 2) == 4);
    REQUIRE(addition(0, 0) == 0);
    REQUIRE(addition(-1, 1) == 0);
}