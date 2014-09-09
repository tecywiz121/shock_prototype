#include "IntParser.h"

template<typename T>
static bool add_with_overflow_check(T a, T b, T* r)
{
	static_assert(static_cast<T>(0) - static_cast<T>(1) > static_cast<T>(0),
			"only valid with unsigned types");
	*r = a + b;
	return a <= *r && b <= *r;
}

template<typename T>
static bool mult_with_overflow_check(T, T, T*);

template<>
bool mult_with_overflow_check(uint64_t a, uint64_t b, uint64_t* r)
{
	// Put the smallest number in a
	if (a > b) {
		uintmax_t t = a;
		a = b;
		b = t;
	}

	if (a > UINT32_MAX) {
		return false;
	}

	uint32_t c = b >> 32;			// Top half of b
	uint32_t d = UINT32_MAX & b;	// Bottom half of b
	uint64_t u = a * c;				// Multiply a with most sig. bits of b
	uint64_t v = a * d;				// Multiply a with least sig. bits of b

	if (u > UINT32_MAX) {
		return false;				// The most sig. bits overflow, so fail
	}

	u <<= 32;

	return add_with_overflow_check(u, v, r);
}

void IntParser::reset()
{
	_overflowed = false;
	_invalid = false;
	_value = 0;
}

void IntParser::next(uint8_t c)
{
	if (_invalid || _overflowed) {
		return;
	}

	if (c < 48 || c > 57) {
		_invalid = true;
		return;
	}

	_overflowed = !mult_with_overflow_check(_value, static_cast<uintmax_t>(10u),
				&_value);

	if (_overflowed) {
		return;
	}

	uint8_t d = c - 48u;

	_overflowed = !add_with_overflow_check(_value, static_cast<uintmax_t>(d),
			&_value);
}
