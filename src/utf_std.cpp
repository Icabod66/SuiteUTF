
//  LibUTF
//  Original design 2010–2016; maintained and extended 2024–2025.
//  Copyright (c) 2010–2025 Ritchie Brannan.
//  MIT License. See LICENSE.txt. Project history: docs/History.md.
//
//  File:   utf_std.cpp
//  Author: Ritchie Brannan
//  Date:   11 July 16
//  
//  Description:
//  
//      Simple UTF code-point handling.
//  
//  Important:
//  
//      strlenUTF8() will only report the correct count of code-points for well formed UTF8 (including Java style).
//  
//  Notes:
//  
//      These functions should be sufficient if you don't need fine control over the encoding and decoding.
//  
//      The functions only encode and decode strictly compliant UTF (with an option for Java style UTF8).
//      If the value is not encodable or not decodable or not compliant the functions return false.
//      If the value is not decodable or not compliant the bytes value will be the code-unit size in bytes (1, 2 or 4).
//      If the buffer is NULL or not large enough to decode, the unicode value will be 0.
//      If a UTF8 or OTHER decode fails or is non-compliant, the unicode value will be 0x800000XX where XX is the lead byte.
//      If a UTF16 or UTF32 decode fails or is non-compliant, the unicode value will be 0x80000000.
//      At the end of the buffer the bytes value will be 0.
//  
//  Usage notes:
//  
//      The use_java flag enables Java style UTF8 with a 2-byte encoding of the NULL code-point.
// 
//      To get a Java style UTF8 handler, call:
//          IUTF::getHandler() or
//          IUTF::getHandler(UTF_TYPE::OTHER) or
//          IUTF::getHandlerOther() or
//          IUTF::getHandlerOther(UTF_OTHER_TYPE::JUTF8).
// 
//      To get an ISO8859-1 (8-bit Unicode) handler, call:
//          IUTF::getHandlerOther(UTF_OTHER_TYPE::ISO8859_1).
// 
//      To get a strict Ascii handler, call:
//          IUTF::getHandlerOther(UTF_OTHER_TYPE::Ascii).
// 
//      To get a Windows Code-Page 1252 handler, call:
//          IUTF::getHandlerOther(UTF_OTHER_TYPE::CP1252).

#include "utf_std.h"
#include "unicode_utilities.h"
#include <string.h>

namespace unicode
{

namespace utf
{

/// UTF encoding identification function
/// 
///     This function attempts to identify UTF text encodings by the presence of a byte-order-marker or 2 leading ASCII
///     characters in the range 0x01 to 0x7f.
/// 
///     The 'bytes' output parameter indicates the number of bytes of BOM encountered.
/// 
UTF_TYPE identifyUTF(const uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept
{
    bytes = 0;
    if (buffer != nullptr)
    {
        if (size >= 2)
        {   //  possible UTF8, UTF16 or utf32
            if (size >= 3)
            {   //  possible UTF8 or UTF32
                if (size >= 4)
                {   //  possible UTF32
                    if ((buffer[0] == 0xffu) && (buffer[1] == 0xfeu) && (buffer[2] == 0x00u) && (buffer[3] == 0x00u))
                    {   //  found utf32 little-endian BOM
                        bytes = 4;
                        return UTF_TYPE::UTF32le;
                    }
                    if ((buffer[0] == 0x00u) && (buffer[1] == 0x00u) && (buffer[2] == 0xfeu) && (buffer[3] == 0xffu))
                    {   //  found UTF32 big-endian BOM
                        bytes = 4;
                        return UTF_TYPE::UTF32be;
                    }
                }
                if ((buffer[0] == 0xefu) && (buffer[1] == 0xbbu) && (buffer[2] == 0xbfu))
                {   //  found UTF8 BOM
                    bytes = 3;
                    return UTF_TYPE::UTF8;
                }
            }
            if ((buffer[0] == 0xffu) && (buffer[1] == 0xfeu))
            {   //  found UTF16 little-endian BOM
                bytes = 2;
                return UTF_TYPE::UTF16le;
            }
            if ((buffer[0] == 0xfeu) && (buffer[1] == 0xffu))
            {   //  found UTF16 big-endian BOM
                bytes = 2;
                return UTF_TYPE::UTF16be;
            }
        }
        const char* const string = reinterpret_cast<const char* const>(buffer);
        if (size >= 2)
        {   //  possible UTF8, UTF16 or UTF32
            if (size >= 4)
            {   //  possible UTF16 or UTF32
                if (size >= 8)
                {   //  possible UTF32
                    if ((string[1] == 0) && (string[2] == 0) && (string[5] == 0) && (string[6] == 0))
                    {   //  possible 2 UTF32 ASCII characters
                        if ((string[3] == 0) && (string[7] == 0) && (string[0] > 0) && (string[4] > 0))
                        {   //  found 2 UTF32 little-endian ascii characters
                            return UTF_TYPE::UTF32le;
                        }
                        if ((string[0] == 0) && (string[4] == 0) && (string[3] > 0) && (string[7] > 0))
                        {   //  found 2 UTF32 big-endian ascii characters
                            return UTF_TYPE::UTF32be;
                        }
                    }
                }
                if ((string[1] == 0) && (string[3] == 0) && (string[0] > 0) && (string[2] > 0))
                {   //  found 2 UTF16 little-endian ASCII characters
                    return UTF_TYPE::UTF16le;
                }
                if ((string[0] == 0) && (string[2] == 0) && (string[1] > 0) && (string[3] > 0))
                {   //  found 2 UTF16 big-endian ASCII characters
                    return UTF_TYPE::UTF16be;
                }
            }
            if ((string[0] > 0) && (string[1] > 0))
            {   //  found 2 UTF8 ASCII characters
                return UTF_TYPE::UTF8;
            }
        }
    }
    return UTF_TYPE::OTHER;
}

namespace std
{


// ==== quick Windows Code-Page 1252 encoded code-point length function ====

uint32_t lenCP1252(const unicode_t unicode) noexcept
{
    uint8_t cp1252;
    return unicodeToCP1252(unicode, cp1252, CP1252Strictness::StrictUndefined) ? 1 : 0;
}

// ==== quick Windows Code-Page 1252 encode and decode functions ====

[[nodiscard]] bool getCP1252(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) noexcept
{
    bytes = 0;
    unicode = 0;
    if ((buffer != nullptr) && (size >= 1))
    {
        bytes = 1;
        uint8_t cp1252 = buffer[0];
        if (cp1252ToUnicode(cp1252, unicode, CP1252Strictness::StrictUndefined))
        {
            return true;
        }
        unicode = (0x80000000u + cp1252);
    }
    return false;
}

[[nodiscard]] bool setCP1252(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) noexcept
{
    if ((buffer != nullptr) && (size >= 1) && (static_cast<uint32_t>(unicode) <= 0x000000ffu))
    {
        uint8_t cp1252;
        if (unicodeToCP1252(unicode, cp1252, CP1252Strictness::StrictUndefined))
        {
            buffer[0] = cp1252;
            bytes = 1;
            return true;
        }
    }
    bytes = 0;
    return false;
}

// ==== quick UTF encode and decode functions ====

[[nodiscard]] bool getBYTE(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes, const bool use_ascii) noexcept
{
    bytes = 0;
    unicode = 0;
    if ((buffer != nullptr) && (size >= 1))
    {
        bytes = 1;
        uint8_t byte = buffer[0];
        if (!use_ascii || (byte <= 0x7fu))
        {
            unicode = static_cast<unicode_t>(buffer[0]);
            return true;
        }
        unicode = (0x80000000u + byte);
    }
    return false;
}

[[nodiscard]] bool setBYTE(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes, const bool use_ascii) noexcept
{
    if ((buffer != nullptr) && (size >= 1) && (static_cast<uint32_t>(unicode) <= (use_ascii ? 0x0000007fu : 0x000000ffu)))
    {
        buffer[0] = static_cast<uint8_t>(unicode);
        bytes = 1;
        return true;
    }
    bytes = 0;
    return false;
}

[[nodiscard]] bool getUTF8(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes, const bool use_java) noexcept
{
    bytes = 0;
    unicode = 0;
    if ((buffer != nullptr) && (size >= 1))
    {
        uint8_t byte = buffer[0];
        bytes = 1;
        unicode = byte;
        if (static_cast<uint8_t>(byte + 8) <= 0xc7u)
        {   //  1 byte (7 bits: 0x00-0x7f) or unexpected continuation byte (0x80-0xbf) or illegal leading byte (0xf8-0xff)
            if (byte <= 0x7fu)
            {
                return true;
            }
        }
        else if (byte <= 0xdfu)
        {   //  2 bytes (11 bits: 0xc0-0xdf)
            if (size >= 2)
            {
                unicode_t value = (unicode & 0x1fu);
                byte = buffer[1];
                if ((byte & 0xc0u) == 0x80u)
                {
                    value = ((value << 6) + (byte & 0x3fu));
                    if ((value >= 0x00000080u) || (use_java && (value == 0x00000000u)))
                    {
                        bytes = 2;
                        unicode = value;
                        return true;
                    }
                }
            }
        }
        else if (byte <= 0xefu)
        {   //  3 bytes (16 bits: 0xe0-0xef)
            if (size >= 3)
            {
                unicode_t value = (unicode & 0x0fu);
                byte = buffer[1];
                if ((byte & 0xc0u) == 0x80u)
                {
                    value = ((value << 6) + (byte & 0x3fu));
                    byte = buffer[2];
                    if ((byte & 0xc0u) == 0x80u)
                    {
                        value = ((value << 6) + (byte & 0x3fu));
                        if ((value >= 0x00000800u) && ((value & 0xfffff800u) != 0x0000d800u))
                        {
                            bytes = 3;
                            unicode = value;
                            return true;
                        }
                    }
                }
            }
        }
        else
        {   //  4 bytes (21 bits: 0xf0-0xf7)
            if (size >= 4)
            {
                unicode_t value = (unicode & 0x07u);
                byte = buffer[1];
                if ((byte & 0xc0u) == 0x80u)
                {
                    value = ((value << 6) + (byte & 0x3fu));
                    byte = buffer[2];
                    if ((byte & 0xc0u) == 0x80u)
                    {
                        value = ((value << 6) + (byte & 0x3fu));
                        byte = buffer[3];
                        if ((byte & 0xc0u) == 0x80u)
                        {
                            value = ((value << 6) + (byte & 0x3fu));
                            if ((value >= 0x00010000u) && (value <= 0x0010ffffu))
                            {
                                bytes = 4;
                                unicode = value;
                                return true;
                            }
                        }
                    }
                }
            }
        }
        unicode |= 0x80000000u;
    }
    return false;
}

[[nodiscard]] bool setUTF8(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes, const bool use_java) noexcept
{
    bytes = 0;
    if ((buffer != nullptr) && (size >= 1) && (static_cast<uint32_t>(unicode) <= 0x0010ffffu))
    {
        if (unicode <= 0x0000007fu)
        {   //  1 byte (7 bits) or 2 bytes (11 bits for Java modified NULL)
            if (use_java && (unicode == 0))
            {
                if (size >= 2)
                {
                    buffer[0] = 0xc0u;
                    buffer[1] = 0x80u;
                    bytes = 2;
                    return true;
                }
            }
            else
            {
                if (size >= 1)
                {
                    buffer[0] = static_cast<uint8_t>(unicode);
                    bytes = 1;
                    return true;
                }
            }
        }
        else if (unicode <= 0x000007ffu)
        {   //  2 bytes (11 bits)
            if (size >= 2)
            {   //  buffer overflow
                buffer[0] = (static_cast<uint8_t>(unicode >> 6) | 0xc0u);
                buffer[1] = ((static_cast<uint8_t>(unicode) & 0x3fu) | 0x80u);
                bytes = 2;
                return true;
            }
        }
        else if (unicode <= 0x0000ffffu)
        {   //  3 bytes (16 bits)
            if ((size >= 3) && ((unicode & 0xfffff800u) != 0x0000d800u))
            {
                buffer[0] = (static_cast<uint8_t>(unicode >> 12) | 0xe0u);
                buffer[1] = ((static_cast<uint8_t>(unicode >> 6) & 0x3fu) | 0x80u);
                buffer[2] = ((static_cast<uint8_t>(unicode) & 0x3fu) | 0x80u);
                bytes = 3;
                return true;
            }
        }
        else
        {   //  4 bytes (21 bits)
            if (size >= 4)
            {
                buffer[0] = (static_cast<uint8_t>(unicode >> 18) | 0xf0u);
                buffer[1] = ((static_cast<uint8_t>(unicode >> 12) & 0x3fu) | 0x80u);
                buffer[2] = ((static_cast<uint8_t>(unicode >> 6) & 0x3fu) | 0x80u);
                buffer[3] = ((static_cast<uint8_t>(unicode) & 0x3fu) | 0x80u);
                bytes = 4;
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] bool getUTF16le(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) noexcept
{
    bytes = 0;
    unicode = 0;
    if ((buffer != nullptr) && (size >= 2))
    {
        bytes = 2;
        unicode_t value = ((static_cast<unicode_t>(buffer[1]) << 8) + buffer[0]);
        if ((value & 0xfffff800u) != 0x0000d800u)
        {
            unicode = value;
            return true;
        }
        else if ((size >= 4) && ((value & 0xfffffc00u) == 0x0000d800u))
        {
            unicode_t extra = ((static_cast<unicode_t>(buffer[3]) << 8) + buffer[2]);
            if ((extra & 0xfffffc00u) == 0x0000dc00u)
            {   //  found low surrogate (valid surrogate pair)
                unicode = (((value & 0x000003ffu) << 10) + (extra & 0x000003ffu) + 0x00010000u);
                bytes = 4;
                return true;
            }
        }
        unicode = 0x80000000u;
    }
    return false;
}

[[nodiscard]] bool setUTF16le(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) noexcept
{
    bytes = 0;
    if ((buffer != nullptr) && (size >= 2) && (static_cast<uint32_t>(unicode) <= 0x0010ffffu) && ((unicode & 0xfffff800u) != 0x0000d800u))
    {
        if (unicode <= 0x0000ffffu)
        {
            buffer[0] = static_cast<uint8_t>(unicode);
            buffer[1] = static_cast<uint8_t>(unicode >> 8);
            bytes = 2;
            return true;
        }
        else if (size >= 4)
        {
            unicode_t surrogate = (unicode - 0x00010000u);
            surrogate = ((((surrogate >> 10) | (surrogate << 16)) & 0x03ff03ffu) | 0xdc00d800u);
            buffer[0] = static_cast<uint8_t>(surrogate);
            buffer[1] = static_cast<uint8_t>(surrogate >> 8);
            buffer[2] = static_cast<uint8_t>(surrogate >> 16);
            buffer[3] = static_cast<uint8_t>(surrogate >> 24);
            bytes = 4;
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool getUTF16be(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) noexcept
{
    bytes = 0;
    unicode = 0;
    if ((buffer != nullptr) && (size >= 2))
    {
        bytes = 2;
        unicode_t value = ((static_cast<unicode_t>(buffer[0]) << 8) + buffer[1]);
        if ((value & 0xfffff800u) != 0x0000d800u)
        {
            unicode = value;
            return true;
        }
        else if ((size >= 4) && ((value & 0xfffffc00u) == 0x0000d800u))
        {
            unicode_t extra = ((static_cast<unicode_t>(buffer[2]) << 8) + buffer[3]);
            if ((extra & 0xfffffc00u) == 0x0000dc00u)
            {   //  found low surrogate (valid surrogate pair)
                unicode = (((value & 0x000003ffu) << 10) + (extra & 0x000003ffu) + 0x00010000u);
                bytes = 4;
                return true;
            }
        }
        unicode = 0x80000000u;
    }
    return false;
}

[[nodiscard]] bool setUTF16be(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) noexcept
{
    bytes = 0;
    if ((buffer != nullptr) && (size >= 2) && (static_cast<uint32_t>(unicode) <= 0x0010ffffu) && ((unicode & 0xfffff800u) != 0x0000d800u))
    {
        if (unicode <= 0x0000ffffu)
        {
            buffer[0] = static_cast<uint8_t>(unicode >> 8);
            buffer[1] = static_cast<uint8_t>(unicode);
            bytes = 2;
            return true;
        }
        else if (size >= 4)
        {
            unicode_t surrogate = (unicode - 0x00010000u);
            surrogate = ((((surrogate >> 10) | (surrogate << 16)) & 0x03ff03ffu) | 0xdc00d800u);
            buffer[0] = static_cast<uint8_t>(surrogate >> 8);
            buffer[1] = static_cast<uint8_t>(surrogate);
            buffer[2] = static_cast<uint8_t>(surrogate >> 24);
            buffer[3] = static_cast<uint8_t>(surrogate >> 16);
            bytes = 4;
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool getUTF32le(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) noexcept
{
    bytes = 0;
    unicode = 0;
    if ((buffer != nullptr) && (size >= 4))
    {
        bytes = 4;
        unicode_t value = ((((((static_cast<unicode_t>(buffer[3]) << 8) + buffer[2]) << 8) + buffer[1]) << 8) + buffer[0]);
        if ((static_cast<uint32_t>(value) <= 0x0010ffffu) && ((value & 0xfffff800u) != 0x0000d800u))
        {
            unicode = value;
            return true;
        }
        unicode = 0x80000000u;
    }
    return false;
}

[[nodiscard]] bool setUTF32le(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) noexcept
{
    bytes = 0;
    if ((buffer != nullptr) && (size >= 4) && (static_cast<uint32_t>(unicode) <= 0x0010ffffu) && ((unicode & 0xfffff800u) != 0x0000d800u))
    {
        buffer[0] = static_cast<uint8_t>(unicode);
        buffer[1] = static_cast<uint8_t>(unicode >> 8);
        buffer[2] = static_cast<uint8_t>(unicode >> 16);
        buffer[3] = static_cast<uint8_t>(unicode >> 24);
        bytes = 4;
        return true;
    }
    return false;
}

[[nodiscard]] bool getUTF32be(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) noexcept
{
    bytes = 0;
    unicode = 0;
    if ((buffer != nullptr) && (size >= 4))
    {
        bytes = 4;
        unicode_t value = ((((((static_cast<unicode_t>(buffer[0]) << 8) + buffer[1]) << 8) + buffer[2]) << 8) + buffer[3]);
        if ((static_cast<uint32_t>(value) <= 0x0010ffffu) && ((value & 0xfffff800u) != 0x0000d800u))
        {
            unicode = value;
            return true;
        }
        unicode = 0x80000000u;
    }
    return false;
}

[[nodiscard]] bool setUTF32be(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) noexcept
{
    bytes = 0;
    if ((buffer != nullptr) && (size >= 4) && (static_cast<uint32_t>(unicode) <= 0x0010ffffu) && ((unicode & 0xfffff800u) != 0x0000d800u))
    {
        buffer[0] = static_cast<uint8_t>(unicode >> 24);
        buffer[1] = static_cast<uint8_t>(unicode >> 16);
        buffer[2] = static_cast<uint8_t>(unicode >> 8);
        buffer[3] = static_cast<uint8_t>(unicode);
        bytes = 4;
        return true;
    }
    return false;
}

// ==== quick UTF null (0) terminated string byte length functions ====

uint32_t strsizeUTF8(const uint8_t* const buffer) noexcept
{
    uint32_t index = 0;
    if (buffer != nullptr)
    {
        while (buffer[index])
        {
            ++index;
        }
    }
    return index;
}

uint32_t strsizeUTF16(const uint8_t* const buffer) noexcept
{
    uint32_t index = 0;
    if (buffer != nullptr)
    {
        while (buffer[index + 0] || buffer[index + 1])
        {
            index += 2;
        }
    }
    return index;
}

uint32_t strsizeUTF32(const uint8_t* const buffer) noexcept
{
    uint32_t index = 0;
    if (buffer != nullptr)
    {
        while (buffer[index + 0] || buffer[index + 1] || buffer[index + 2] || buffer[index + 3])
        {
            index += 4;
        }
    }
    return index;
}

// ==== quick UTF null (0) terminated code-point counting functions ====

uint32_t strlenUTF8(const uint8_t* const buffer) noexcept
{
    uint32_t count = 0;
    if (buffer != nullptr)
    {
        uint8_t byte = 0;
        for (uint32_t index = 0; (byte = buffer[index]) != 0; ++index)
        {
            if ((byte & 0xc0u) != 0x80u)
            {
                ++count;
            }
        }
    }
    return count;
}

uint32_t strlenUTF16le(const uint8_t* const buffer) noexcept
{
    uint32_t count = 0;
    if (buffer != nullptr)
    {
        uint16_t value = 0;
        for (uint32_t index = 0; (value = ((static_cast<uint16_t>(buffer[index + 1]) << 8) + buffer[index + 0])) != 0; index += 2)
        {
            if ((value & 0xfc00u) == 0xd800u)
            {
                value = ((static_cast<uint16_t>(buffer[index + 3]) << 8) + buffer[index + 2]);
                if ((value & 0xfc00u) == 0xdc00u)
                {
                    index += 2;
                }
            }
            ++count;
        }
    }
    return count;
}

uint32_t strlenUTF16be(const uint8_t* const buffer) noexcept
{
    uint32_t count = 0;
    if (buffer != nullptr)
    {
        uint16_t value = 0;
        for (uint32_t index = 0; (value = ((static_cast<uint16_t>(buffer[index + 0]) << 8) + buffer[index + 1])) != 0; index += 2)
        {
            if ((value & 0xfc00u) == 0xd800u)
            {
                value = ((static_cast<uint16_t>(buffer[index + 2]) << 8) + buffer[index + 3]);
                if ((value & 0xfc00u) == 0xdc00u)
                {
                    index += 2;
                }
            }
            ++count;
        }
    }
    return count;
}

uint32_t strlenUTF32(const uint8_t* const buffer) noexcept
{
    uint32_t count = 0;
    if (buffer != nullptr)
    {
        for (uint32_t index = 0; buffer[index + 0] || buffer[index + 1] || buffer[index + 2] || buffer[index + 3]; index += 4)
        {
            ++count;
        }
    }
    return count;
}

// ==== quick UTF fixed buffer size code-point counting functions ====

uint32_t strlenUTF8(const uint8_t* const buffer, const uint32_t size) noexcept
{
    uint32_t count = 0;
    if (buffer != nullptr)
    {
        uint32_t index = 0;
        for (uint32_t limit = size; limit; --limit)
        {
            if ((buffer[index] & 0xc0u) != 0x80u)
            {
                ++count;
            }
            ++index;
        }
    }
    return count;
}

uint32_t strlenUTF16le(const uint8_t* const buffer, const uint32_t size) noexcept
{
    uint32_t count = 0;
    if (buffer != nullptr)
    {
        uint32_t index = 0;
        for (uint32_t limit = size; limit >= 2; limit -= 2)
        {
            uint16_t value = ((static_cast<uint16_t>(buffer[index + 1]) << 8) + buffer[index + 0]);
            if (((value & 0xfc00u) == 0xd800u) && (limit >= 4))
            {
                value = ((static_cast<uint16_t>(buffer[index + 3]) << 8) + buffer[index + 2]);
                if ((value & 0xfc00u) == 0xdc00u)
                {
                    index += 2;
                    limit -= 2;
                }
            }
            index += 2;
            ++count;
        }
    }
    return count;
}

uint32_t strlenUTF16be(const uint8_t* const buffer, const uint32_t size) noexcept
{
    uint32_t count = 0;
    if (buffer != nullptr)
    {
        uint32_t index = 0;
        for (uint32_t limit = size; limit >= 2; limit -= 2)
        {
            uint16_t value = ((static_cast<uint16_t>(buffer[index + 0]) << 8) + buffer[index + 1]);
            if (((value & 0xfc00u) == 0xd800u) && (limit >= 4))
            {
                value = ((static_cast<uint16_t>(buffer[index + 2]) << 8) + buffer[index + 3]);
                if ((value & 0xfc00u) == 0xdc00u)
                {
                    index += 2;
                    limit -= 2;
                }
            }
            index += 2;
            ++count;
        }
    }
    return count;
}

// ==== quick UTF null (0) terminated format conversion size calculation functions (excludes the size of the null terminator) ====
// ==== note: the size of UTF32 buffers can be calculated directly from the code-point count returned by the strlen functions ====

uint32_t strsizeUTF8fromUTF16le(const uint8_t* const buffer) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t bytes = 0;
        unicode_t unicode = -1;
        for (uint32_t index = 0; unicode; index += bytes)
        {
            if (getUTF16le(&buffer[index], 4, unicode, bytes))
            {
                if (unicode)
                {
                    needs += lenUTF8(unicode, false);
                }
            }
        }
    }
    return needs;
}

uint32_t strsizeUTF8fromUTF16be(const uint8_t* const buffer) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t bytes = 0;
        unicode_t unicode = -1;
        for (uint32_t index = 0; unicode; index += bytes)
        {
            if (getUTF16be(&buffer[index], 4, unicode, bytes))
            {
                if (unicode)
                {
                    needs += lenUTF8(unicode, false);
                }
            }
        }
    }
    return needs;
}

uint32_t strsizeUTF8fromUTF32le(const uint8_t* const buffer) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t bytes = 0;
        unicode_t unicode = -1;
        for (uint32_t index = 0; unicode; index += bytes)
        {
            if (getUTF32le(&buffer[index], 4, unicode, bytes))
            {
                if (unicode)
                {
                    needs += lenUTF8(unicode, false);
                }
            }
        }
    }
    return needs;
}

uint32_t strsizeUTF8fromUTF32be(const uint8_t* const buffer) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t bytes = 0;
        unicode_t unicode = -1;
        for (uint32_t index = 0; unicode; index += bytes)
        {
            if (getUTF32be(&buffer[index], 4, unicode, bytes))
            {
                if (unicode)
                {
                    needs += lenUTF8(unicode, false);
                }
            }
        }
    }
    return needs;
}

uint32_t strsizeUTF16fromUTF8(const uint8_t* const buffer, const bool use_java) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t bytes = 0;
        unicode_t unicode = -1;
        for (uint32_t index = 0; unicode; index += bytes)
        {
            if (getUTF8(&buffer[index], 4, unicode, bytes, use_java))
            {
                if (unicode)
                {
                    needs += lenUTF16(unicode);
                }
                else if (use_java && (bytes == 2))
                {
                    needs += 2;
                    unicode = -1;
                }
            }
        }
    }
    return needs;
}

uint32_t strsizeUTF16fromUTF32le(const uint8_t* const buffer) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t bytes = 0;
        unicode_t unicode = -1;
        for (uint32_t index = 0; unicode; index += bytes)
        {
            if (getUTF32le(&buffer[index], 4, unicode, bytes))
            {
                if (unicode)
                {
                    needs += lenUTF16(unicode);
                }
            }
        }
    }
    return needs;
}

uint32_t strsizeUTF16fromUTF32be(const uint8_t* const buffer) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t bytes = 0;
        unicode_t unicode = -1;
        for (uint32_t index = 0; unicode; index += bytes)
        {
            if (getUTF32be(&buffer[index], 4, unicode, bytes))
            {
                if (unicode)
                {
                    needs += lenUTF16(unicode);
                }
            }
        }
    }
    return needs;
}

// ==== quick UTF fixed buffer size format conversion size calculation functions ====
// ==== note: the size of UTF32 buffers can be calculated directly from the code-point count returned by the strlen functions ====

uint32_t strsizeUTF8fromUTF16le(const uint8_t* const buffer, const uint32_t size, const bool use_java) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t limit = size;
        uint32_t bytes = 0;
        unicode_t unicode = 0;
        for (uint32_t index = 0; index < size; index += bytes)
        {
            if (getUTF16le(&buffer[index], limit, unicode, bytes))
            {
                needs += lenUTF8(unicode, use_java);
            }
            else if (unicode == 0)
            {
                break;
            }
            limit -= bytes;
        }
    }
    return needs;
}

uint32_t strsizeUTF8fromUTF16be(const uint8_t* const buffer, const uint32_t size, const bool use_java) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t limit = size;
        uint32_t bytes = 0;
        unicode_t unicode = 0;
        for (uint32_t index = 0; index < size; index += bytes)
        {
            if (getUTF16be(&buffer[index], limit, unicode, bytes))
            {
                needs += lenUTF8(unicode, use_java);
            }
            else if (unicode == 0)
            {
                break;
            }
            limit -= bytes;
        }
    }
    return needs;
}

uint32_t strsizeUTF8fromUTF32le(const uint8_t* const buffer, const uint32_t size, const bool use_java) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t limit = size;
        uint32_t bytes = 0;
        unicode_t unicode = 0;
        for (uint32_t index = 0; index < size; index += bytes)
        {
            if (getUTF32le(&buffer[index], limit, unicode, bytes))
            {
                needs += lenUTF8(unicode, use_java);
            }
            else if (unicode == 0)
            {
                break;
            }
            limit -= bytes;
        }
    }
    return needs;
}

uint32_t strsizeUTF8fromUTF32be(const uint8_t* const buffer, const uint32_t size, const bool use_java) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t limit = size;
        uint32_t bytes = 0;
        unicode_t unicode = 0;
        for (uint32_t index = 0; index < size; index += bytes)
        {
            if (getUTF32be(&buffer[index], limit, unicode, bytes))
            {
                needs += lenUTF8(unicode, use_java);
            }
            else if (unicode == 0)
            {
                break;
            }
            limit -= bytes;
        }
    }
    return needs;
}

uint32_t strsizeUTF16fromUTF8(const uint8_t* const buffer, const uint32_t size, const bool use_java) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t limit = size;
        uint32_t bytes = 0;
        unicode_t unicode = 0;
        for (uint32_t index = 0; index < size; index += bytes)
        {
            if (getUTF8(&buffer[index], limit, unicode, bytes, use_java))
            {
                needs += lenUTF16(unicode);
            }
            else if (unicode == 0)
            {
                break;
            }
            limit -= bytes;
        }
    }
    return needs;
}

uint32_t strsizeUTF16fromUTF32le(const uint8_t* const buffer, const uint32_t size) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t limit = size;
        uint32_t bytes = 0;
        unicode_t unicode = 0;
        for (uint32_t index = 0; index < size; index += bytes)
        {
            if (getUTF32le(&buffer[index], limit, unicode, bytes))
            {
                needs += lenUTF16(unicode);
            }
            else if (unicode == 0)
            {
                break;
            }
            limit -= bytes;
        }
    }
    return needs;
}

uint32_t strsizeUTF16fromUTF32be(const uint8_t* const buffer, const uint32_t size) noexcept
{
    uint32_t needs = 0;
    if (buffer != nullptr)
    {
        uint32_t limit = size;
        uint32_t bytes = 0;
        unicode_t unicode = 0;
        for (uint32_t index = 0; index < size; index += bytes)
        {
            if (getUTF32be(&buffer[index], limit, unicode, bytes))
            {
                needs += lenUTF16(unicode);
            }
            else if (unicode == 0)
            {
                break;
            }
            limit -= bytes;
        }
    }
    return needs;
}

// ==== encoded unicode code-point handling functions abstraction interface utility functions ====

[[nodiscard]] bool IUTF::get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept
{
    if (text.length >= text.offset)
    {
        return get(&text.buffer[text.offset], (text.length - text.offset), unicode, bytes);
    }
    unicode = 0;
    bytes = 0;
    return false;
}

[[nodiscard]] bool IUTF::set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept
{
    if (text.length >= text.offset)
    {
        return set(&text.buffer[text.offset], (text.length - text.offset), unicode, bytes);
    }
    bytes = 0;
    return false;
}

[[nodiscard]] bool IUTF::setBOM(utf_text& text, uint32_t& bytes) const noexcept
{
    if (text.length >= text.offset)
    {
        return setBOM(&text.buffer[text.offset], (text.length - text.offset), bytes);
    }
    bytes = 0;
    return false;
}

[[nodiscard]] bool IUTF::setNull(utf_text& text, uint32_t& bytes) const noexcept
{
    if (text.length >= text.offset)
    {
        return setNull(&text.buffer[text.offset], (text.length - text.offset), bytes);
    }
    bytes = 0;
    return false;
}

[[nodiscard]] bool IUTF::read(utf_text& text, unicode_t& unicode) const noexcept
{
    uint32_t bytes = 0;
    bool success = get(text, unicode, bytes);
    text.offset += bytes;
    return success;
}

[[nodiscard]] bool IUTF::write(utf_text& text, const unicode_t unicode) const noexcept
{
    uint32_t bytes = 0;
    bool success = set(text, unicode, bytes);
    text.offset += bytes;
    return success;
}

[[nodiscard]] bool IUTF::writeBOM(utf_text& text) const noexcept
{
    uint32_t bytes = 0;
    bool success = setBOM(text, bytes);
    text.offset += bytes;
    return success;
}

[[nodiscard]] bool IUTF::writeNull(utf_text& text) const noexcept
{
    uint32_t bytes = 0;
    bool success = setNull(text, bytes);
    text.offset += bytes;
    return success;
}

[[nodiscard]] bool IUTF::validate(const utf_text& text) const noexcept
{   //  attempts to read the entire buffer and returns false on any read fails
    if ((text.buffer == nullptr) || (text.offset < text.length))
    {
        return false;
    }
    utf_text scan = text;
    while (scan.offset < scan.length)
    {
        unicode_t unicode;
        if (!read(scan, unicode))
        {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool IUTF::getNLF(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept
{
    bytes = 0;
    bool success = get(text, unicode, bytes);
    if (success)
    {
        switch (unicode)
        {
            case(0x000au):  //  line-feed
            case(0x000du):  //  carriage return
            {   //  possible { 0x0d, 0x0a } or { 0x0a, 0x0d } pairing
                utf_text next = text;
                next.offset += bytes;
                uint32_t extra = 0;
                unicode_t pairing = 0;
                if (get(next, pairing, extra))
                {
                    if (static_cast<uint32_t>(unicode) == (pairing ^ 0x0007u))
                    {   //  found a { 0x0d, 0x0a } or { 0x0a, 0x0d } pairing
                        bytes += extra;
                    }
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
    return success;
}

[[nodiscard]] bool IUTF::readNLF(utf_text& text, unicode_t& unicode) const noexcept
{
    uint32_t bytes = 0;
    bool success = getNLF(text, unicode, bytes);
    text.offset += bytes;
    return success;
}

[[nodiscard]] bool IUTF::getLine(const utf_text& text, utf_text& line, uint32_t& bytes) const noexcept
{
    bytes = 0;
    line.length = 0;
    line.offset = 0;
    line.buffer = nullptr;
    bool success = (text.buffer != nullptr) && (text.offset <= text.length);
    if (success)
    {
        utf_text scan;
        scan.length = (text.length - text.offset);
        scan.offset = 0;
        scan.buffer = &text.buffer[text.offset];
        unicode_t unicode;
        success = false;
        while (getNLF(scan, unicode, bytes))
        {
            if ((unicode == 0x000au) || (unicode == 0x0000u))
            {
                bytes += scan.offset;
                line.length = scan.offset;
                line.offset = 0;
                line.buffer = scan.buffer;
                success = true;
                break;
            }
            scan.offset += bytes;
        };
    }
    return success;
}

[[nodiscard]] bool IUTF::readLine(utf_text& text, utf_text& line) const noexcept
{
    uint32_t bytes = 0;
    bool success = getLine(text, line, bytes);
    text.offset += bytes;
    return success;
}

// ==== concrete classes for abstracted UTF handling ====

class CUTF8 : public IUTF
{
    virtual UTF_TYPE            utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual uint32_t            unitSize(void) const noexcept { return 1; }
    virtual uint32_t            len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, false); }
    virtual uint32_t            lenBOM() const noexcept { return 3; }
    virtual uint32_t            lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] bool  get(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) const noexcept { return getUTF8(buffer, size, unicode, bytes, false); }
    virtual [[nodiscard]] bool  set(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) const noexcept { return setUTF8(buffer, size, unicode, bytes, false); }
    virtual [[nodiscard]] bool  setBOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF8_BOM(buffer, size, bytes); }
    virtual [[nodiscard]] bool  setNull(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF8_NULL(buffer, size, bytes); }
    virtual uint32_t            strsize(const uint8_t* const buffer) const noexcept { return strsizeUTF8(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer) const noexcept { return strlenUTF8(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer, const uint32_t size) const noexcept { return strlenUTF8(buffer, size); }
};

class CJUTF8 : public IUTF
{
    virtual UTF_TYPE            utfType(void) const noexcept { return UTF_TYPE::UTF8; }
    virtual uint32_t            unitSize(void) const noexcept { return 1; }
    virtual uint32_t            len(const unicode_t unicode) const noexcept { return lenUTF8(unicode, true); }
    virtual uint32_t            lenBOM() const noexcept { return 3; }
    virtual uint32_t            lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] bool  get(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) const noexcept { return getUTF8(buffer, size, unicode, bytes, true); }
    virtual [[nodiscard]] bool  set(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) const noexcept { return setUTF8(buffer, size, unicode, bytes, true); }
    virtual [[nodiscard]] bool  setBOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF8_BOM(buffer, size, bytes); }
    virtual [[nodiscard]] bool  setNull(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF8_NULL(buffer, size, bytes); }
    virtual uint32_t            strsize(const uint8_t* const buffer) const noexcept { return strsizeUTF8(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer) const noexcept { return strlenUTF8(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer, const uint32_t size) const noexcept { return strlenUTF8(buffer, size); }
};

class CUTF16le : public IUTF
{
    virtual UTF_TYPE            utfType(void) const noexcept { return UTF_TYPE::UTF16le; }
    virtual uint32_t            unitSize(void) const noexcept { return 2; }
    virtual uint32_t            len(const unicode_t unicode) const noexcept { return lenUTF16(unicode); }
    virtual uint32_t            lenBOM() const noexcept { return 2; }
    virtual uint32_t            lenNull() const noexcept { return 2; }
    virtual [[nodiscard]] bool  get(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) const noexcept { return getUTF16le(buffer, size, unicode, bytes); }
    virtual [[nodiscard]] bool  set(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) const noexcept { return setUTF16le(buffer, size, unicode, bytes); }
    virtual [[nodiscard]] bool  setBOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF16le_BOM(buffer, size, bytes); }
    virtual [[nodiscard]] bool  setNull(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF16_NULL(buffer, size, bytes); }
    virtual uint32_t            strsize(const uint8_t* const buffer) const noexcept { return strsizeUTF16(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer) const noexcept { return strlenUTF16le(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer, const uint32_t size) const noexcept { return strlenUTF16le(buffer, size); }
};

class CUTF16be : public IUTF
{
    virtual UTF_TYPE            utfType(void) const noexcept { return UTF_TYPE::UTF16be; }
    virtual uint32_t            unitSize(void) const noexcept { return 2; }
    virtual uint32_t            len(const unicode_t unicode) const noexcept { return lenUTF16(unicode); }
    virtual uint32_t            lenBOM() const noexcept { return 2; }
    virtual uint32_t            lenNull() const noexcept { return 2; }
    virtual [[nodiscard]] bool  get(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) const noexcept { return getUTF16be(buffer, size, unicode, bytes); }
    virtual [[nodiscard]] bool  set(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) const noexcept { return setUTF16be(buffer, size, unicode, bytes); }
    virtual [[nodiscard]] bool  setBOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF16be_BOM(buffer, size, bytes); }
    virtual [[nodiscard]] bool  setNull(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF16_NULL(buffer, size, bytes); }
    virtual uint32_t            strsize(const uint8_t* const buffer) const noexcept { return strsizeUTF16(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer) const noexcept { return strlenUTF16be(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer, const uint32_t size) const noexcept { return strlenUTF16be(buffer, size); }
};

class CUTF32le : public IUTF
{
    virtual UTF_TYPE            utfType(void) const noexcept { return UTF_TYPE::UTF32le; }
    virtual uint32_t            unitSize(void) const noexcept { return 4; }
    virtual uint32_t            len(const unicode_t unicode) const noexcept { return lenUTF32(unicode); }
    virtual uint32_t            lenBOM() const noexcept { return 4; }
    virtual uint32_t            lenNull() const noexcept { return 4; }
    virtual [[nodiscard]] bool  get(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) const noexcept { return getUTF32le(buffer, size, unicode, bytes); }
    virtual [[nodiscard]] bool  set(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) const noexcept { return setUTF32le(buffer, size, unicode, bytes); }
    virtual [[nodiscard]] bool  setBOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF32le_BOM(buffer, size, bytes); }
    virtual [[nodiscard]] bool  setNull(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF32_NULL(buffer, size, bytes); }
    virtual uint32_t            strsize(const uint8_t* const buffer) const noexcept { return strsizeUTF32(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer) const noexcept { return strlenUTF32(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer, const uint32_t size) const noexcept { (void)(buffer); return size >> 2; }
};

class CUTF32be : public IUTF
{
    virtual UTF_TYPE            utfType(void) const noexcept { return UTF_TYPE::UTF32be; }
    virtual uint32_t            unitSize(void) const noexcept { return 4; }
    virtual uint32_t            len(const unicode_t unicode) const noexcept { return lenUTF32(unicode); }
    virtual uint32_t            lenBOM() const noexcept { return 4; }
    virtual uint32_t            lenNull() const noexcept { return 4; }
    virtual [[nodiscard]] bool  get(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) const noexcept { return getUTF32be(buffer, size, unicode, bytes); }
    virtual [[nodiscard]] bool  set(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) const noexcept { return setUTF32be(buffer, size, unicode, bytes); }
    virtual [[nodiscard]] bool  setBOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF32be_BOM(buffer, size, bytes); }
    virtual [[nodiscard]] bool  setNull(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF32_NULL(buffer, size, bytes); }
    virtual uint32_t            strsize(const uint8_t* const buffer) const noexcept { return strsizeUTF32(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer) const noexcept { return strlenUTF32(buffer); }
    virtual uint32_t            strlen(const uint8_t* const buffer, const uint32_t size) const noexcept { (void)(buffer); return size >> 2; }
};

class CBYTE : public IUTF
{
    virtual UTF_TYPE            utfType(void) const noexcept { return UTF_TYPE::OTHER; }
    virtual uint32_t            unitSize(void) const noexcept { return 1; }
    virtual uint32_t            len(const unicode_t unicode) const noexcept { return lenBYTE(unicode, false); }
    virtual uint32_t            lenBOM() const noexcept { return 0; }
    virtual uint32_t            lenNull() const noexcept { return 1; }
    virtual [[nodiscard]] bool  get(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) const noexcept { return getBYTE(buffer, size, unicode, bytes, false); }
    virtual [[nodiscard]] bool  set(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) const noexcept { return setBYTE(buffer, size, unicode, bytes, false); }
    virtual [[nodiscard]] bool  setBOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { (void)(buffer); (void)(size); bytes = 0; return true; }
    virtual [[nodiscard]] bool  setNull(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept { return setUTF8_NULL(buffer, size, bytes); }
    virtual uint32_t            strsize(const uint8_t* const buffer) const noexcept { return buffer ? static_cast<uint32_t>(::strlen(reinterpret_cast<const char* const>(buffer))) : 0; }
    virtual uint32_t            strlen(const uint8_t* const buffer) const noexcept { return buffer ? static_cast<uint32_t>(::strlen(reinterpret_cast<const char* const>(buffer))) : 0; }
    virtual uint32_t            strlen(const uint8_t* const buffer, const uint32_t size) const noexcept { (void)(buffer); return size; }
};

class CASCII : public CBYTE
{
    virtual uint32_t            len(const unicode_t unicode) const noexcept { return lenBYTE(unicode, true); }
    virtual [[nodiscard]] bool  get(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) const noexcept { return getBYTE(buffer, size, unicode, bytes, true); }
    virtual [[nodiscard]] bool  set(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) const noexcept { return setBYTE(buffer, size, unicode, bytes, true); }
};

class CCP1252 : public CBYTE
{
    virtual uint32_t            len(const unicode_t unicode) const noexcept { return std::lenCP1252(unicode); }
    virtual [[nodiscard]] bool  get(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) const noexcept { return std::getCP1252(buffer, size, unicode, bytes); }
    virtual [[nodiscard]] bool  set(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) const noexcept { return std::setCP1252(buffer, size, unicode, bytes); }
};

// ==== quick UTF abstracted handler request functions ====

const IUTF& IUTF::getHandler(const UTF_TYPE utfType) noexcept
{
    switch (utfType)
    {
        case(UTF_TYPE::UTF8):       { static const CUTF8    handler; return handler; }
        case(UTF_TYPE::UTF16le):    { static const CUTF16le handler; return handler; }
        case(UTF_TYPE::UTF16be):    { static const CUTF16be handler; return handler; }
        case(UTF_TYPE::UTF32le):    { static const CUTF32le handler; return handler; }
        case(UTF_TYPE::UTF32be):    { static const CUTF32be handler; return handler; }
        default:                    { return getHandler(UTF_OTHER_TYPE::JUTF8); }
    }
}

const IUTF& IUTF::getHandler(const UTF_OTHER_TYPE utfOtherType) noexcept
{
    switch (utfOtherType)
    {
        case(UTF_OTHER_TYPE::ISO8859_1):    { static const CBYTE   handler; return handler; }
        case(UTF_OTHER_TYPE::Ascii):        { static const CASCII  handler; return handler; }
        case(UTF_OTHER_TYPE::CP1252):       { static const CCP1252 handler; return handler; }
        default:                            { static const CJUTF8  handler; return handler; }
    }
}

};  //  namespace std

};  //  namespace utf

};  //  namespace unicode
