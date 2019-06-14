
// Prerequisites
#include "ForwardDeclarations.h"
#include "GlobalEnvironment.h"
class GlobalEnvironment* gEnv; // global var
#include "../fakes_impl.inl"

#include <gtest/gtest.h>
#include <string>

namespace {

TEST(UtilsTest, Func_SanitizeUtf8_ReplaceXff)
{
    const char * input = "\xFF"; // Character 255 (only range 0-127 is valid for single-octet characters)
    const char * replacer = u8"\xFFFD"; // Unicode Character 'REPLACEMENT CHARACTER' (U+FFFD)

    const std::string res = RoR::Utils::SanitizeUtf8String(input);
    ASSERT_STREQ(res.c_str(), replacer);
}

}  // namespace
