// Based on https://github.com/google/googletest/blob/master/googletest/docs/primer.md

#include "Utils.h"
#include "gtest/gtest.h"

namespace {

// Tests that the Foo::Bar() method does Abc.
TEST(UtilsTest, FuncSanitizeUtf8) {
    const std::string res = RoR::Utils::SanitizeUtf8String("\xFF"); // Character 255 (only range 0-127 is valid for single-octet characters)
    ASSERT_STREQ(res.c_str(), u8"\xFFFD"); // Unicode Character 'REPLACEMENT CHARACTER' (U+FFFD)
}

}  // namespace
