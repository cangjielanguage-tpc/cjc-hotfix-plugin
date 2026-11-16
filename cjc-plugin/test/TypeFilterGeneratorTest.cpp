#include "../src/TypeFilterGenerator.h"
#include "gtest/gtest.h"

TEST(matches, simple)
{
    ASSERT_FALSE(HotfixPlugin::matches("pkg1", {}));

    ASSERT_TRUE(HotfixPlugin::matches("pkg1", {"pkg1"}));

    ASSERT_TRUE(HotfixPlugin::matches("pkg1.Class1", {"pkg1.Class1"}));

    ASSERT_TRUE(HotfixPlugin::matches("pkg2.pkg3.Func1", {"pkg2.*.Func1"}));
    ASSERT_TRUE(HotfixPlugin::matches("pkg2.pkg4.Func1", {"pkg2.*.Func1"}));
    ASSERT_FALSE(HotfixPlugin::matches("pkg2.pkg4.Func11", {"pkg2.*.Func1"}));
    ASSERT_FALSE(HotfixPlugin::matches("ppkg2.pkg4.Func1", {"pkg2.*.Func1"}));
    ASSERT_FALSE(HotfixPlugin::matches("pkg2.pkg5.pkg6.Func1", {"pkg2.*.Func1"}));
    ASSERT_FALSE(HotfixPlugin::matches("pkg2.Func1", {"pkg2.*.Func1"}));

    ASSERT_TRUE(HotfixPlugin::matches("pkg3.pkg4.Class3", {"pkg3.**.Class3"}));
    ASSERT_TRUE(HotfixPlugin::matches("pkg3.pkg4.pkg5.Class3", {"pkg3.**.Class3"}));
    ASSERT_FALSE(HotfixPlugin::matches("pkg3.Class3", {"pkg3.**.Class3"}));
}