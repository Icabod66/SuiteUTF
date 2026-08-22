
//  Copyright (c) 2010-2026 Ritchie Brannan
//  License: MIT (see LICENSE file in repository root)
//
//  SuiteUTF
//  Original design 2010-2016; maintained and extended 2024-2025.
//  Copyright (c) 2010-2025 Ritchie Brannan.
//  MIT License. See LICENSE.txt. Project history: docs/History.md.
//
//  File:   utf_helpers.h
//  Author: Ritchie Brannan
//  Date:   11 July 16
//  
//  Description:
//  
//      UTF helper functions.

#pragma once

#ifndef __UTF_HELPERS_INCLUDED__
#define __UTF_HELPERS_INCLUDED__

#include "unicode_type.h"

namespace unicode
{

namespace utf
{

// ==== UTF8 inline helper function declarations ====
inline constexpr uint32_t bitCountUTF8(const uint32_t bytes) noexcept;
inline constexpr unicode_t maxUnicodeUTF8(const uint32_t bytes) noexcept;
inline constexpr uint32_t leadToBytesUTF8(const uint8_t lead) noexcept;

// ==== UTF8 byte identification inline functions ====
inline constexpr bool isLeadUTF8(const uint8_t byte) noexcept { return ((byte & 0xc0u) != 0x80u) && (byte < 0xfeu); };
inline constexpr bool isContUTF8(const uint8_t byte) noexcept { return (byte & 0xc0u) == 0x80u; };
inline constexpr bool isBadUTF8(const uint8_t byte)  noexcept { return byte >= 0xfeu; };

// ==== GB18030 byte identification inline functions ====
inline constexpr bool isIllegalGB18030_Byte(const uint8_t byte)  noexcept { return byte == 0xffu; };
inline constexpr bool possibleGB18030_Byte0(const uint8_t byte0) noexcept { return byte0 <= 0xfeu; };
inline constexpr bool possibleGB18030_Byte1(const uint8_t byte1) noexcept { return ((byte1 >= 0x40u) && (byte1 <= 0xfeu) && (byte1 != 0x7fu)) || ((byte1 >= 0x30u) && (byte1 <= 0x39u)); };
inline constexpr bool possibleGB18030_Byte2(const uint8_t byte2) noexcept { return (byte2 >= 0x81u) && (byte2 <= 0xfeu); };
inline constexpr bool possibleGB18030_Byte3(const uint8_t byte3) noexcept { return (byte3 >= 0x30u) && (byte3 <= 0x39u); };
inline constexpr bool possibleGB18030_1Byte(const uint8_t byte0) noexcept { return byte0 <= 0x80u; };
inline constexpr bool possibleGB18030_2Byte(const uint8_t byte1) noexcept { return (byte1 >= 0x40u) && (byte1 <= 0xfeu) && (byte1 != 0x7fu); };
inline constexpr bool possibleGB18030_4Byte(const uint8_t byte1) noexcept { return (byte1 >= 0x30u) && (byte1 <= 0x39u); };

// ==== Shift encoding (SJIS and similar) byte identification inline functions ====
inline constexpr bool isIllegalSHIFT_Byte(const uint8_t byte)  noexcept { return byte >= 0xfdu; };
inline constexpr bool possibleSHIFT_Byte0(const uint8_t byte0) noexcept { return (byte0 <= 0xfcu) && (byte0 != 0x80u) && (byte0 != 0xa0u); };
inline constexpr bool possibleSHIFT_Byte1(const uint8_t byte1) noexcept { return (byte1 >= 0x40u) && (byte1 <= 0xfcu) && (byte1 != 0x7fu); };
inline constexpr bool possibleSHIFT_1Byte(const uint8_t byte0) noexcept { return (byte0 <= 0x7fu) || ((byte0 >= 0xa1u) && (byte0 <= 0xdfu)); };
inline constexpr bool possibleSHIFT_2Byte(const uint8_t byte0) noexcept { return (byte0 >= 0x81u) && (byte0 <= 0xfcu) && ((byte0 <= 0x9fu) || (byte0 >= 0xe0u)); };

// ==== inline function bodies ====

constexpr uint32_t bitCountUTF8(const uint32_t bytes) noexcept
{   //  returns the number of bits encodable in a UTF code point based on the number of bytes used by the UTF8 encoding
    uint32_t bitCount = 0;
    if (static_cast<uint32_t>(bytes - 1) < 6)
    {
        int32_t n = static_cast<int32_t>((bytes | (bytes << 2)) - 6);
        bitCount = static_cast<uint32_t>((n & ((~n) >> 31)) + 7);
    }
    return bitCount;
}

constexpr unicode_t maxUnicodeUTF8(const uint32_t bytes) noexcept
{   //  returns the maximum unicode value that can be encoded in the specified number of bytes
    unicode_t unicode = -1;
    if (static_cast<uint32_t>(bytes - 1) < 6)
    {
        int32_t n = static_cast<int32_t>((bytes | (bytes << 2)) - 6);
        unicode = ((unicode_t(1) << static_cast<uint32_t>((n & ((~n) >> 31)) + 7)) - 1);
    }
    return unicode;
}

constexpr uint32_t leadToBytesUTF8(const uint8_t lead) noexcept
{   //  returns the number of bytes in the UTF code point based on the leading byte
    if (static_cast<uint8_t>(lead + 2) <= 0xc1u)
    {   //  1 byte (7 bits: 0x00-0x7f) or unexpected continuation byte (0x80-0xbf) or illegal leading byte (0xfe-0xff)
        return 1;
    }
    else if (lead <= 0xefu)
    {   //  2 bytes (11 bits: 0xc0-0xdf) or 3 bytes (16 bits: 0xe0-0xef)
        return static_cast<uint32_t>((lead >> 5) & 3);
    }
    else if (lead <= 0xf7u)
    {   //  4 bytes (21 bits: 0xf0-0xf7)
        return 4;
    }
    else
    {   //  extended encoding: 5 bytes (26 bits: 0xf8-0xfb) or 6 bytes (31 bits: 0xfc-0xfd)
        return static_cast<uint32_t>(((lead >> 2) & 3) + 3);
    }
}

};  //  namespace utf

};  //  namespace unicode

#endif  //  #ifndef __UTF_HELPERS_INCLUDED__
