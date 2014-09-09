#include <gtest/gtest.h>
#include "IntParser.h"

TEST(IntParserTest, create)
{
    IntParser p;
    ASSERT_FALSE(p.overflowed());
    ASSERT_FALSE(p.invalid());
}

TEST(IntParserTest, one_digit)
{
    IntParser p;
    p.next('1');

    uintmax_t x;
    ASSERT_TRUE(p.value(&x));
    ASSERT_EQ(1, x);
}

TEST(IntParserTest, two_digits)
{
    IntParser p;
    p.next('1');
    p.next('2');

    uintmax_t x;
    ASSERT_TRUE(p.value(&x));
    ASSERT_EQ(12, x);
}

TEST(IntParserTest, max)
{
    IntParser p;

    for (char x : std::to_string(UINTMAX_MAX)) {
        p.next(x);
    }

    uintmax_t v;
    ASSERT_TRUE(p.value(&v));
    ASSERT_EQ(UINTMAX_MAX, v);
}

TEST(IntParserTest, overflow)
{
    IntParser p;

    p.next('1');
    for (char x : std::to_string(UINTMAX_MAX)) {
        p.next(x);
    }

    uintmax_t v;
    ASSERT_FALSE(p.value(&v));
    ASSERT_TRUE(p.overflowed());
}

TEST(IntParserTest, invalid)
{
    IntParser p;
    p.next('a');

    uintmax_t x;
    ASSERT_FALSE(p.value(&x));
    ASSERT_TRUE(p.invalid());
}
