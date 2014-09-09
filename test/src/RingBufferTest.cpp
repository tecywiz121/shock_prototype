#include "gtest/gtest.h"
#include "RingBuffer.h"
#include <cstdint>

using std::size_t;

class Readable
{
private:
    uint16_t _total = UINT16_MAX;
    uint16_t _current = 0;

public:
    Readable() = default;
    explicit Readable(uint16_t total) : _total(total) {}
    int read(void* buf, uint16_t count, uint32_t f = 0) {
        (void)f;
        uint8_t *b = static_cast<uint8_t*>(buf);

        uint16_t x = _total-_current;
        count = std::min(count, x);

        for (size_t ii = 0; ii < count; ii++) {
            b[ii] = _current++;
        }

        return count;
    }

    int read() {
        if (_current >= _total) {
            return -1;
        }

        return _current++;
    }

    uint16_t available() {
        return _total - _current;
    }

    bool connected() { return true; }

    void reset() {
        _current = 0;
    }
};

TEST(RingBufferTest, Capacity)
{
    RingBuffer<100> a;
    ASSERT_EQ(100, a.capacity());
}

TEST(RingBufferTest, AvailableEmpty)
{
    RingBuffer<100> a;
    ASSERT_EQ(0, a.available());
}

TEST(RingBufferTest, AvailableFull)
{
    RingBuffer<100> a;

    Readable r;
    a.readFrom(r);

    ASSERT_EQ(100, a.available());
}

TEST(RingBufferTest, AvailableSome)
{
    RingBuffer<100> a;
    Readable r(3);

    a.readFrom(r);

    ASSERT_EQ(3, a.available());
}

TEST(RingBufferTest, ReadIntoFull)
{
    RingBuffer<100> a;
    Readable r;

    a.readFrom(r);

    for (size_t ii = 0; ii < 100; ii++) {
        ASSERT_EQ(ii, a.read());
    }

    ASSERT_GT(0, a.read());
}

TEST(RingBufferTest, ReadIntoEmpty)
{
    RingBuffer<100> a;
    Readable r(0);

    a.readFrom(r);

    ASSERT_GT(0, a.read());
}

TEST(RingBufferTest, ReadIntoSome)
{
    RingBuffer<100> a;
    Readable r(34);

    a.readFrom(r);

    for (size_t ii = 0; ii < 34; ii++) {
        ASSERT_EQ(ii, a.read());
    }

    ASSERT_GT(0, a.read());
}

TEST(RingBufferTest, ReadIntoTwice)
{
    RingBuffer<100> a;
    Readable r(50);

    a.readFrom(r);
    r.reset();
    a.readFrom(r);

    for (int jj = 0; jj < 2; jj++) {
        for (size_t ii = 0; ii < 50; ii++) {
            ASSERT_EQ(ii, a.read());
        }
    }

    ASSERT_GT(0, a.read());
}

TEST(RingBufferTest, ReadIntoWriteReadWrite)
{
    RingBuffer<100> a;
    Readable r;

    a.readFrom(r);
    r.reset();

    for (size_t ii = 0; ii < 10; ii++) {
        ASSERT_EQ(ii, a.read());
    }

    ASSERT_EQ(90, a.available());

    a.readFrom(r);
    r.reset();

    ASSERT_EQ(100, a.available());

    for (size_t ii = 0; ii < 90; ii++) {
        ASSERT_EQ(ii+10, a.read());
    }

    ASSERT_EQ(10, a.available());

    for (size_t ii = 0; ii < 10; ii++) {
        ASSERT_EQ(ii, a.read());
    }

    ASSERT_GT(0, a.read());
}

TEST(RingBufferTest, ReadUntilFull)
{
    RingBuffer<100> a;
    Readable r;

    a.readFrom(r);

    uint8_t buffer[100];
    ASSERT_EQ(100, a.readUntil(buffer, 'c', 100));

    for (size_t ii = 0; ii < 100; ii++) {
        ASSERT_EQ(ii, buffer[ii]);
    }

    ASSERT_GT(0, a.read());
}

TEST(RingBufferTest, ReadUntilEmpty)
{
    RingBuffer<100> a;

    uint8_t buffer[100];
    ASSERT_EQ(0, a.readUntil(buffer, 'c', 100));
}

TEST(RingBufferTest, ReadUntilNoMatch)
{
    RingBuffer<100> a;
    Readable r(50);

    a.readFrom(r);

    uint8_t buffer[100];
    ASSERT_EQ(50, a.readUntil(buffer, 'c', 100));

    for (size_t ii = 0; ii < 50; ii++) {
        ASSERT_EQ(ii, buffer[ii]);
    }
}

TEST(RingBufferTest, PutBackEmpty)
{
    RingBuffer<100> a;
    a.putBack(77);
    ASSERT_EQ(1, a.available());
    ASSERT_EQ(77, a.read());
    ASSERT_GT(0, a.read());

    Readable r;
    a.readFrom(r);

    for (size_t ii = 0; ii < 100; ii++) {
        ASSERT_EQ(ii, a.read());
    }

    ASSERT_GT(0, a.read());
}

TEST(RingBufferTest, PutBackFull)
{
    RingBuffer<100> a;
    Readable r;
    a.readFrom(r);
    ASSERT_EQ(0, a.read());

    a.putBack(255);
    ASSERT_EQ(100, a.available());
    ASSERT_EQ(255, a.read());

    for (size_t ii = 0; ii < 99; ii++) {
        ASSERT_EQ(ii+1, a.read());
    }
}
