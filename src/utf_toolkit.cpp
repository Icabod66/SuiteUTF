
//  Copyright (c) 2010-2026 Ritchie Brannan
//  License: MIT (see LICENSE file in repository root)
//
//  SuiteUTF
//  Original design 2010-2016; maintained and extended 2024-2025.
//  Copyright (c) 2010-2025 Ritchie Brannan.
//  MIT License. See LICENSE.txt. Project history: docs/History.md.
//
//  File:   utf_toolkit.cpp
//  Author: Ritchie Brannan
//  Date:   11 July 16
//  
//  Description:
//  
//      Robust UTF code-point handling.
//  
//  Notes:
//  
//      Care should be always be taken to validate text buffers of unknown provenance before use.
//  
//      Decoder standards compliance:
//  
//          The UTF_SUB_TYPE_UTF8ns and UTF_SUB_TYPE_JUTF8ns types will pass Markus Kuhn's decoder stress test
//          with correct alignment.
//  
//          The UTF_SUB_TYPE_UTF8st and UTF_SUB_TYPE_JUTF8st types will pass Markus Kuhn's decoder stress test
//          using the malformed sequence as single bytes variation as specified in the introduction to the test.
//          These decoders produce results similar to the ATOM text editor and most web browsers.
//  
//          For all these types, a replacement character should be used when the decoder error return includes
//          any of the CP_USE_REPLACEMENT bits.
//  
//      Function control flags:
//  
//          use_ascii   : limits byte decoding to 7-bits.
//          use_ucs2    : disables UTF16 surrogate pairs and limits unicode to the BMP (U+0000 to U+FFFF).
//          use_ucs4    : enables UTF32 extended UCS4 range (U+00110000 to U+7FFFFFFF) as a standard encoding.
//          use_cesu    : enables use of surrogate pairs for supplemental plane code-points (U+10000 to U+10ffff).
//          use_java    : enables modified-UTF8 (Java-style 2-byte overlong NULL code-point) as a standard encoding.
//          coalesce    : enables UTF8 coalescing of sequences beginning with an illegal or invalid byte.
//          strict      : enables UTF8 strictness making decode fails and irregular forms 1 byte long.
//          le          : enables little-endian encoding and decoding.
//  
//      ASCII buffers:
//  
//          Despite being stored as bytes, ASCII strings are considered to be 7-bit. Byte values with the top
//          bit set are treated as illegal bytes.
//  
//          BYTE buffers can be used to read and write full byte values.
//  
//      Invalid and non-standard encodings:
//  
//          The encode and decode functions take a permissive approach to allow as many variations in encoding
//          as possible. A comprehensive set of flags are provided to allow the user to customise the behaviour.
//  
//          Examples of this permissiveness include: handling of overlong and extended UTF8 encodings; isolated
//          unicode surrogates are not treated as errors; the full UCS4 character range is allowed for UTF8 and
//          UTF32; 31-bit values can be read from and written to UTF8 and UTF32; and non-standard compatibility
//          formats are supported (JCESU8 and CESU32).
//  
//      Non-decodable encoding sequences:
//      
//          Sequences starting with an unexpected or illegal byte consider all bytes up to the next legal starting
//          byte to be a single invalid code-point.
//      
//          Sequences which encounter an unexpected or illegal byte after the starting byte treat all the bytes upto
//          the problem byte as a single invalid code-point.
//      
//          Non-decodable sequences obviously have implications for counting code-points in badly encoded buffers.
//  
//          If a decode function fails due to encountering an unexpected or illegal byte at any position in the encoding,
//          it will return the first byte of the sequence (which is not necessarily the byte which caused the failure) as
//          the unicode code-point value.
//  
//      Overlong UTF8:
//  
//          The cp_errors::bits::ModifiedUTF8 and cp_errors::bits::OverlongUTF8 flags are exclusive of each other, both flags must be checked
//          to test for all overlong encodings.

#include "utf_toolkit.h"
#include "unicode_utilities.h"

namespace unicode
{

namespace utf
{

namespace toolkit
{

namespace internal
{

/// internal low level UTF8 fetch function
/// 
///     This is a helper function for the decodeUTF8() function and only returns a subset of the error codes.
/// 
///     The coalesce flag controls the bytes count set when handling sequences with an illegal or invalid lead byte:
/// 
///         coalesce == false   :   bytes = 1
///         coalesce == true    :   bytes = offset to the next valid lead byte or buffer end (limit)
/// 
[[nodiscard]] cp_errors fetchUTF8(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes, const bool coalesce) noexcept
{
    cp_errors errors;
    bytes = 0;
    unicode = 0;
    if (size < 1)
    {
        errors = cp_errors::bits::ReadExhausted;
    }
    else
    {
        uint8_t byte = buffer[0];
        unicode = static_cast<unicode_t>(byte);
        if (static_cast<uint8_t>(byte + 2) <= 0xc1u)
        {   //  1 byte (7 bits: 0x00-0x7f) or unexpected continuation byte (0x80-0xbf) or illegal leading byte (0xfe-0xff)
            bytes = 1;
            if (byte > 0x7fu)
            {
                errors |= ((cp_errors::bits::Failed | cp_errors::bits::NotDecodable) | ((byte >= 0xfeu) ? cp_errors::bits::DisallowedByte : cp_errors::bits::UnexpectedByte));
                if (coalesce)
                {
                    uint32_t count = size;
                    for (uint32_t index = 1; index < count; ++index)
                    {
                        byte = buffer[index];
                        if (((byte & 0xc0u) != 0x80u) && (byte < 0xfeu))
                        {
                            count = index;
                            break;
                        }
                    }
                    bytes = count;
                }
            }
        }
        else
        {
            uint32_t count = 0;
            if (byte <= 0xefu)
            {   //  2 bytes (11 bits: 0xc0-0xdf) or 3 bytes (16 bits: 0xe0-0xef)
                count = static_cast<uint32_t>((byte >> 5) & 3);
            }
            else if (byte <= 0xf7u)
            {   //  4 bytes (21 bits: 0xf0-0xf7)
                count = 4;
            }
            else
            {   //  extended encoding: 5 bytes (26 bits: 0xf8-0xfb) or 6 bytes (31 bits: 0xfc-0xfd)
                count = static_cast<uint32_t>(((byte >> 2) & 7) - 1);
                errors |= cp_errors::bits::ExtendedUTF8;
            }
            if (count > size)
            {   //  truncated continuation
                count = size;
                errors |= (cp_errors::bits::Failed | cp_errors::bits::NotDecodable | cp_errors::bits::ReadTruncated);
            }
            unicode_t value = (static_cast<unicode_t>(byte) & ((unicode_t(1) << (7 - count)) - 1));
            for (uint32_t index = 1; index < count; ++index)
            {
                byte = buffer[index];
                if ((byte & 0xc0u) != 0x80u)
                {
                    errors.clear(cp_errors::bits::ReadTruncated);
                    errors |= ((cp_errors::bits::Failed | cp_errors::bits::NotDecodable) | ((byte >= 0xfeu) ? cp_errors::bits::DisallowedByte : cp_errors::bits::UnexpectedByte));
                    errors.set_byte_index(index);
                    count = index;
                    break;
                }
                value = ((value << 6) | (byte & 0x3fu));
            }
            bytes = count;
            if (errors.no_error())
            {
                unicode = value;
                if (count > 1)
                {   //  possible overlong
                    int32_t n = (static_cast<int32_t>(count | (count << 2)) - 11);
                    if ((value >> static_cast<uint32_t>((n & ((~n) >> 31)) + 7)) == 0)
                    {   //  the unicode character used an overlong or modified encoding: a 2 byte UTF8 encoding of U+0000 is a common exception to the overlong encoding rule notably used in Java
                        errors |= (((value == 0) && (count == 2)) ? cp_errors::bits::ModifiedUTF8 : cp_errors::bits::OverlongUTF8);
                    }
                }
            }
        }
    }
    return errors;
}

// ==== internal low level UTF8 sequence scanning functions ====

//  Notes:
//  
//  These are helper functions for the backUTF8() and skipUTF8() functions.
//  
//  The bytes parameter is the number of bytes of qualifying code-point found.
//  The extra parameter is the number of invalid or illegal bytes following the code-point.

void backSeqUTF8(const uint8_t* const buffer, const uint32_t offset, const uint32_t limit, uint32_t& bytes, uint32_t& extra, const bool use_cesu) noexcept
{
    bytes = extra = 0;
    uint32_t check = 0;
    uint32_t count = 0;
    uint32_t index = offset;
    while (limit > count)
    {
        --index;
        ++count;
        uint8_t byte = buffer[index];
        if (byte <= 0xfdu)
        {   //  lead or continuation byte
            ++check;
            if ((byte & 0xc0u) != 0x80u)
            {   //  lead byte
                if (use_cesu)
                {   //  cesu
                    bytes = check;
                    if (byte <= 0x7fu)
                    {
                        bytes = 1;
                    }
                    else if (byte <= 0xdfu)
                    {
                        if (check >= 2)
                        {
                            bytes = 2;
                        }
                    }
                    else
                    {
                        const uint8_t* const verify = (buffer + index);
                        bool low_surrogate = false;
                        if (byte <= 0xefu)
                        {
                            if (check >= 3)
                            {
                                low_surrogate = ((byte == 0xedu) && ((verify[1] & 0xf0u) == 0xb0u));
                                bytes = 3;
                            }
                        }
                        else if (byte <= 0xf7u)
                        {
                            if (check >= 4)
                            {
                                low_surrogate = ((byte == 0xf0u) && (verify[1] == 0x8du) && ((verify[2] & 0xf0u) == 0xb0u));
                                bytes = 4;
                            }
                        }
                        else if (byte <= 0xfbu)
                        {
                            if (check >= 5)
                            {
                                low_surrogate = ((byte == 0xf8u) && (verify[2] == 0x8du) && ((verify[3] & 0xf0u) == 0xb0u));
                                bytes = 5;
                            }
                        }
                        else
                        {
                            if (check >= 6)
                            {
                                low_surrogate = ((byte == 0xfcu) && (verify[3] == 0x8du) && ((verify[4] & 0xf0u) == 0xb0u));
                                bytes = 6;
                            }
                        }
                        if (low_surrogate)
                        {   //  found a low surrogate, check for a leading high surrogate
                            check = (limit - count);
                            if (check >= 3)
                            {   //  a high surrogate is possible
                                if (((verify[-1] & 0xc0u) == 0x80u) && ((verify[-2] & 0xf0u) == 0xa0u))
                                {   //  initial bytes match a high surrogate, just need to check how long the encoding is
                                    byte = verify[-3];
                                    if (byte == 0xedu)
                                    {
                                        extra = 3;
                                    }
                                    else if ((byte == 0x8du) && (check >= 4))
                                    {
                                        byte = verify[-4];
                                        if (byte == 0xf0u)
                                        {
                                            extra = 4;
                                        }
                                        else if ((byte == 0x80u) && (check >= 5))
                                        {
                                            byte = verify[-5];
                                            if (byte == 0xf8u)
                                            {
                                                extra = 5;
                                            }
                                            else if ((byte == 0x80u) && (check >= 6))
                                            {
                                                byte = verify[-6];
                                                if (byte == 0xfcu)
                                                {
                                                    extra = 6;
                                                }
                                            }
                                        }
                                    }
                                    bytes += extra;
                                    count += extra;
                                }
                            }
                        }
                    }
                }
                else
                {   //  non-cesu
                    if (byte <= 0xdfu)
                    {
                        bytes = ((byte <= 0x7fu) ? 1 : 2);
                    }
                    else if (byte <= 0xf7u)
                    {
                        bytes = ((byte <= 0xefu) ? 3 : 4);
                    }
                    else
                    {
                        bytes = ((byte <= 0xfbu) ? 5 : 6);
                    }
                    if (bytes > check) bytes = check;
                }
                count -= bytes;
                break;
            }
        }
        else
        {   //  illegal byte
            check = 0;
        }
    }
    extra = count;
}

void stepSeqUTF8(const uint8_t* const buffer, const uint32_t offset, const uint32_t limit, uint32_t& bytes, uint32_t& extra, const bool use_cesu) noexcept
{
    bytes = extra = 0;
    uint32_t count = 0;
    uint32_t index = offset;
    if (limit)
    {
        uint8_t byte = buffer[index];
        ++count;
        ++index;
        if (((byte & 0xc0u) != 0x80u) && (byte <= 0xfdu))
        {   //  sequence starts with a lead byte
            while (limit > count)
            {
                if ((buffer[index] & 0xc0u) != 0x80u)
                {   //  found end of continuation bytes
                    break;
                }
                ++count;
                ++index;
            }
            uint32_t check = count;
            if (use_cesu)
            {   //  cesu
                bytes = check;
                if (byte <= 0x7fu)
                {
                    bytes = 1;
                }
                else if (byte <= 0xdfu)
                {
                    if (limit >= 2)
                    {
                        bytes = 2;
                    }
                }
                else
                {
                    bool high_surrogate = false;
                    if (byte <= 0xefu)
                    {
                        if (check >= 3)
                        {
                            high_surrogate = (byte == 0xedu) && ((buffer[1] & 0xf0u) == 0xa0u);
                            bytes = 3;
                        }
                    }
                    else if (byte <= 0xf7u)
                    {
                        if (check >= 4)
                        {
                            high_surrogate = (byte == 0xf0u) && (buffer[1] == 0x8du) && ((buffer[2] & 0xf0u) == 0xa0u);
                            bytes = 4;
                        }
                    }
                    else if (byte <= 0xfbu)
                    {
                        if (check >= 5)
                        {
                            high_surrogate = (byte == 0xf8u) && (buffer[2] == 0x8du) && ((buffer[3] & 0xf0u) == 0xa0u);
                            bytes = 5;
                        }
                    }
                    else
                    {
                        if (check >= 6)
                        {
                            high_surrogate = (byte == 0xfcu) && (buffer[3] == 0x8du) && ((buffer[4] & 0xf0u) == 0xa0u);
                            bytes = 6;
                        }
                    }
                    if (high_surrogate)
                    {
                        check = (limit - bytes);
                        if (check >= 3)
                        {   //  a high surrogate is possible
                            const uint8_t* const verify = (buffer + bytes);
                            switch (verify[0])
                            {
                                case(0xedu):
                                {
                                    extra = 3;
                                    break;
                                }
                                case(0xf0u):
                                {
                                    if ((check >= 4) && (verify[1] == 0x8du))
                                    {
                                        extra = 4;
                                    }
                                    break;
                                }
                                case(0xf8u):
                                {
                                    if ((check >= 5) && ((verify[1] & 0xc0u) == 0x80u) && (verify[2] == 0x8du))
                                    {
                                        extra = 5;
                                    }
                                    break;
                                }
                                case(0xfcu):
                                {
                                    if ((check >= 6) && ((verify[1] & 0xc0u) == 0x80u) && ((verify[2] & 0xc0u) == 0x80u) && (verify[3] == 0x8du))
                                    {
                                        extra = 6;
                                    }
                                    break;
                                }
                                default:
                                {
                                    break;
                                }
                            }
                            if (extra)
                            {
                                if (((verify[extra - 2] & 0xf0u) == 0xb0u) && ((verify[extra - 1] & 0xc0u) == 0x80u))
                                {
                                    bytes += extra;
                                    count += extra;
                                    index += extra;
                                }
                            }
                        }
                    }
                }
            }
            else
            {   //  non-cesu
                if (byte <= 0xdfu)
                {
                    bytes = ((byte <= 0x7fu) ? 1 : 2);
                }
                else if (byte <= 0xf7u)
                {
                    bytes = ((byte <= 0xefu) ? 3 : 4);
                }
                else
                {
                    bytes = ((byte <= 0xfbu) ? 5 : 6);
                }
                if (bytes > check) bytes = check;
            }
            count -= bytes;
        }
        while (limit > count)
        {
            byte = buffer[index];
            if (((byte & 0xc0u) != 0x80u) && (byte <= 0xfdu))
            {   //  found next lead byte
                break;
            }
            ++count;
            ++index;
        }
        extra = count;
    }
}

void backSeqUTF8st(const uint8_t* const buffer, const uint32_t offset, const uint32_t limit, uint32_t& bytes, uint32_t& extra, const bool use_cesu, const bool use_java) noexcept
{
    bytes = extra = 0;
    uint32_t check = 0;
    uint32_t count = 0;
    uint32_t index = offset;
    while (limit > count)
    {
        --index;
        ++count;
        uint8_t byte = buffer[index];
        if (byte <= 0xf7u)
        {   //  lead or continuation byte
            ++check;
            if ((byte & 0xc0u) != 0x80u)
            {   //  lead byte
                if (byte <= 0x7fu)
                {
                    bytes = 1;
                }
                else
                {
                    const uint8_t* const verify = (buffer + index);
                    if ((check >= 2) && ((verify[1] & 0xc0u) == 0x80u))
                    {
                        uint16_t leading = ((static_cast<uint16_t>(byte) << 8) | verify[1]);
                        if (byte <= 0xdfu)
                        {
                            if ((leading >= 0xc280u) || (use_java && (leading == 0xc080u)))
                            {   //  >= 0x00000080 or java and == 0x00000000
                                bytes = 2;
                            }
                        }
                        else if ((check >= 3) && ((verify[2] & 0xc0u) == 0x80u))
                        {
                            if (byte <= 0xefu)
                            {
                                if (leading >= 0xe0a0u)
                                {   //  >= 0x00000800
                                    if ((leading & 0xffe0u) != 0xeda0u)
                                    {   //  not a surrogate value
                                        bytes = 3;
                                    }
                                    else if (use_cesu && ((leading & 0xfff0u) == 0xedb0u) && ((limit - count) >= 3))
                                    {   //  using cesu and found a low surrogate and there are enough bytes for a leading high surrogate
                                        if (((verify[-1] & 0xc0u) == 0x80u) && ((verify[-2] & 0xf0u) == 0xa0u) && (verify[-3] == 0xedu))
                                        {   //  found a surrogate pair
                                            bytes = 6;
                                            count += 3;
                                        }
                                    }
                                }
                            }
                            else if ((check >= 4) && ((verify[3] & 0xc0u) == 0x80u))
                            {
                                if ((leading >= 0xf090u) && (leading <= 0xf48fu))
                                {   //  >= 0x00010000 and <= 0x0010ffff 
                                    bytes = 4;
                                }
                            }
                        }
                    }
                }
                count -= bytes;
                break;
            }
        }
        else
        {   //  illegal byte
            check = 0;
        }
    }
    extra = count;
}

void stepSeqUTF8st(const uint8_t* const buffer, const uint32_t offset, const uint32_t limit, uint32_t& bytes, uint32_t& extra, const bool use_cesu, const bool use_java) noexcept
{
    bytes = extra = 0;
    uint32_t count = 0;
    uint32_t index = offset;
    if (limit)
    {
        uint8_t byte = buffer[index];
        if (((byte & 0xc0u) != 0x80u) && (byte <= 0xf7u))
        {   //  sequence starts with a lead byte
            if (byte <= 0x7fu)
            {
                bytes = 1;
            }
            else if ((limit >= 2) && ((buffer[1] & 0xc0u) == 0x80u))
            {
                uint16_t leading = ((static_cast<uint16_t>(byte) << 8) | buffer[1]);
                if (byte <= 0xdfu)
                {
                    if ((leading >= 0xc280u) || (use_java && (leading == 0xc080u)))
                    {   //  >= 0x00000080 or java and == 0x00000000
                        bytes = 2;
                    }
                }
                else if ((limit >= 3) && ((buffer[2] & 0xc0u) == 0x80u))
                {
                    if (byte <= 0xefu)
                    {
                        if (leading >= 0xe0a0u)
                        {   //  >= 0x00000800
                            if ((leading & 0xffe0u) != 0xeda0u)
                            {   //  not a surrogate value
                                bytes = 3;
                            }
                            else if (use_cesu && ((leading & 0xfff0u) == 0xeda0u) && (limit >= 6))
                            {   //  using cesu and found a low surrogate and there are enough bytes for a leading high surrogate
                                if ((buffer[3] == 0xedu) && ((buffer[4] & 0xf0u) == 0xa0u) && ((buffer[5] & 0xc0u) == 0x80u))
                                {   //  found a surrogate pair
                                    bytes = 6;
                                }
                            }
                        }
                    }
                    else if ((limit >= 4) && ((buffer[3] & 0xc0u) == 0x80u))
                    {
                        if ((leading >= 0xf090u) && (leading <= 0xf48fu))
                        {   //  >= 0x00010000 and <= 0x0010ffff 
                            bytes = 4;
                        }
                    }
                }
            }
        }
        count = (bytes ? bytes : 1);
        index += count;
        while (limit > count)
        {
            byte = buffer[index];
            if (((byte & 0xc0u) != 0x80u) && (byte <= 0xf7u))
            {   //  found next lead byte
                break;
            }
            ++count;
            ++index;
        }
        extra = (count - bytes);
    }
}

};  //  namespace internal

// ==== encoded code-point length functions ====

uint32_t lenUTF8(const unicode_t unicode, const bool use_cesu, const bool use_java) noexcept
{
    uint32_t bytes = 0;
    if (static_cast<uint32_t>(unicode) <= 0x7fffffffu)
    {   //  is an encodable unicode value
        if (unicode <= 0x0000007fu)
        {   //  1 byte (7 bits)
            bytes = ((use_java && (unicode == 0x00000000u)) ? 2 : 1);
        }
        else if (unicode <= 0x000007ffu)
        {   //  2 bytes (11 bits)
            bytes = 2;
        }
        else if (unicode <= 0x0000ffffu)
        {   //  3 bytes (16 bits)
            bytes = 3;
        }
        else if (unicode <= 0x0010ffffu)
        {   //  4 bytes (standard UTF8: 21 bits) or 6 bytes (CESU UTF8: UTF16 surrogates encoded as 2 UTF8 characters)
            bytes = (use_cesu ? 6 : 4);
        }
        else if (unicode <= 0x001fffffu)
        {   //  4 bytes (21 bits)
            bytes = 4;
        }
        else if (unicode <= 0x03ffffffu)
        {   //  5 bytes (26 bits)
            bytes = 5;
        }
        else
        {   //  6 bytes (31 bits)
            bytes = 6;
        }
    }
    return bytes;
}

uint32_t lenUTF16(const unicode_t unicode, const bool use_ucs2) noexcept
{
    uint32_t bytes = 0;
    if (static_cast<uint32_t>(unicode) <= 0x0010ffffu)
    {
        if (unicode <= 0x0000ffffu)
        {
            bytes = 2;
        }
        else if (!use_ucs2)
        {
            bytes = 4;
        }
    }
    return bytes;
}

uint32_t lenUTF32(const unicode_t unicode, const bool use_cesu, const bool use_ucs4) noexcept
{
    uint32_t bytes = 0;
    if (static_cast<uint32_t>(unicode) <= uint32_t(use_ucs4 ? 0x7fffffffu : 0x0010ffffu))
    {
        bytes = ((use_cesu && (unicode >= 0x00010000u) && (unicode <= 0x0010ffffu)) ? 8 : 4);
    }
    return bytes;
}

// ==== low level code-point encoding functions ====

[[nodiscard]] cp_errors encodeBYTE(utf_text& text, const unicode_t unicode, uint32_t& bytes, const bool use_ascii) noexcept
{
    bytes = 0;
    cp_errors errors = get_errors(text);
    if (unicode <= 0x00000000u)
    {
        errors |= (unicode ? (cp_errors::bits::Failed | cp_errors::bits::NotEncodable | cp_errors::bits::InvalidPoint | cp_errors::bits::NotEnoughBits) : cp_errors::bits::DelimitString);
    }
    else if (static_cast<uint32_t>(unicode) > (use_ascii ? 0x0000007fu : 0x000000ffu))
    {
        errors |= (cp_errors::bits::Failed | cp_errors::bits::NotEncodable | cp_errors::bits::NotEnoughBits);
        if (unicode >= 0x0000d800u)
        {
            if (unicode > 0x0010ffffu)
            {
                errors |= cp_errors::bits::ExtendedUCS4;
            }
            else if (unicode >= 0x0000fdd0u)
            {
                if ((unicode <= 0x0000fdefu) || ((unicode & 0x0000fffeu) == 0x0000fffeu))
                {
                    errors |= cp_errors::bits::NonCharacter;
                }
                if (unicode > 0x0000ffffu)
                {
                    errors |= cp_errors::bits::Supplementary;
                }
            }
            else if ((unicode & 0xfffff800u) == 0x0000d800u)
            {
                errors |= ((unicode & 0x00000400u) ? cp_errors::bits::LowSurrogate : cp_errors::bits::HighSurrogate);
            }
        }
    }
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < 1)
        {   //  buffer overflow
            errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
        }
        else
        {
            uint8_t* const buffer = &text.buffer[text.offset];
            buffer[0] = static_cast<uint8_t>(unicode);
            bytes = 1;
        }
    }
    return errors;
}

[[nodiscard]] cp_errors encodeUTF8(utf_text& text, const unicode_t unicode, uint32_t& bytes, const bool use_cesu, const bool use_java) noexcept
{
    bytes = 0;
    cp_errors errors = get_errors(text);
    if (unicode <= 0x00000000u)
    {
        errors |= (unicode ? (cp_errors::bits::Failed | cp_errors::bits::NotEncodable | cp_errors::bits::InvalidPoint | cp_errors::bits::NotEnoughBits) : (use_java ? cp_errors::bits::ModifiedUTF8 : cp_errors::bits::DelimitString));
    }
    else if (unicode >= 0x0000d800u)
    {
        if (unicode > 0x0010ffffu)
        {
            errors |= ((unicode > 0x001fffffu) ? (cp_errors::bits::ExtendedUTF8 | cp_errors::bits::ExtendedUCS4 | cp_errors::bits::IrregularForm) : (cp_errors::bits::ExtendedUCS4 | cp_errors::bits::IrregularForm));
        }
        else if (unicode >= 0x0000fdd0u)
        {
            if ((unicode <= 0x0000fdefu) || ((unicode & 0x0000fffeu) == 0x0000fffeu))
            {
                errors |= cp_errors::bits::NonCharacter;
            }
            if (unicode > 0x0000ffffu)
            {
                errors |= (use_cesu ? (cp_errors::bits::Supplementary | cp_errors::bits::SurrogatePair) : cp_errors::bits::Supplementary);
            }
        }
        else if ((unicode & 0xfffff800u) == 0x0000d800u)
        {
            errors |= ((unicode & 0x00000400u) ? (cp_errors::bits::LowSurrogate | cp_errors::bits::IrregularForm) : (cp_errors::bits::HighSurrogate | cp_errors::bits::IrregularForm));
        }
    }
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        uint8_t* const buffer = &text.buffer[text.offset];
        if (unicode <= 0x0000007fu)
        {   //  1 byte (standard UTF8: 7 bits) or 2 bytes (modified NULL: 11 bits)
            if (errors.any(cp_errors::bits::ModifiedUTF8))
            {   //  2 bytes (modified NULL: 11 bits)
                if (limit < 2)
                {   //  buffer overflow
                    errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
                }
                else
                {
                    buffer[0] = 0xc0u;
                    buffer[1] = 0x80u;
                    bytes = 2;
                }
            }
            else
            {   //  1 byte (standard UTF8: 7 bits)
                if (limit < 1)
                {   //  buffer overflow
                    errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
                }
                else
                {
                    buffer[0] = static_cast<uint8_t>(unicode);
                    bytes = 1;
                }
            }
        }
        else if (unicode <= 0x000007ffu)
        {   //  2 bytes (11 bits)
            if (limit < 2)
            {   //  buffer overflow
                errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
            }
            else
            {
                buffer[0] = (static_cast<uint8_t>(unicode >> 6) | 0xc0u);
                buffer[1] = ((static_cast<uint8_t>(unicode) & 0x3fu) | 0x80u);
                bytes = 2;
            }
        }
        else if (unicode <= 0x0000ffffu)
        {   //  3 bytes (16 bits)
            if (limit < 3)
            {   //  buffer overflow
                errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
            }
            else
            {
                buffer[0] = (static_cast<uint8_t>(unicode >> 12) | 0xe0u);
                buffer[1] = ((static_cast<uint8_t>(unicode >> 6) & 0x3fu) | 0x80u);
                buffer[2] = ((static_cast<uint8_t>(unicode) & 0x3fu) | 0x80u);
                bytes = 3;
            }
        }
        else if ((unicode <= 0x0010ffffu) && errors.any(cp_errors::bits::SurrogatePair))
        {   //  6 bytes (CESU UTF8: UTF16 surrogates encoded as 2 UTF8 characters)
            if (limit < 6)
            {   //  buffer overflow
                errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
            }
            else
            {
                unicode_t surrogate = (unicode - 0x00010000u);
                surrogate = ((((surrogate >> 10) | (surrogate << 16)) & 0x03ff03ffu) | 0xdc00d800u);
                buffer[0] = ((static_cast<uint8_t>(surrogate >> 12) & 0x0fu) | 0xe0u);
                buffer[1] = ((static_cast<uint8_t>(surrogate >> 6) & 0x3fu) | 0x80u);
                buffer[2] = ((static_cast<uint8_t>(surrogate) & 0x3fu) | 0x80u);
                buffer[3] = ((static_cast<uint8_t>(surrogate >> 28) & 0x0fu) | 0xe0u);
                buffer[4] = ((static_cast<uint8_t>(surrogate >> 22) & 0x3fu) | 0x80u);
                buffer[5] = ((static_cast<uint8_t>(surrogate >> 16) & 0x3fu) | 0x80u);
                bytes = 6;
            }
        }
        else if (unicode <= 0x001fffffu)
        {   //  4 bytes (21 bits)
            if (limit < 4)
            {   //  buffer overflow
                errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
            }
            else
            {
                buffer[0] = (static_cast<uint8_t>(unicode >> 18) | 0xf0u);
                buffer[1] = ((static_cast<uint8_t>(unicode >> 12) & 0x3fu) | 0x80u);
                buffer[2] = ((static_cast<uint8_t>(unicode >> 6) & 0x3fu) | 0x80u);
                buffer[3] = ((static_cast<uint8_t>(unicode) & 0x3fu) | 0x80u);
                bytes = 4;
            }
        }
        else if (unicode <= 0x03ffffffu)
        {   //  5 bytes (26 bits)
            if (limit < 5)
            {   //  buffer overflow
                errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
            }
            else
            {
                buffer[0] = (static_cast<uint8_t>(unicode >> 24) | 0xf8u);
                buffer[1] = ((static_cast<uint8_t>(unicode >> 18) & 0x3fu) | 0x80u);
                buffer[2] = ((static_cast<uint8_t>(unicode >> 12) & 0x3fu) | 0x80u);
                buffer[3] = ((static_cast<uint8_t>(unicode >> 6) & 0x3fu) | 0x80u);
                buffer[4] = ((static_cast<uint8_t>(unicode) & 0x3fu) | 0x80u);
                bytes = 5;
            }
        }
        else
        {   //  6 bytes (31 bits)
            if (limit < 6)
            {   //  buffer overflow
                errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
            }
            else
            {
                buffer[0] = (static_cast<uint8_t>(unicode >> 30) | 0xfcu);
                buffer[1] = ((static_cast<uint8_t>(unicode >> 24) & 0x3fu) | 0x80u);
                buffer[2] = ((static_cast<uint8_t>(unicode >> 18) & 0x3fu) | 0x80u);
                buffer[3] = ((static_cast<uint8_t>(unicode >> 12) & 0x3fu) | 0x80u);
                buffer[4] = ((static_cast<uint8_t>(unicode >> 6) & 0x3fu) | 0x80u);
                buffer[5] = ((static_cast<uint8_t>(unicode) & 0x3fu) | 0x80u);
                bytes = 6;
            }
        }
    }
    return errors;
}

[[nodiscard]] cp_errors encodeUTF8n(utf_text& text, const unicode_t unicode, const uint32_t bytes, const bool use_java) noexcept
{
    cp_errors errors = get_errors(text);
    if (static_cast<uint32_t>(bytes - 1) < 6)
    {
        if (unicode == 0x00000000u)
        {
            errors |= ((bytes < 2) ? cp_errors::bits::DelimitString : ((bytes > 2) ? (cp_errors::bits::OverlongUTF8 | cp_errors::bits::IrregularForm) : (use_java ? cp_errors::bits::ModifiedUTF8 : (cp_errors::bits::ModifiedUTF8 | cp_errors::bits::IrregularForm))));
        }
        else
        {
            int32_t n = (static_cast<int32_t>(bytes | (bytes << 2)) - 6);
            if ((unicode >> static_cast<uint32_t>((n & ((~n) >> 31)) + 7)) != 0)
            {
                errors |= (cp_errors::bits::Failed | cp_errors::bits::NotEncodable | cp_errors::bits::NotEnoughBits);
            }
            else if (bytes > 1)
            {
                n -= 5;
                if ((unicode >> static_cast<uint32_t>((n & ((~n) >> 31)) + 7)) == 0)
                {
                    errors |= (cp_errors::bits::OverlongUTF8 | cp_errors::bits::IrregularForm);
                }
            }
        }
    }
    else
    {
        errors |= (cp_errors::bits::Failed | cp_errors::bits::NotEncodable | cp_errors::bits::BadSizeUTF8);
    }
    if (unicode < 0x00000000u)
    {
        errors |= (cp_errors::bits::Failed | cp_errors::bits::NotEncodable | cp_errors::bits::InvalidPoint | cp_errors::bits::NotEnoughBits);
    }
    else if (unicode >= 0x0000d800u)
    {
        if (unicode > 0x0010ffffu)
        {
            errors |= ((unicode > 0x001fffffu) ? (cp_errors::bits::ExtendedUTF8 | cp_errors::bits::ExtendedUCS4 | cp_errors::bits::IrregularForm) : (cp_errors::bits::ExtendedUCS4 | cp_errors::bits::IrregularForm));
        }
        else if (unicode >= 0x0000fdd0u)
        {
            if ((unicode <= 0x0000fdefu) || ((unicode & 0x0000fffeu) == 0x0000fffeu))
            {
                errors |= cp_errors::bits::NonCharacter;
            }
            if (unicode > 0x0000ffffu)
            {
                errors |= cp_errors::bits::Supplementary;
            }
        }
        else if ((unicode & 0xfffff800u) == 0x0000d800u)
        {
            errors |= ((unicode & 0x00000400u) ? (cp_errors::bits::LowSurrogate | cp_errors::bits::IrregularForm) : (cp_errors::bits::HighSurrogate | cp_errors::bits::IrregularForm));
        }
    }
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < bytes)
        {   //  buffer overflow
            errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
        }
        else
        {
            uint8_t* const buffer = &text.buffer[text.offset];
            unicode_t value = unicode;
            for (uint32_t index = (bytes - 1); index; --index)
            {
                buffer[index] = ((static_cast<uint8_t>(value) & 0x3fu) | 0x80u);
                value >>= 6;
            }
            uint8_t mask = ((bytes > 1) ? (0x7fu >> bytes) : 0x7fu);
            buffer[0] = ((static_cast<uint8_t>(value) & mask) | (~mask << 1));
        }
    }
    return errors;
}

[[nodiscard]] cp_errors encodeUTF16(utf_text& text, const unicode_t unicode, uint32_t& bytes, const bool le, const bool use_ucs2) noexcept
{
    bytes = 0;
    cp_errors errors = get_errors(text, 1);
    if (unicode <= 0x00000000u)
    {
        errors |= (unicode ? (cp_errors::bits::Failed | cp_errors::bits::NotEncodable | cp_errors::bits::InvalidPoint | cp_errors::bits::NotEnoughBits) : cp_errors::bits::DelimitString);
    }
    else if (unicode >= 0x0000d800u)
    {
        if (unicode > 0x0010ffffu)
        {
            errors |= (cp_errors::bits::Failed | cp_errors::bits::ExtendedUCS4 | cp_errors::bits::NotEnoughBits);
        }
        else if (unicode >= 0x0000fdd0u)
        {
            if ((unicode <= 0x0000fdefu) || ((unicode & 0x0000fffeu) == 0x0000fffeu))
            {
                errors |= cp_errors::bits::NonCharacter;
            }
            if (unicode > 0x0000ffffu)
            {
                errors |= (use_ucs2 ? (cp_errors::bits::Failed | cp_errors::bits::Supplementary | cp_errors::bits::NotEnoughBits) : (cp_errors::bits::Supplementary | cp_errors::bits::SurrogatePair));
            }
        }
        else if ((unicode & 0xfffff800u) == 0x0000d800u)
        {
            errors |= ((unicode & 0x00000400u) ? (cp_errors::bits::LowSurrogate | cp_errors::bits::IrregularForm) : (cp_errors::bits::HighSurrogate | cp_errors::bits::IrregularForm));
        }
    }
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        uint8_t* const buffer = &text.buffer[text.offset];
        if (errors.any(cp_errors::bits::SurrogatePair))
        {   //  surrogate pair
            if (limit < 4)
            {   //  buffer overflow
                errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
            }
            else
            {
                unicode_t surrogate = (unicode - 0x00010000u);
                surrogate = ((((surrogate >> 10) | (surrogate << 16)) & 0x03ff03ffu) | 0xdc00d800u);
                if (le)
                {
                    buffer[0] = static_cast<uint8_t>(surrogate);
                    buffer[1] = static_cast<uint8_t>(surrogate >> 8);
                    buffer[2] = static_cast<uint8_t>(surrogate >> 16);
                    buffer[3] = static_cast<uint8_t>(surrogate >> 24);
                }
                else
                {
                    buffer[0] = static_cast<uint8_t>(surrogate >> 8);
                    buffer[1] = static_cast<uint8_t>(surrogate);
                    buffer[2] = static_cast<uint8_t>(surrogate >> 24);
                    buffer[3] = static_cast<uint8_t>(surrogate >> 16);
                }
                bytes = 4;
            }
        }
        else
        {   //  simple UTF16
            if (limit < 2)
            {   //  buffer overflow
                errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
            }
            else
            {
                if (le)
                {
                    buffer[0] = static_cast<uint8_t>(unicode);
                    buffer[1] = static_cast<uint8_t>(unicode >> 8);
                }
                else
                {
                    buffer[0] = static_cast<uint8_t>(unicode >> 8);
                    buffer[1] = static_cast<uint8_t>(unicode);
                }
                bytes = 2;
            }
        }
    }
    return errors;
}

[[nodiscard]] cp_errors encodeUTF32(utf_text& text, const unicode_t unicode, uint32_t& bytes, const bool le, const bool use_cesu, const bool use_ucs4) noexcept
{
    bytes = 0;
    cp_errors errors = get_errors(text, 3);
    if (unicode <= 0x00000000u)
    {
        errors |= (unicode ? cp_errors::bits::InvalidPoint : cp_errors::bits::DelimitString);
    }
    else if (unicode >= 0x0000d800u)
    {
        if (unicode > 0x0010ffffu)
        {
            errors |= (use_ucs4 ? cp_errors::bits::ExtendedUCS4 : (cp_errors::bits::ExtendedUCS4 | cp_errors::bits::IrregularForm));
        }
        else if (unicode >= 0x0000fdd0u)
        {
            if ((unicode <= 0x0000fdefu) || ((unicode & 0x0000fffeu) == 0x0000fffeu))
            {
                errors |= cp_errors::bits::NonCharacter;
            }
            if (unicode > 0x0000ffffu)
            {
                errors |= (use_cesu ? (cp_errors::bits::Supplementary | cp_errors::bits::SurrogatePair) : cp_errors::bits::Supplementary);
            }
        }
        else if ((unicode & 0xfffff800u) == 0x0000d800u)
        {
            errors |= ((unicode & 0x00000400u) ? (cp_errors::bits::LowSurrogate | cp_errors::bits::IrregularForm) : (cp_errors::bits::HighSurrogate | cp_errors::bits::IrregularForm));
        }
    }
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        uint8_t* const buffer = &text.buffer[text.offset];
        if (errors.any(cp_errors::bits::SurrogatePair))
        {   //  surrogate pair
            if (limit < 8)
            {   //  buffer overflow
                errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
            }
            else
            {
                unicode_t surrogate = (unicode - 0x00010000u);
                surrogate = ((((surrogate >> 10) | (surrogate << 16)) & 0x03ff03ffu) | 0xdc00d800u);
                if (le)
                {
                    buffer[0] = static_cast<uint8_t>(surrogate);
                    buffer[1] = static_cast<uint8_t>(surrogate >> 8);
                    buffer[2] = buffer[3] = static_cast<uint8_t>(0);
                    buffer[4] = static_cast<uint8_t>(surrogate >> 16);
                    buffer[5] = static_cast<uint8_t>(surrogate >> 24);
                    buffer[6] = buffer[7] = static_cast<uint8_t>(0);
                }
                else
                {
                    buffer[0] = buffer[1] = static_cast<uint8_t>(0);
                    buffer[2] = static_cast<uint8_t>(surrogate >> 8);
                    buffer[3] = static_cast<uint8_t>(surrogate);
                    buffer[4] = buffer[5] = static_cast<uint8_t>(0);
                    buffer[6] = static_cast<uint8_t>(surrogate >> 24);
                    buffer[7] = static_cast<uint8_t>(surrogate >> 16);
                }
                bytes = 8;
            }
        }
        else
        {   //  simple UTF32
            if (limit < 4)
            {   //  buffer overflow
                errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
            }
            else
            {
                if (le)
                {
                    buffer[0] = static_cast<uint8_t>(unicode);
                    buffer[1] = static_cast<uint8_t>(unicode >> 8);
                    buffer[2] = static_cast<uint8_t>(unicode >> 16);
                    buffer[3] = static_cast<uint8_t>(unicode >> 24);
                }
                else
                {
                    buffer[0] = static_cast<uint8_t>(unicode >> 24);
                    buffer[1] = static_cast<uint8_t>(unicode >> 16);
                    buffer[2] = static_cast<uint8_t>(unicode >> 8);
                    buffer[3] = static_cast<uint8_t>(unicode);
                }
                bytes = 4;
            }
        }
    }
    return errors;
}

[[nodiscard]] cp_errors encodeCP1252(utf_text& text, const unicode_t unicode, uint32_t& bytes, const bool strict) noexcept
{
    bytes = 0;
    uint8_t cp1252 = 0;
    cp_errors errors = get_errors(text);
    if (unicode <= 0x00000000u)
    {
        errors |= (unicode ? (cp_errors::bits::Failed | cp_errors::bits::NotEncodable | cp_errors::bits::InvalidPoint | cp_errors::bits::NotEnoughBits) : cp_errors::bits::DelimitString);
    }
    else if (!unicodeToCP1252(unicode, cp1252, (strict ? CP1252Strictness::StrictUndefined : CP1252Strictness::WindowsCompatible)))
    {
        errors |= (cp_errors::bits::Failed | cp_errors::bits::NotEncodable);
        if (unicode >= 0x0000d800u)
        {
            if (unicode > 0x0010ffffu)
            {
                errors |= cp_errors::bits::ExtendedUCS4;
            }
            else if (unicode >= 0x0000fdd0u)
            {
                if ((unicode <= 0x0000fdefu) || ((unicode & 0x0000fffeu) == 0x0000fffeu))
                {
                    errors |= cp_errors::bits::NonCharacter;
                }
                if (unicode > 0x0000ffffu)
                {
                    errors |= cp_errors::bits::Supplementary;
                }
            }
            else if ((unicode & 0xfffff800u) == 0x0000d800u)
            {
                errors |= ((unicode & 0x00000400u) ? cp_errors::bits::LowSurrogate : cp_errors::bits::HighSurrogate);
            }
        }
    }
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < 1)
        {   //  buffer overflow
            errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
        }
        else
        {
            uint8_t* const buffer = &text.buffer[text.offset];
            buffer[0] = cp1252;
            bytes = 1;
        }
    }
    return errors;
}

// ==== low level code-point decoding functions ====

[[nodiscard]] cp_errors decodeBYTE(const utf_text& text, unicode_t& unicode, uint32_t& bytes, const bool use_ascii, const bool coalesce) noexcept
{
    bytes = 0;
    unicode = 0;
    cp_errors errors = get_errors(text);
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < 1)
        {   //  buffer over-run
            errors |= (limit ? (cp_errors::bits::Failed | cp_errors::bits::ReadTruncated) : cp_errors::bits::ReadExhausted);
        }
        else
        {
            uint8_t* const buffer = &text.buffer[text.offset];
            unicode = static_cast<unicode_t>(buffer[0]);
            bytes = 1;
            if (use_ascii && (unicode & 0x00000080u))
            {
                errors |= (cp_errors::bits::Failed | cp_errors::bits::NotDecodable | cp_errors::bits::DisallowedByte);
                if (coalesce)
                {
                    uint32_t count = limit;
                    for (uint32_t index = 1; index < count; ++index)
                    {
                        if ((buffer[index] & 0x80u) != 0x80u)
                        {
                            count = index;
                            break;
                        }
                    }
                    bytes = count;
                }
            }
            else if (unicode == 0x00000000u)
            {
                errors |= cp_errors::bits::DelimitString;
            }
        }
    }
    return errors;
}

[[nodiscard]] cp_errors decodeUTF8(const utf_text& text, unicode_t& unicode, uint32_t& bytes, const bool use_cesu, const bool use_java, const bool strict, const bool coalesce) noexcept
{
    bytes = 0;
    unicode = 0;
    cp_errors errors = get_errors(text);
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        uint8_t* const buffer = &text.buffer[text.offset];
        errors |= internal::fetchUTF8(buffer, limit, unicode, bytes, (coalesce && !strict));
        if (errors.no_error())
        {   //  successfully read a UTF8 code-point
            if (unicode >= 0x0000d800u)
            {
                if (unicode > 0x0010ffffu)
                {
                    errors |= cp_errors::bits::ExtendedUCS4;
                }
                else if (unicode >= 0x0000fdd0u)
                {
                    if ((unicode <= 0x0000fdefu) || ((unicode & 0x0000fffeu) == 0x0000fffeu))
                    {
                        errors |= cp_errors::bits::NonCharacter;
                    }
                    if (unicode > 0x0000ffffu)
                    {
                        errors |= cp_errors::bits::Supplementary;
                    }
                }
                else if ((unicode & 0xfffff800u) == 0x0000d800u)
                {   //  found a surrogate
                    if (unicode & 0x00000400u)
                    {   //  found an unpaired low surrogate
                        errors |= cp_errors::bits::LowSurrogate;
                    }
                    else
                    {   //  found an unpaired or leading high surrogate
                        errors |= cp_errors::bits::HighSurrogate;
                        if (use_cesu)
                        {   //  found a leading high surrogate
                            uint32_t extra = 0;
                            unicode_t lowbits = 0;
                            cp_errors check = internal::fetchUTF8(&buffer[bytes], (limit - bytes), lowbits, extra, false);
                            if (check & (cp_errors::bits::ReadTruncated | cp_errors::bits::ReadExhausted))
                            {
                                errors |= cp_errors::bits::TruncatedPair;
                            }
                            else if (check.no_error())
                            {
                                if ((lowbits & 0xfffffc00u) == 0x0000dc00u)
                                {   //  found low surrogate (valid surrogate pair)
                                    unicode = (((unicode & 0x000003ffu) << 10) + (lowbits & 0x000003ffu) + 0x00010000u);
                                    bytes += extra;
                                    errors |= check;
                                    errors ^= (cp_errors::bits::SurrogatePair | cp_errors::bits::Supplementary | cp_errors::bits::HighSurrogate);
                                    if ((unicode & 0x0000fffeu) == 0x0000fffeu)
                                    {
                                        errors |= cp_errors::bits::NonCharacter;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if ((unicode == 0x00000000u) && (errors.none(cp_errors::bits::ModifiedUTF8 | cp_errors::bits::OverlongUTF8)))
            {
                errors |= cp_errors::bits::DelimitString;
            }
            if (errors.any((use_java ? cp_errors::bits::None : cp_errors::bits::ModifiedUTF8) | (cp_errors::bits::OverlongUTF8 | cp_errors::bits::ExtendedUTF8 | cp_errors::bits::ExtendedUCS4 | cp_errors::bits::HighSurrogate | cp_errors::bits::LowSurrogate)))
            {
                errors |= cp_errors::bits::IrregularForm;
                if (strict)
                {
                    errors |= (cp_errors::bits::Failed | cp_errors::bits::NotDecodable);
                    unicode = static_cast<unicode_t>(buffer[0]);
                    bytes = 1;
                }
            }
        }
        else if (strict && (bytes > 1))
        {
            bytes = 1;
        }
    }
    return errors;
}

[[nodiscard]] cp_errors decodeUTF16(const utf_text& text, unicode_t& unicode, uint32_t& bytes, const bool le, const bool use_ucs2) noexcept
{
    bytes = 0;
    unicode = 0;
    cp_errors errors = get_errors(text, 1);
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < 2)
        {   //  buffer over-run
            errors |= (limit ? (cp_errors::bits::Failed | cp_errors::bits::ReadTruncated) : cp_errors::bits::ReadExhausted);
        }
        else
        {
            uint8_t* const buffer = &text.buffer[text.offset];
            unicode = (le ? ((static_cast<unicode_t>(buffer[1]) << 8) + buffer[0]) : ((static_cast<unicode_t>(buffer[0]) << 8) + buffer[1]));
            bytes = 2;
            if (unicode >= 0x0000d800u)
            {
                if (unicode >= 0x0000fdd0u)
                {
                    if ((unicode <= 0x0000fdefu) || ((unicode & 0x0000fffeu) == 0x0000fffeu))
                    {
                        errors |= cp_errors::bits::NonCharacter;
                    }
                }
                else if ((unicode & 0xfffff800u) == 0x0000d800u)
                {   //  found a surrogate
                    errors |= cp_errors::bits::IrregularForm;
                    if (unicode & 0x00000400u)
                    {   //  found an unpaired low surrogate
                        errors |= cp_errors::bits::LowSurrogate;
                    }
                    else
                    {   //  found an unpaired or leading high surrogate
                        errors |= cp_errors::bits::HighSurrogate;
                        if (!use_ucs2)
                        {   //  found a leading high surrogate
                            if (limit < 4)
                            {   //  truncated surrogate pair
                                errors |= cp_errors::bits::TruncatedPair;
                            }
                            else
                            {
                                unicode_t lowbits = (le ? ((static_cast<unicode_t>(buffer[3]) << 8) + buffer[2]) : ((static_cast<unicode_t>(buffer[2]) << 8) + buffer[3]));
                                if ((lowbits & 0xfffffc00u) == 0x0000dc00u)
                                {   //  found low surrogate (valid surrogate pair)
                                    unicode = (((unicode & 0x000003ffu) << 10) + (lowbits & 0x000003ffu) + 0x00010000u);
                                    bytes = 4;
                                    errors ^= (cp_errors::bits::SurrogatePair | cp_errors::bits::Supplementary | cp_errors::bits::HighSurrogate | cp_errors::bits::IrregularForm);
                                    if ((unicode & 0x0000fffeu) == 0x0000fffeu)
                                    {
                                        errors |= cp_errors::bits::NonCharacter;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (unicode == 0x00000000u)
            {
                errors |= cp_errors::bits::DelimitString;
            }
        }
    }
    return errors;
}

[[nodiscard]] cp_errors decodeUTF32(const utf_text& text, unicode_t& unicode, uint32_t& bytes, const bool le, const bool use_cesu, const bool use_ucs4) noexcept
{
    bytes = 0;
    unicode = 0;
    cp_errors errors = get_errors(text, 3);
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        uint8_t* const buffer = &text.buffer[text.offset];
        if (limit < 4)
        {   //  buffer over-run
            errors |= (limit ? (cp_errors::bits::Failed | cp_errors::bits::ReadTruncated) : cp_errors::bits::ReadExhausted);
        }
        else
        {
            unicode = (le ?
                ((((((static_cast<unicode_t>(buffer[3]) << 8) + buffer[2]) << 8) + buffer[1]) << 8) + buffer[0]) :
                ((((((static_cast<unicode_t>(buffer[0]) << 8) + buffer[1]) << 8) + buffer[2]) << 8) + buffer[3]));
            bytes = 4;
            if (unicode <= 0x00000000u)
            {
                errors |= (unicode ? (cp_errors::bits::InvalidPoint | cp_errors::bits::IrregularForm) : cp_errors::bits::DelimitString);
            }
            else if (unicode >= 0x0000d800u)
            {
                if (unicode > 0x0010ffffu)
                {
                    errors |= (use_ucs4 ? cp_errors::bits::ExtendedUCS4 : (cp_errors::bits::ExtendedUCS4 | cp_errors::bits::IrregularForm));
                }
                else if (unicode >= 0x0000fdd0u)
                {
                    if ((unicode <= 0x0000fdefu) || ((unicode & 0x0000fffeu) == 0x0000fffeu))
                    {
                        errors |= cp_errors::bits::NonCharacter;
                    }
                    if (unicode > 0x0000ffffu)
                    {
                        errors |= cp_errors::bits::Supplementary;
                    }
                }
                else if ((unicode & 0xfffff800u) == 0x0000d800u)
                {   //  found a surrogate
                    errors |= cp_errors::bits::IrregularForm;
                    if (unicode & 0x00000400u)
                    {   //  found an unpaired low surrogate
                        errors |= cp_errors::bits::LowSurrogate;
                    }
                    else
                    {   //  found an unpaired or leading high surrogate
                        errors |= cp_errors::bits::HighSurrogate;
                        if (use_cesu)
                        {   //  found a leading high surrogate
                            if (limit < 8)
                            {   //  truncated surrogate pair
                                errors |= cp_errors::bits::TruncatedPair;
                            }
                            else
                            {
                                unicode_t lowbits = (le ?
                                    ((((((static_cast<unicode_t>(buffer[7]) << 8) + buffer[6]) << 8) + buffer[5]) << 8) + buffer[4]) :
                                    ((((((static_cast<unicode_t>(buffer[4]) << 8) + buffer[5]) << 8) + buffer[6]) << 8) + buffer[7]));
                                if ((lowbits & 0xfffffc00u) == 0x0000dc00u)
                                {   //  found low surrogate (valid surrogate pair)
                                    unicode = (((unicode & 0x000003ffu) << 10) + (lowbits & 0x000003ffu) + 0x00010000u);
                                    bytes = 8;
                                    errors ^= (cp_errors::bits::SurrogatePair | cp_errors::bits::Supplementary | cp_errors::bits::HighSurrogate | cp_errors::bits::IrregularForm);
                                    if ((unicode & 0x0000fffeu) == 0x0000fffeu)
                                    {
                                        errors |= cp_errors::bits::NonCharacter;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return errors;
}

[[nodiscard]] cp_errors decodeCP1252(const utf_text& text, unicode_t& unicode, uint32_t& bytes, const bool strict, const bool coalesce) noexcept
{
    bytes = 0;
    unicode = 0;
    cp_errors errors = get_errors(text);
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < 1)
        {   //  buffer over-run
            errors |= (limit ? (cp_errors::bits::Failed | cp_errors::bits::ReadTruncated) : cp_errors::bits::ReadExhausted);
        }
        else
        {
            CP1252Strictness strictness = strict ? CP1252Strictness::StrictUndefined : CP1252Strictness::WindowsCompatible;
            uint8_t* const buffer = &text.buffer[text.offset];
            uint8_t cp1252 = buffer[0];
            bytes = 1;
            if (!cp1252ToUnicode(cp1252, unicode, strictness))
            {
                errors |= (cp_errors::bits::Failed | cp_errors::bits::NotDecodable);
                if (coalesce)
                {
                    uint32_t count = limit;
                    for (uint32_t index = 1; index < count; ++index)
                    {
                        cp1252 = buffer[index];
                        if (cp1252ToUnicode(cp1252, unicode, strictness))
                        {
                            unicode = 0;
                            count = index;
                            break;
                        }
                    }
                    bytes = count;
                }
            }
            else if (unicode == 0x00000000u)
            {
                errors |= cp_errors::bits::DelimitString;
            }
        }
    }
    return errors;
}

// ==== low level utf byte order marker and NULL code-point fast encoding functions ====

[[nodiscard]] cp_errors encodeUTF8_BOM(utf_text& text, uint32_t& bytes) noexcept
{
    bytes = 0;
    cp_errors errors = get_errors(text);
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < 3)
        {   //  buffer overflow
            errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
        }
        else
        {
            uint8_t* const buffer = &text.buffer[text.offset];
            buffer[0] = 0xefu;
            buffer[1] = 0xbbu;
            buffer[2] = 0xbfu;
            bytes = 3;
        }
    }
    return errors;
}

[[nodiscard]] cp_errors encodeUTF16_BOM(utf_text& text, uint32_t& bytes, const bool le) noexcept
{
    bytes = 0;
    cp_errors errors = get_errors(text, 1);
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < 2)
        {   //  buffer overflow
            errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
        }
        else
        {
            uint8_t* const buffer = &text.buffer[text.offset];
            if (le)
            {
                buffer[0] = 0xffu;
                buffer[1] = 0xfeu;
            }
            else
            {
                buffer[0] = 0xfeu;
                buffer[1] = 0xffu;
            }
            bytes = 2;
        }
    }
    return errors;
}

[[nodiscard]] cp_errors encodeUTF32_BOM(utf_text& text, uint32_t& bytes, const bool le) noexcept
{
    bytes = 0;
    cp_errors errors = get_errors(text, 3);
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < 4)
        {   //  buffer overflow
            errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
        }
        else
        {
            uint8_t* const buffer = &text.buffer[text.offset];
            if (le)
            {
                buffer[0] = 0xffu;
                buffer[1] = 0xfeu;
                buffer[2] = buffer[3] = 0x00u;
            }
            else
            {
                buffer[0] = buffer[1] = 0x00u;
                buffer[2] = 0xfeu;
                buffer[3] = 0xffu;
            }
            bytes = 4;
        }
    }
    return errors;
}

[[nodiscard]] cp_errors encodeUTF8_NULL(utf_text& text, uint32_t& bytes) noexcept
{
    bytes = 0;
    cp_errors errors = get_errors(text);
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < 1)
        {   //  buffer overflow
            errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
        }
        else
        {
            uint8_t* const buffer = &text.buffer[text.offset];
            buffer[0] = 0x00u;
            bytes = 1;
        }
    }
    return errors;
}

[[nodiscard]] cp_errors encodeUTF16_NULL(utf_text& text, uint32_t& bytes) noexcept
{
    bytes = 0;
    cp_errors errors = get_errors(text, 1);
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < 2)
        {   //  buffer overflow
            errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
        }
        else
        {
            uint8_t* const buffer = &text.buffer[text.offset];
            buffer[0] = 0x00u;
            buffer[1] = 0x00u;
            bytes = 2;
        }
    }
    return errors;
}

[[nodiscard]] cp_errors encodeUTF32_NULL(utf_text& text, uint32_t& bytes) noexcept
{
    bytes = 0;
    cp_errors errors = get_errors(text, 3);
    if (errors.no_error())
    {
        const uint32_t limit = (text.length - text.offset);
        if (limit < 4)
        {   //  buffer overflow
            errors |= (cp_errors::bits::Failed | cp_errors::bits::WriteOverflow);
        }
        else
        {
            uint8_t* const buffer = &text.buffer[text.offset];
            buffer[0] = 0x00u;
            buffer[1] = 0x00u;
            buffer[2] = 0x00u;
            buffer[3] = 0x00u;
            bytes = 4;
        }
    }
    return errors;
}

// ==== encoded code-point data stream forward and backward skip functions ====
// ==== note: These functions return the count of code-points (not bytes) that were successfully skipped ====

uint32_t backBYTE(utf_text& text, const uint32_t count, const bool use_ascii, const bool coalesce) noexcept
{
    uint32_t points = 0;
    if (count && get_errors(text).no_error())
    {
        uint32_t limit = text.offset;
        if (use_ascii && coalesce)
        {
            const uint8_t* buffer = &text.buffer[text.offset];
            bool ascii = true;
            while ((points < count) && (limit > 0))
            {
                --limit;
                --buffer;
                if ((buffer[0] & 0x80u) != 0x80u)
                {
                    ++points;
                    ascii = true;
                }
                else if (ascii)
                {
                    ++points;
                    ascii = false;
                }
            }
            text.offset = limit;
        }
        else
        {
            points = ((count <= limit) ? count : limit);
            text.offset -= points;
        }
    }
    return points;
}

uint32_t stepBYTE(utf_text& text, const uint32_t count, const bool use_ascii, const bool coalesce) noexcept
{
    uint32_t points = 0;
    if (count && get_errors(text).no_error())
    {
        uint32_t limit = (text.length - text.offset);
        if (use_ascii && coalesce)
        {
            const uint8_t* buffer = &text.buffer[text.offset];
            bool ascii = true;
            while ((points < count) && (limit > 0))
            {
                --limit;
                if ((buffer[0] & 0x80u) != 0x80u)
                {
                    ++points;
                    ascii = true;
                }
                else if (ascii)
                {
                    ++points;
                    ascii = false;
                }
                ++buffer;
            }
            text.offset = (text.length - limit);
        }
        else
        {
            points = ((count <= limit) ? count : limit);
            text.offset += points;
        }
    }
    return points;
}

uint32_t backUTF8(utf_text& text, const uint32_t count, const bool use_cesu, const bool use_java, const bool strict, const bool coalesce) noexcept
{
    uint32_t points = 0;
    if (count && get_errors(text).no_error())
    {
        const uint8_t* const buffer = text.buffer;
        uint32_t offset = text.offset;
        uint32_t limit = offset;
        uint32_t bytes = 0;
        uint32_t extra = 0;
        while ((points < count) && (limit > 0))
        {
            if (bytes)
            {
                ++points;
                offset -= bytes;
                limit -= bytes;
                bytes = 0;
            }
            else
            {
                strict ? internal::backSeqUTF8st(buffer, offset, limit, bytes, extra, use_cesu, use_java) : internal::backSeqUTF8(buffer, offset, limit, bytes, extra, use_cesu);
                if (extra)
                {
                    if (coalesce && !strict)
                    {
                        ++points;
                        offset -= extra;
                        limit -= extra;
                    }
                    else
                    {
                        points += extra;
                        offset -= extra;
                        limit -= extra;
                        if (points > count)
                        {
                            offset += (points - count);
                            points = count;
                        }
                    }
                    extra = 0;
                }
            }
        }
        text.offset = offset;
    }
    return points;
}

uint32_t stepUTF8(utf_text& text, const uint32_t count, const bool use_cesu, const bool use_java, const bool strict, const bool coalesce) noexcept
{
    uint32_t points = 0;
    if (count && get_errors(text).no_error())
    {
        const uint8_t* const buffer = text.buffer;
        uint32_t offset = text.offset;
        uint32_t limit = (text.length - offset);
        uint32_t bytes = 0;
        uint32_t extra = 0;
        while ((points < count) && (limit > 0))
        {
            if (extra)
            {
                if (coalesce && !strict)
                {
                    ++points;
                    offset += extra;
                    limit -= extra;
                }
                else
                {
                    points += extra;
                    offset += extra;
                    limit -= extra;
                    if (points > count)
                    {
                        offset -= (points - count);
                        points = count;
                    }
                }
                extra = 0;
            }
            else
            {
                strict ? internal::stepSeqUTF8st(buffer, offset, limit, bytes, extra, use_cesu, use_java) : internal::stepSeqUTF8(buffer, offset, limit, bytes, extra, use_cesu);
                if (bytes)
                {
                    ++points;
                    offset += bytes;
                    limit -= bytes;
                    bytes = 0;
                }
            }
        }
        text.offset = offset;
    }
    return points;
}

uint32_t backUTF16(utf_text& text, const uint32_t count, const bool le, const bool use_ucs2) noexcept
{
    uint32_t points = 0;
    if (count && get_errors(text, 1).no_error())
    {
        uint32_t limit = text.offset;
        if (use_ucs2)
        {
            points = (limit >> 1);
            if (points > count)
            {
                points = count;
            }
            limit -= (points << 1);
        }
        else
        {
            const uint8_t* buffer = &text.buffer[text.offset];
            bool pairing = false;
            while ((points < count) && (limit >= 2))
            {
                ++points;
                limit -= 2;
                buffer -= 2;
                unicode_t unicode = (le ? ((static_cast<unicode_t>(buffer[1]) << 8) + buffer[0]) : ((static_cast<unicode_t>(buffer[0]) << 8) + buffer[1]));
                if ((unicode & 0xfffff800u) == 0x0000d800u)
                {   //  found a surrogate
                    if ((unicode & 0x00000400u) != 0)
                    {   //  high surrogate
                        pairing = true;
                    }
                    else if (pairing)
                    {   //  valid surrogate pair
                        --points;
                        pairing = false;
                    }
                }
                else
                {
                    pairing = false;
                }
            }
        }
        text.offset = limit;
    }
    return points;
}

uint32_t stepUTF16(utf_text& text, const uint32_t count, const bool le, const bool use_ucs2) noexcept
{
    uint32_t points = 0;
    if (count && get_errors(text, 1).no_error())
    {
        uint32_t limit = (text.length - text.offset);
        if (use_ucs2)
        {
            points = (limit >> 1);
            if (points > count)
            {
                points = count;
            }
            limit -= (points << 1);
        }
        else
        {
            const uint8_t* buffer = &text.buffer[text.offset];
            bool pairing = false;
            while ((points < count) && (limit >= 2))
            {
                ++points;
                limit -= 2;
                unicode_t unicode = (le ? ((static_cast<unicode_t>(buffer[1]) << 8) + buffer[0]) : ((static_cast<unicode_t>(buffer[0]) << 8) + buffer[1]));
                if ((unicode & 0xfffff800u) == 0x0000d800u)
                {   //  found a surrogate
                    if ((unicode & 0x00000400u) == 0)
                    {   //  low surrogate
                        pairing = true;
                    }
                    else if (pairing)
                    {   //  valid surrogate pair
                        --points;
                        pairing = false;
                    }
                }
                else
                {
                    pairing = false;
                }
                buffer += 2;
            }
        }
        text.offset = (text.length - limit);
    }
    return points;
}

uint32_t backUTF32(utf_text& text, const uint32_t count, const bool le, const bool use_cesu) noexcept
{
    uint32_t points = 0;
    if (count && get_errors(text, 3).no_error())
    {
        uint32_t limit = text.offset;
        if (use_cesu)
        {
            const uint8_t* buffer = &text.buffer[text.offset];
            bool pairing = false;
            while ((points < count) && (limit >= 4))
            {
                ++points;
                limit -= 4;
                buffer -= 4;
                unicode_t unicode = (le ?
                    ((((((static_cast<unicode_t>(buffer[3]) << 8) + buffer[2]) << 8) + buffer[1]) << 8) + buffer[0]) :
                    ((((((static_cast<unicode_t>(buffer[0]) << 8) + buffer[1]) << 8) + buffer[2]) << 8) + buffer[3]));
                if ((unicode & 0xfffff800u) == 0x0000d800u)
                {   //  found a surrogate
                    if ((unicode & 0x00000400u) != 0)
                    {   //  high surrogate
                        pairing = true;
                    }
                    else if (pairing)
                    {   //  valid surrogate pair
                        --points;
                        pairing = false;
                    }
                }
                else
                {
                    pairing = false;
                }
            }
        }
        else
        {
            points = (limit >> 2);
            if (points > count)
            {
                points = count;
            }
            limit -= (points << 2);
        }
        text.offset = limit;
    }
    return points;
}

uint32_t stepUTF32(utf_text& text, const uint32_t count, const bool le, const bool use_cesu) noexcept
{
    uint32_t points = 0;
    if (count && get_errors(text, 3).no_error())
    {
        uint32_t limit = (text.length - text.offset);
        if (use_cesu)
        {
            const uint8_t* buffer = &text.buffer[text.offset];
            bool pairing = false;
            while ((points < count) && (limit >= 4))
            {
                ++points;
                limit -= 4;
                unicode_t unicode = (le ?
                    ((((((static_cast<unicode_t>(buffer[3]) << 8) + buffer[2]) << 8) + buffer[1]) << 8) + buffer[0]) :
                    ((((((static_cast<unicode_t>(buffer[0]) << 8) + buffer[1]) << 8) + buffer[2]) << 8) + buffer[3]));
                if ((unicode & 0xfffff800u) == 0x0000d800u)
                {   //  found a surrogate
                    if ((unicode & 0x00000400u) == 0)
                    {   //  low surrogate
                        pairing = true;
                    }
                    else if (pairing)
                    {   //  valid surrogate pair
                        --points;
                        pairing = false;
                    }
                }
                else
                {
                    pairing = false;
                }
                buffer += 4;
            }
        }
        else
        {
            points = (limit >> 2);
            if (points > count)
            {
                points = count;
            }
            limit -= (points << 2);
        }
        text.offset = (text.length - limit);
    }
    return points;
}

uint32_t backCP1252(utf_text& text, const uint32_t count, const bool strict, const bool coalesce) noexcept
{
    uint32_t points = 0;
    if (count && get_errors(text).no_error())
    {
        uint32_t limit = text.offset;
        if (coalesce)
        {
            CP1252Strictness strictness = strict ? CP1252Strictness::StrictUndefined : CP1252Strictness::WindowsCompatible;
            const uint8_t* buffer = &text.buffer[text.offset];
            uint8_t cp1252;
            unicode_t unicode;
            bool valid = true;
            while ((points < count) && (limit > 0))
            {
                --limit;
                --buffer;
                cp1252 = buffer[0];
                if (cp1252ToUnicode(cp1252, unicode, strictness))
                {
                    ++points;
                    valid = true;
                }
                else if (valid)
                {
                    ++points;
                    valid = false;
                }
            }
            text.offset = limit;
        }
        else
        {
            points = ((count <= limit) ? count : limit);
            text.offset -= points;
        }
    }
    return points;
}

uint32_t stepCP1252(utf_text& text, const uint32_t count, const bool strict, const bool coalesce) noexcept
{
    uint32_t points = 0;
    if (count && get_errors(text).no_error())
    {
        uint32_t limit = (text.length - text.offset);
        if (coalesce)
        {
            CP1252Strictness strictness = strict ? CP1252Strictness::StrictUndefined : CP1252Strictness::WindowsCompatible;
            const uint8_t* buffer = &text.buffer[text.offset];
            uint8_t cp1252;
            unicode_t unicode;
            bool valid = true;
            while ((points < count) && (limit > 0))
            {
                --limit;
                cp1252 = buffer[0];
                if (cp1252ToUnicode(cp1252, unicode, strictness))
                {
                    ++points;
                    valid = true;
                }
                else if (valid)
                {
                    ++points;
                    valid = false;
                }
                ++buffer;
            }
            text.offset = (text.length - limit);
        }
        else
        {
            points = ((count <= limit) ? count : limit);
            text.offset += points;
        }
    }
    return points;
}

// ==== UTF8 overlong encoding index functions ====

//  Notes:
//
//      These functions return false if the encoding or index is invalid.
//
//      There are 0x04210880 overlong code-points which allows a maximum encodable overlong index of 0x0421087f.
//
//      Index 0 corresponds to the Java style 2-byte NULL encoding, but the remaining indices are potentially
//      available for other private signalling.
//
//      At the time of writing, other than the Java style NULL, no standards exist for using overlong encoding
//      and strict decoders should ignore all overlong forms or treat them as decode failures.

[[nodiscard]] bool overlongToIndexUTF8(const unicode_t unicode, const uint32_t bytes, uint32_t& index) noexcept
{
    if (unicode >= 0x00000000u)
    {
        if (bytes == 2)
        {
            if (unicode < 0x00000080u)
            {
                index = static_cast<uint32_t>(unicode);
                return true;
            }
        }
        else if (bytes == 3)
        {
            if (unicode < 0x00000800u)
            {
                index = static_cast<uint32_t>(unicode + 0x00000080u);
                return true;
            }
        }
        else if (bytes == 4)
        {
            if (unicode < 0x00010000u)
            {
                index = static_cast<uint32_t>(unicode + 0x00000880u);
                return true;
            }
        }
        else if (bytes == 5)
        {
            if (unicode < 0x00200000u)
            {
                index = static_cast<uint32_t>(unicode + 0x00010880u);
                return true;
            }
        }
        else if (bytes == 6)
        {
            if (unicode < 0x04000000u)
            {
                index = static_cast<uint32_t>(unicode + 0x00210880u);
                return true;
            }
        }
    }
    index = 0;
    return false;
}

[[nodiscard]] bool indexToOverlongUTF8(const uint32_t index, unicode_t& unicode, uint32_t& bytes) noexcept
{
    if (index < 0x00000080u)
    {
        unicode = static_cast<unicode_t>(index);
        bytes = 2;
        return true;
    }
    else if (index < 0x00000880u)
    {
        unicode = static_cast<unicode_t>(index - 0x00000080u);
        bytes = 3;
        return true;
    }
    else if (index < 0x00010880u)
    {
        unicode = static_cast<unicode_t>(index - 0x00000880u);
        bytes = 4;
        return true;
    }
    else if (index < 0x00210880u)
    {
        unicode = static_cast<unicode_t>(index - 0x00010880u);
        bytes = 5;
        return true;
    }
    else if (index < 0x04210880u)
    {
        unicode = static_cast<unicode_t>(index - 0x00210880u);
        bytes = 6;
        return true;
    }
    unicode = 0;
    bytes = 0;
    return false;
}

// ==== encoded unicode code-point handling functions abstraction interface utility functions ====

[[nodiscard]] cp_errors IUTFTK::read(utf_text& text, unicode_t& unicode) const noexcept
{
    uint32_t bytes = 0;
    cp_errors errors = get(text, unicode, bytes);
    text.offset += bytes;
    return errors;
}

[[nodiscard]] cp_errors IUTFTK::write(utf_text& text, const unicode_t unicode) const noexcept
{
    uint32_t bytes = 0;
    cp_errors errors = set(text, unicode, bytes);
    text.offset += bytes;
    return errors;
}

[[nodiscard]] cp_errors IUTFTK::writeBOM(utf_text& text) const noexcept
{
    uint32_t bytes = 0;
    cp_errors errors = setBOM(text, bytes);
    text.offset += bytes;
    return errors;
}

[[nodiscard]] cp_errors IUTFTK::writeNull(utf_text& text) const noexcept
{
    uint32_t bytes = 0;
    cp_errors errors = setNull(text, bytes);
    text.offset += bytes;
    return errors;
}

[[nodiscard]] cp_errors IUTFTK::validate(const utf_text& text) const noexcept
{   //  attempts to read the entire buffer accumulating warnings, fails immediately on any errors
    cp_errors errors = get_errors(text);
    if (errors.no_error())
    {
        utf_text scan = text;
        while (scan.offset < scan.length)
        {
            unicode_t unicode;
            errors |= read(scan, unicode);
            if (errors.error())
            {
                break;
            }
        }
    }
    return errors;
}

[[nodiscard]] cp_errors IUTFTK::getNLF(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept
{
    bytes = 0;
    cp_errors errors = get(text, unicode, bytes);
    if (errors.no_error())
    {
        switch (unicode)
        {
            case(0x000au): //  line-feed
            case(0x000du): //  carriage return
            {   //  possible { 0x0d, 0x0a } or { 0x0a, 0x0d } pairing
                utf_text next = text;
                next.offset += bytes;
                uint32_t extra = 0;
                unicode_t pairing = 0;
                cp_errors check = get(next, pairing, extra);
                if (get_errors(text).no_error() && (static_cast<uint32_t>(unicode) == (pairing ^ 0x0007u)))
                {   //  found a { 0x0d, 0x0a } or { 0x0a, 0x0d } pairing
                    bytes += extra;
                    errors |= check;
                }
                unicode = 0x000au;
                break;
            }
            case(0x000bu):  //  vertical tab
            case(0x000cu):  //  form-feed
            case(0x0085u):  //  next line
            case(0x2028u):  //  line separator
            case(0x2029u):  //  paragraph separator
            {
                unicode = 0x000au;
                break;
            }
            default:
            {
                break;
            }
        }
    }
    return errors;
}

[[nodiscard]] cp_errors IUTFTK::readNLF(utf_text& text, unicode_t& unicode) const noexcept
{
    uint32_t bytes = 0;
    cp_errors errors = getNLF(text, unicode, bytes);
    text.offset += bytes;
    return errors;
}

[[nodiscard]] cp_errors IUTFTK::getLine(const utf_text& text, utf_text& line, uint32_t& bytes) const noexcept
{
    bytes = 0;
    line.length = 0;
    line.offset = 0;
    line.buffer = nullptr;
    cp_errors errors = get_errors(text);
    if (errors.no_error())
    {
        utf_text scan;
        scan.length = (text.length - text.offset);
        scan.offset = 0;
        scan.buffer = &text.buffer[text.offset];
        unicode_t unicode;
        while ((errors |= getNLF(scan, unicode, bytes)).no_error())
        {
            if ((unicode == 0x000au) || (unicode == 0x0000u))
            {
                bytes += scan.offset;
                line.length = scan.offset;
                line.offset = 0;
                line.buffer = scan.buffer;
                break;
            }
            scan.offset += bytes;
        };
    }
    return errors;
}

[[nodiscard]] cp_errors IUTFTK::readLine(utf_text& text, utf_text& line) const noexcept
{
    uint32_t bytes = 0;
    cp_errors errors = getLine(text, line, bytes);
    text.offset += bytes;
    return errors;
}

// ==== concrete classes for encoded unicode code-point handling ====

struct CUTF_UTF8 : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::UTF8; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, false, false); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, false, false, false, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, false, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, false, false, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, false, false, true); }
};

struct CUTF_UTF8ns : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::UTF8ns; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, false, false); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, false, false, false, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, false, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, false, false, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, false, false, false); }
};

struct CUTF_UTF8st : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::UTF8st; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, false, false); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, false, false, true, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, false, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, false, true, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, false, true, false); }
};

struct CUTF_JUTF8 : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::JUTF8; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, false, true); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, false, true, false, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, false, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, false, false, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, false, false, true); }
};

struct CUTF_JUTF8ns : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::JUTF8ns; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, false, true); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, false, true, false, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, false, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, false, false, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, false, false, false); }
};

struct CUTF_JUTF8st : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::JUTF8st; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, false, true); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, false, true, true, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, false, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, false, true, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, false, true, false); }
};

struct CUTF_CESU8 : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::CESU8; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, true, false); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, true, false, false, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, true, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, true, false, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, true, false, true); }
};

struct CUTF_CESU8ns : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::CESU8ns; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, true, false); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, true, false, false, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, true, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, true, false, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, true, false, false); }
};

struct CUTF_CESU8st : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::CESU8st; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, true, false); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, true, false, true, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, true, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, true, true, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, true, true, false); }
};

struct CUTF_JCESU8 : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::JCESU8; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, true, true); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, true, true, false, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, true, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, true, false, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, true, false, true); }
};

struct CUTF_JCESU8ns : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::JCESU8ns; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, true, true); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, true, true, false, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, true, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, true, false, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, true, false, false); }
};

struct CUTF_JCESU8st : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::JCESU8st; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, true, true); }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF8(text, unicode, bytes, true, true, true, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF8(text, unicode, bytes, true, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF8(text, count, true, true, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF8(text, count, true, true, false); }
};

struct CUTF_UTF16le : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF16le; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::UTF16le; }
    virtual uint32_t                unitSize(void) const noexcept { return 2; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF16(unicode, false); }
    virtual uint32_t                lenBOM() const noexcept { return 2; }
    virtual uint32_t                lenNull() const noexcept { return 2; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF16(text, unicode, bytes, true, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF16(text, unicode, bytes, true, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF16_BOM(text, bytes, true); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF16_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF16(text, count, true, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF16(text, count, true, false); }
};

struct CUTF_UTF16be : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF16be; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::UTF16be; }
    virtual uint32_t                unitSize(void) const noexcept { return 2; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF16(unicode, false); }
    virtual uint32_t                lenBOM() const noexcept { return 2; }
    virtual uint32_t                lenNull() const noexcept { return 2; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF16(text, unicode, bytes, false, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF16(text, unicode, bytes, false, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF16_BOM(text, bytes, false); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF16_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF16(text, count, false, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF16(text, count, false, false); }
};

struct CUTF_UCS2le : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF16le; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::UCS2le; }
    virtual uint32_t                unitSize(void) const noexcept { return 2; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF16(unicode, true); }
    virtual uint32_t                lenBOM() const noexcept { return 2; }
    virtual uint32_t                lenNull() const noexcept { return 2; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF16(text, unicode, bytes, true, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF16(text, unicode, bytes, true, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF16_BOM(text, bytes, true); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF16_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF16(text, count, true, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF16(text, count, true, true); }
};

struct CUTF_UCS2be : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF16be; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::UCS2be; }
    virtual uint32_t                unitSize(void) const noexcept { return 2; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF16(unicode, true); }
    virtual uint32_t                lenBOM() const noexcept { return 2; }
    virtual uint32_t                lenNull() const noexcept { return 2; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF16(text, unicode, bytes, false, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF16(text, unicode, bytes, false, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF16_BOM(text, bytes, false); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF16_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF16(text, count, false, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF16(text, count, false, true); }
};

struct CUTF_UTF32le : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF32le; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::UTF32le; }
    virtual uint32_t                unitSize(void) const noexcept { return 4; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF32(unicode, false, false); }
    virtual uint32_t                lenBOM() const noexcept { return 4; }
    virtual uint32_t                lenNull() const noexcept { return 4; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF32(text, unicode, bytes, true, false, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF32(text, unicode, bytes, true, false, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_BOM(text, bytes, true); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF32(text, count, true, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF32(text, count, true, false); }
};

struct CUTF_UTF32be : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF32be; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::UTF32be; }
    virtual uint32_t                unitSize(void) const noexcept { return 4; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF32(unicode, false, false); }
    virtual uint32_t                lenBOM() const noexcept { return 4; }
    virtual uint32_t                lenNull() const noexcept { return 4; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF32(text, unicode, bytes, false, false, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF32(text, unicode, bytes, false, false, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_BOM(text, bytes, false); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF32(text, count, false, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF32(text, count, false, false); }
};

struct CUTF_UCS4le : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF32le; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::UCS4le; }
    virtual uint32_t                unitSize(void) const noexcept { return 4; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF32(unicode, false, true); }
    virtual uint32_t                lenBOM() const noexcept { return 4; }
    virtual uint32_t                lenNull() const noexcept { return 4; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF32(text, unicode, bytes, true, false, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF32(text, unicode, bytes, true, false, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_BOM(text, bytes, true); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF32(text, count, true, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF32(text, count, true, false); }
};

struct CUTF_UCS4be : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF32be; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::UCS4be; }
    virtual uint32_t                unitSize(void) const noexcept { return 4; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF32(unicode, false, true); }
    virtual uint32_t                lenBOM() const noexcept { return 4; }
    virtual uint32_t                lenNull() const noexcept { return 4; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF32(text, unicode, bytes, false, false, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF32(text, unicode, bytes, false, false, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_BOM(text, bytes, false); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF32(text, count, false, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF32(text, count, false, false); }
};

struct CUTF_CESU32le : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF32le; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::CESU32le; }
    virtual uint32_t                unitSize(void) const noexcept { return 4; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF32(unicode, true, false); }
    virtual uint32_t                lenBOM() const noexcept { return 4; }
    virtual uint32_t                lenNull() const noexcept { return 4; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF32(text, unicode, bytes, true, true, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF32(text, unicode, bytes, true, true, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_BOM(text, bytes, true); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF32(text, count, true, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF32(text, count, true, true); }
};

struct CUTF_CESU32be : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF32be; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::CESU32be; }
    virtual uint32_t                unitSize(void) const noexcept { return 4; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF32(unicode, true, false); }
    virtual uint32_t                lenBOM() const noexcept { return 4; }
    virtual uint32_t                lenNull() const noexcept { return 4; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF32(text, unicode, bytes, false, true, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF32(text, unicode, bytes, false, true, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_BOM(text, bytes, false); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF32(text, count, false, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF32(text, count, false, true); }
};

struct CUTF_CESU4le : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF32le; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::CESU32le; }
    virtual uint32_t                unitSize(void) const noexcept { return 4; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF32(unicode, true, true); }
    virtual uint32_t                lenBOM() const noexcept { return 4; }
    virtual uint32_t                lenNull() const noexcept { return 4; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF32(text, unicode, bytes, true, true, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF32(text, unicode, bytes, true, true, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_BOM(text, bytes, true); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF32(text, count, true, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF32(text, count, true, true); }
};

struct CUTF_CESU4be : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::UTF32be; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::CESU32be; }
    virtual uint32_t                unitSize(void) const noexcept { return 4; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return lenUTF32(unicode, true, true); }
    virtual uint32_t                lenBOM() const noexcept { return 4; }
    virtual uint32_t                lenNull() const noexcept { return 4; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeUTF32(text, unicode, bytes, false, true, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeUTF32(text, unicode, bytes, false, true, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_BOM(text, bytes, false); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF32_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backUTF32(text, count, false, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepUTF32(text, count, false, true); }
};

struct CUTF_BYTE : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::OTHER; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::BYTE; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return (static_cast<unicode_t>(unicode & 0x000000ffu) == unicode) ? 1 : 0; }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeBYTE(text, unicode, bytes, false, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeBYTE(text, unicode, bytes, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backBYTE(text, count, false, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepBYTE(text, count, false, true); }
};

struct CUTF_BYTEns : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::OTHER; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::BYTEns; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return (static_cast<unicode_t>(unicode & 0x000000ffu) == unicode) ? 1 : 0; }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeBYTE(text, unicode, bytes, false, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeBYTE(text, unicode, bytes, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backBYTE(text, count, false, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepBYTE(text, count, false, false); }
};

struct CUTF_ASCII : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::OTHER; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::ASCII; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return (static_cast<unicode_t>(unicode & 0x0000007fu) == unicode) ? 1 : 0; }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeBYTE(text, unicode, bytes, true, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeBYTE(text, unicode, bytes, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backBYTE(text, count, true, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepBYTE(text, count, true, true); }
};

struct CUTF_ASCIIns : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::OTHER; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::ASCIIns; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return (static_cast<unicode_t>(unicode & 0x0000007fu) == unicode) ? 1 : 0; }
    virtual uint32_t                lenBOM() const noexcept { return 3; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeBYTE(text, unicode, bytes, true, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeBYTE(text, unicode, bytes, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_BOM(text, bytes); }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backBYTE(text, count, true, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepBYTE(text, count, true, false); }
};

struct CUTF_CP1252 : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::OTHER; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::CP1252; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return (static_cast<unicode_t>(unicode & 0x0000007fu) == unicode) ? 1 : 0; }
    virtual uint32_t                lenBOM() const noexcept { return 0; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeCP1252(text, unicode, bytes, false, true); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeCP1252(text, unicode, bytes, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { (void)text; bytes = 0; return cp_errors::bits::None; }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backCP1252(text, count, false, true); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepCP1252(text, count, false, true); }
};

struct CUTF_CP1252ns : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::OTHER; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::CP1252ns; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return (static_cast<unicode_t>(unicode & 0x0000007fu) == unicode) ? 1 : 0; }
    virtual uint32_t                lenBOM() const noexcept { return 0; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeCP1252(text, unicode, bytes, false, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeCP1252(text, unicode, bytes, false); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { (void)text; bytes = 0; return cp_errors::bits::None; }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backCP1252(text, count, false, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepCP1252(text, count, false, false); }
};

struct CUTF_CP1252st : public IUTFTK
{
    virtual UTF_TYPE                utfType(void) const noexcept { return UTF_TYPE::OTHER; }
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept { return UTF_SUB_TYPE::CP1252st; }
    virtual uint32_t                unitSize(void) const noexcept { return 1; }
    virtual uint32_t                len(const unicode_t unicode) const noexcept { return (static_cast<unicode_t>(unicode & 0x0000007fu) == unicode) ? 1 : 0; }
    virtual uint32_t                lenBOM() const noexcept { return 0; }
    virtual uint32_t                lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept { return decodeCP1252(text, unicode, bytes, true, false); }
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept { return encodeCP1252(text, unicode, bytes, true); }
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept { (void)text; bytes = 0; return cp_errors::bits::None; }
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept { return encodeUTF8_NULL(text, bytes); }
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept { return backCP1252(text, count, true, false); }
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept { return stepCP1252(text, count, true, false); }
};

// ==== encoded unicode code-point handler request functions ====

const IUTFTK& IUTFTK::getHandler(const UTF_TYPE utfType) noexcept
{
    static const UTF_SUB_TYPE subTypes[static_cast<uint32_t>(UTF_TYPE::COUNT)] = { UTF_SUB_TYPE::UTF8st, UTF_SUB_TYPE::UTF16le, UTF_SUB_TYPE::UTF32le, UTF_SUB_TYPE::UTF32be, UTF_SUB_TYPE::JUTF8st };
    uint32_t index = static_cast<uint32_t>(utfType);
    return getHandler(index < static_cast<uint32_t>(UTF_TYPE::COUNT) ? subTypes[index] : UTF_SUB_TYPE::JUTF8st);
}

const IUTFTK& IUTFTK::getHandler(const UTF_SUB_TYPE utfSubType) noexcept
{
    switch (utfSubType)
    {
        case(UTF_SUB_TYPE::UTF8):       { static const CUTF_UTF8     handler; return handler; }
        case(UTF_SUB_TYPE::UTF8ns):     { static const CUTF_UTF8ns   handler; return handler; }
        case(UTF_SUB_TYPE::UTF8st):     { static const CUTF_UTF8st   handler; return handler; }
        case(UTF_SUB_TYPE::JUTF8):      { static const CUTF_JUTF8    handler; return handler; }
        case(UTF_SUB_TYPE::JUTF8ns):    { static const CUTF_JUTF8ns  handler; return handler; }
        case(UTF_SUB_TYPE::JUTF8st):    { static const CUTF_JUTF8st  handler; return handler; }
        case(UTF_SUB_TYPE::CESU8):      { static const CUTF_CESU8    handler; return handler; }
        case(UTF_SUB_TYPE::CESU8ns):    { static const CUTF_CESU8ns  handler; return handler; }
        case(UTF_SUB_TYPE::CESU8st):    { static const CUTF_CESU8st  handler; return handler; }
        case(UTF_SUB_TYPE::JCESU8):     { static const CUTF_JCESU8   handler; return handler; }
        case(UTF_SUB_TYPE::JCESU8ns):   { static const CUTF_JCESU8ns handler; return handler; }
        case(UTF_SUB_TYPE::JCESU8st):   { static const CUTF_JCESU8st handler; return handler; }
        case(UTF_SUB_TYPE::UTF16le):    { static const CUTF_UTF16le  handler; return handler; }
        case(UTF_SUB_TYPE::UTF16be):    { static const CUTF_UTF16be  handler; return handler; }
        case(UTF_SUB_TYPE::UCS2le):     { static const CUTF_UCS2le   handler; return handler; }
        case(UTF_SUB_TYPE::UCS2be):     { static const CUTF_UCS2be   handler; return handler; }
        case(UTF_SUB_TYPE::UTF32le):    { static const CUTF_UTF32le  handler; return handler; }
        case(UTF_SUB_TYPE::UTF32be):    { static const CUTF_UTF32be  handler; return handler; }
        case(UTF_SUB_TYPE::UCS4le):     { static const CUTF_UCS4le   handler; return handler; }
        case(UTF_SUB_TYPE::UCS4be):     { static const CUTF_UCS4be   handler; return handler; }
        case(UTF_SUB_TYPE::CESU32le):   { static const CUTF_CESU32le handler; return handler; }
        case(UTF_SUB_TYPE::CESU32be):   { static const CUTF_CESU32be handler; return handler; }
        case(UTF_SUB_TYPE::CESU4le):    { static const CUTF_CESU4le  handler; return handler; }
        case(UTF_SUB_TYPE::CESU4be):    { static const CUTF_CESU4be  handler; return handler; }
        case(UTF_SUB_TYPE::BYTE):       { static const CUTF_BYTE     handler; return handler; }
        case(UTF_SUB_TYPE::BYTEns):     { static const CUTF_BYTEns   handler; return handler; }
        case(UTF_SUB_TYPE::ASCII):      { static const CUTF_ASCII    handler; return handler; }
        case(UTF_SUB_TYPE::ASCIIns):    { static const CUTF_ASCIIns  handler; return handler; }
        case(UTF_SUB_TYPE::CP1252):     { static const CUTF_CP1252   handler; return handler; }
        case(UTF_SUB_TYPE::CP1252ns):   { static const CUTF_CP1252ns handler; return handler; }
        case(UTF_SUB_TYPE::CP1252st):   { static const CUTF_CP1252st handler; return handler; }
        default:                        { static const CUTF_JUTF8st  handler; return handler; }
    }
}

const IUTFTK& IUTFTK::getHandler(const UTF_OTHER_TYPE utfOtherType) noexcept
{
    static const UTF_SUB_TYPE subTypes[static_cast<uint32_t>(UTF_OTHER_TYPE::COUNT)] = { UTF_SUB_TYPE::JUTF8st, UTF_SUB_TYPE::BYTEns, UTF_SUB_TYPE::ASCIIns, UTF_SUB_TYPE::CP1252st };
    uint32_t index = static_cast<uint32_t>(utfOtherType);
    return getHandler(index < static_cast<uint32_t>(UTF_OTHER_TYPE::COUNT) ? subTypes[index] : UTF_SUB_TYPE::JUTF8st);
}

};  //  namespace toolkit

};  //  namespace utf

};  //  namespace unicode
