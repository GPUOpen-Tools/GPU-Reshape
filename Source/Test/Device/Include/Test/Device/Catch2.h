#pragma once

// Catch2
#include <catch2/catch.hpp>

/// Require with formatting, fx. REQUIRE_FORMAT(a == 2, "Expected 1 but got " << a)
#define REQUIRE_FORMAT(COND, ...) do { if (!(COND)) { WARN(__VA_ARGS__); REQUIRE(nullptr == "Require failed"); } } while((void)0, 0)
