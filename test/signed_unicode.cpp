// Regression coverage for signed Unicode comparisons against zero.
// Build with assertions enabled, for example:
// g++ -std=c++17 -Iinclude src/*.cpp test/signed_unicode.cpp -o signed_unicode_test

#include "suite_utf.h"
#include <cassert>
#include <initializer_list>
#include <limits>

namespace utf = unicode::utf;
namespace toolkit = unicode::utf::toolkit;
using unicode::unicode_t;
using bits = toolkit::cp_errors::bits;

template <typename Encoder>
void check_encoder(Encoder encode)
{
    for (const unicode_t value : {std::numeric_limits<unicode_t>::min(), -2, -1, 0, 1, 127})
    {
        uint8_t buffer[8] = {0xa5};
        utf::utf_text text{sizeof(buffer), 0, buffer};
        uint32_t bytes = 0;
        const auto errors = encode(text, value, bytes);
        if (value < 0)
        {
            assert(errors.failed());
            assert(errors.any(bits::InvalidPoint));
            assert(errors.any(bits::NotEncodable));
            assert(errors.none(bits::DelimitString));
            assert(bytes == 0 && buffer[0] == 0xa5);
        }
        else
        {
            assert(errors.no_error() && bytes > 0);
            assert(errors.none(bits::InvalidPoint));
            assert(errors.any(bits::DelimitString) == (value == 0));
        }
    }
}

int main()
{
    // Negative sentinel values must never become short, encodable code points
    // when an upper-bound comparison changes from unsigned to signed.
    for (const unicode_t value : {std::numeric_limits<unicode_t>::min(), -65536, -256, -2, -1})
    {
        assert(utf::std::lenBYTE(value) == 0);
        assert(utf::std::lenUTF8(value) == 0);
        assert(utf::std::lenUTF16(value) == 0);
        assert(utf::std::lenUTF32(value) == 0);
        assert(toolkit::lenUTF8(value) == 0);
        assert(toolkit::lenUTF16(value) == 0);
        assert(toolkit::lenUTF32(value) == 0);
        uint8_t buffer[4] = {0xa5};
        uint32_t bytes = 0;
        assert(!utf::std::setUTF8(buffer, sizeof(buffer), value, bytes));
        assert(bytes == 0 && buffer[0] == 0xa5);
        assert(!unicode::isNonCharacter(value));
        assert(!unicode::isCombining(value));
        assert(!unicode::isPrivateUse(value));
        assert(!unicode::isBreakingWhite(value));
        assert(!unicode::isAsciiText(value));
        assert(!unicode::isAsciiWhite(value));
        assert(!unicode::isAsciiBlack(value));
        assert(!unicode::isStrictAsciiText(value));
        assert(!unicode::isNameStartXML(value));
        assert(!unicode::isNameExtraXML(value));
        assert(!unicode::isNameXML(value));
    }

    // Exercise both sides of UTF-8 length boundaries and the scalar range.
    for (const unicode_t value : {0x00000000, 0x0000007f, 0x00000080, 0x000007ff,
                                  0x00000800, 0x0000d7ff, 0x0000e000, 0x0000ffff,
                                  0x00010000, 0x0010ffff})
    {
        uint8_t buffer[4] = {};
        uint32_t written = 0;
        assert(utf::std::setUTF8(buffer, sizeof(buffer), value, written));
        assert(written == utf::std::lenUTF8(value));
        unicode_t decoded = -1;
        uint32_t read = 0;
        assert(utf::std::getUTF8(buffer, written, decoded, read));
        assert(decoded == value && read == written);
    }
    {
        const uint8_t invalid[] = {0x80u};
        unicode_t decoded = 0;
        uint32_t bytes = 0;
        assert(!utf::std::getUTF8(invalid, sizeof(invalid), decoded, bytes));
        assert(decoded < 0x00000000 && bytes == 1);
        assert(static_cast<uint32_t>(decoded) == 0x80000080u);
    }

    check_encoder([](utf::utf_text& text, unicode_t value, uint32_t& bytes) {
        return toolkit::encodeBYTE(text, value, bytes);
    });
    check_encoder([](utf::utf_text& text, unicode_t value, uint32_t& bytes) {
        return toolkit::encodeUTF8(text, value, bytes);
    });
    check_encoder([](utf::utf_text& text, unicode_t value, uint32_t& bytes) {
        return toolkit::encodeUTF16(text, value, bytes);
    });
    check_encoder([](utf::utf_text& text, unicode_t value, uint32_t& bytes) {
        return toolkit::encodeCP1252(text, value, bytes);
    });

    for (const unicode_t value : {std::numeric_limits<unicode_t>::min(), -2, -1, 0, 1, 127})
    {
        uint8_t buffer[8] = {0xa5};
        utf::utf_text text{sizeof(buffer), 0, buffer};
        for (uint32_t width = 1; width <= 6; ++width)
        {
            const auto errors = toolkit::encodeUTF8n(text, value, width);
            assert(errors.any(bits::InvalidPoint) == (value < 0));
            assert(errors.failed() == (value < 0));
            if (value < 0) assert(buffer[0] == 0xa5);
        }

        for (const bool little_endian : {false, true})
        {
            uint32_t bytes = 0;
            const auto encoded = toolkit::encodeUTF32(text, value, bytes, little_endian);
            assert(encoded.no_error() && bytes == 4);
            assert(encoded.any(bits::InvalidPoint) == (value < 0));
            assert(encoded.any(bits::DelimitString) == (value == 0));
            unicode_t decoded_value = 0;
            const auto decoded = toolkit::decodeUTF32(text, decoded_value, bytes, little_endian);
            assert(decoded.no_error() && bytes == 4 && decoded_value == value);
            assert(decoded.any(bits::InvalidPoint) == (value < 0));
            assert(decoded.any(bits::DelimitString) == (value == 0));
        }

        if (value < 0)
        {
            assert(!unicode::isAsciiCC(value));
            assert(!unicode::isCleanXML(value));
            assert(!unicode::isHexEscapedJSON(value));
            for (uint32_t width = 2; width <= 6; ++width)
            {
                uint32_t index = 123;
                assert(!toolkit::overlongToIndexUTF8(value, width, index));
                assert(index == 0);
            }
        }
    }
    assert(unicode::isAsciiCC(0));
    assert(unicode::isHexEscapedJSON(0));
    assert(!unicode::isCleanXML(0));
    assert(unicode::isCleanXML('A'));
}
