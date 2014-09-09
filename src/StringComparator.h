#ifndef STRINGCOMPARATOR_H
#define STRINGCOMPARATOR_H

#include <stdlib.h>
#include <stdint.h>
#include <WString.h>

class StringComparator;

class StringComparison
{
    friend class StringComparator;
private:
    const StringComparator* _parent;
    bool* _invalid;
    size_t _count = 0;

    explicit StringComparison(const StringComparator* parent);

public:
    StringComparison() = default;
    StringComparison(const StringComparison&) = delete; // Copy constructor
    StringComparison(StringComparison&&);

    StringComparison& operator=(const StringComparison&) = delete; // Assignment
    StringComparison& operator=(StringComparison&&);

    void reset();
    void next(uint8_t c);
    bool hasMatch(size_t& idx) const;

    ~StringComparison();
};

class StringComparator
{
    friend class StringComparison;
private:
    const char** _strings;
    const size_t _n_strings;

    uint8_t charAt(size_t idx, size_t pos) const;

public:
    StringComparator(const char** strings, size_t n);

    StringComparison create();
};

#endif /* STRINGCOMPARATOR_H */
