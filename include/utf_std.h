
//  Copyright (c) 2010-2026 Ritchie Brannan
//  License: MIT (see LICENSE file in repository root)
//
//  SuiteUTF
//  Original design 2010-2016; maintained and extended 2024-2025.
//  Copyright (c) 2010-2025 Ritchie Brannan.
//  MIT License. See LICENSE.txt. Project history: docs/History.md.
//
//  File:   utf_std.h
//  Author: Ritchie Brannan
//  Date:   11 July 16
//  
//  Description:
//  
//      Simple UTF code-point handling.
//  
//  Important:
//  
//      Care should be always be taken to validate text buffers of unknown provenance before use.
//  
//      strlenUTF8() will only report the correct count of code-points for well formed UTF8 (including Java style).
//  
//  Decoder standards compliance:
//  
//      The quick GetUTF8() and SetUTF8() functions in will produce results that match the standard behaviour of most web browsers.
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
//          IUTF::getHandler(UTF_OTHER_TYPE::JUTF8).
// 
//      To get an ISO8859-1 (8-bit Unicode) handler, call:
//          IUTF::getHandler(UTF_OTHER_TYPE::ISO8859_1).
// 
//      To get a strict Ascii handler, call:
//          IUTF::getHandler(UTF_OTHER_TYPE::Ascii).
// 
//      To get a Windows Code-Page 1252 handler, call:
//          IUTF::getHandler(UTF_OTHER_TYPE::CP1252).

#pragma once

#ifndef __UTF_STD_INCLUDED__
#define __UTF_STD_INCLUDED__

#include "unicode_type.h"

namespace unicode
{

namespace utf
{

/// encoded code-point data stream structure
struct utf_text
{
    uint32_t    length; //! buffer size in bytes
    uint32_t    offset; //! byte offset into buffer
    uint8_t*    buffer;
};

/// UTF encoding type enumeration
enum class UTF_TYPE : int32_t
{
    UTF8    = 0,    //  UTF8
    UTF16le = 1,    //  little endian UTF16
    UTF16be = 2,    //  big endian UTF16
    UTF32le = 3,    //  little endian UTF32
    UTF32be = 4,    //  big endian UTF32
    OTHER   = 5,    //  non-UTF or unidentified (defaults to Java style UTF8)
    COUNT   = 6     //  count of types
};

/// Non-UTF or non-strict encoding type enumeration
enum class UTF_OTHER_TYPE : int32_t
{
    JUTF8     = 0,  //  Java style UTF8 (default)
    ISO8859_1 = 1,  //  ISO8859-1 (8-bit Unicode)
    Ascii     = 2,  //  strict Ascii
    CP1252    = 3,  //  Windows Code-Page 1252
    COUNT     = 4   //  count of types
};

/// UTF encoding identification function
/// 
///     This function attempts to identify UTF text encodings by the presence of a byte-order-marker or 2 leading ASCII
///     characters in the range 0x01 to 0x7f.
/// 
///     The 'bytes' output parameter indicates the number of bytes of BOM encountered (0, 2, 3 or 4).
/// 
UTF_TYPE identifyUTF(const uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept;

namespace std
{

// ==== quick Windows Code-Page 1252 encoded code-point length function ====
uint32_t lenCP1252(const unicode_t unicode) noexcept;

// ==== quick Windows Code-Page 1252 encode and decode functions ====
[[nodiscard]] bool getCP1252(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) noexcept;
[[nodiscard]] bool setCP1252(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) noexcept;

// ==== quick UTF encoded code-point length functions ====
inline constexpr uint32_t lenBYTE(const unicode_t unicode, const bool use_ascii = false) noexcept;
inline constexpr uint32_t lenUTF8(const unicode_t unicode, const bool use_java = false) noexcept;
inline constexpr uint32_t lenUTF16(const unicode_t unicode) noexcept;
inline constexpr uint32_t lenUTF32(const unicode_t unicode) noexcept;

// ==== quick UTF encode and decode functions ====
[[nodiscard]] bool getBYTE(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes, const bool use_ascii = false) noexcept;
[[nodiscard]] bool setBYTE(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes, const bool use_ascii = false) noexcept;
[[nodiscard]] bool getUTF8(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes, const bool use_java = false) noexcept;
[[nodiscard]] bool setUTF8(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes, const bool use_java = false) noexcept;
[[nodiscard]] bool getUTF16le(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) noexcept;
[[nodiscard]] bool setUTF16le(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) noexcept;
[[nodiscard]] bool getUTF16be(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) noexcept;
[[nodiscard]] bool setUTF16be(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) noexcept;
[[nodiscard]] bool getUTF32le(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) noexcept;
[[nodiscard]] bool setUTF32le(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) noexcept;
[[nodiscard]] bool getUTF32be(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) noexcept;
[[nodiscard]] bool setUTF32be(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) noexcept;

// ==== quick UTF byte order marker and NULL code-point encode functions ====
inline [[nodiscard]] bool setUTF8_BOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept;
inline [[nodiscard]] bool setUTF16le_BOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept;
inline [[nodiscard]] bool setUTF16be_BOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept;
inline [[nodiscard]] bool setUTF32le_BOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept;
inline [[nodiscard]] bool setUTF32be_BOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept;
inline [[nodiscard]] bool setUTF8_NULL(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept;
inline [[nodiscard]] bool setUTF16_NULL(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept;
inline [[nodiscard]] bool setUTF32_NULL(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept;

// ==== quick UTF null (0) terminated string byte length functions ====
uint32_t strsizeUTF8(const uint8_t* const buffer) noexcept;
uint32_t strsizeUTF16(const uint8_t* const buffer) noexcept;
uint32_t strsizeUTF32(const uint8_t* const buffer) noexcept;

// ==== quick UTF null (0) terminated code-point counting functions ====
uint32_t strlenUTF8(const uint8_t* const buffer) noexcept;
uint32_t strlenUTF16le(const uint8_t* const buffer) noexcept;
uint32_t strlenUTF16be(const uint8_t* const buffer) noexcept;
uint32_t strlenUTF32(const uint8_t* const buffer) noexcept;

// ==== quick UTF fixed buffer size code-point counting functions ====
uint32_t strlenUTF8(const uint8_t* const buffer, const uint32_t size) noexcept;
uint32_t strlenUTF16le(const uint8_t* const buffer, const uint32_t size) noexcept;
uint32_t strlenUTF16be(const uint8_t* const buffer, const uint32_t size) noexcept;

// ==== quick UTF null (0) terminated format conversion size calculation functions (excludes the size of the null terminator) ====
// ==== note: the size of UTF32 buffers can be calculated directly from the code-point count returned by the strlen functions ====
uint32_t strsizeUTF8fromUTF16le(const uint8_t* const buffer) noexcept;
uint32_t strsizeUTF8fromUTF16be(const uint8_t* const buffer) noexcept;
uint32_t strsizeUTF8fromUTF32le(const uint8_t* const buffer) noexcept;
uint32_t strsizeUTF8fromUTF32be(const uint8_t* const buffer) noexcept;
uint32_t strsizeUTF16fromUTF8(const uint8_t* const buffer, const bool use_java = false) noexcept;
uint32_t strsizeUTF16fromUTF32le(const uint8_t* const buffer) noexcept;
uint32_t strsizeUTF16fromUTF32be(const uint8_t* const buffer) noexcept;

// ==== quick UTF fixed buffer size format conversion size calculation functions ====
// ==== note: the size of UTF32 buffers can be calculated directly from the code-point count returned by the strlen functions ====
uint32_t strsizeUTF8fromUTF16le(const uint8_t* const buffer, const uint32_t size, const bool use_java = false) noexcept;
uint32_t strsizeUTF8fromUTF16be(const uint8_t* const buffer, const uint32_t size, const bool use_java = false) noexcept;
uint32_t strsizeUTF8fromUTF32le(const uint8_t* const buffer, const uint32_t size, const bool use_java = false) noexcept;
uint32_t strsizeUTF8fromUTF32be(const uint8_t* const buffer, const uint32_t size, const bool use_java = false) noexcept;
uint32_t strsizeUTF16fromUTF8(const uint8_t* const buffer, const uint32_t size, const bool use_java = false) noexcept;
uint32_t strsizeUTF16fromUTF32le(const uint8_t* const buffer, const uint32_t size) noexcept;
uint32_t strsizeUTF16fromUTF32be(const uint8_t* const buffer, const uint32_t size) noexcept;

/// quick UTF abstracted functions interface with utility functions
struct IUTF
{
protected:
    inline                      IUTF() {};
    inline                      ~IUTF() {};
public:
    static const IUTF&          getHandler(const UTF_TYPE utfType = UTF_TYPE::OTHER) noexcept;
    static const IUTF&          getHandler(const UTF_OTHER_TYPE utfOtherType) noexcept;
    virtual UTF_TYPE            utfType(void) const noexcept = 0;
    virtual uint32_t            unitSize(void) const noexcept = 0;
    virtual uint32_t            len(const unicode_t unicode) const noexcept = 0;
    virtual uint32_t            lenBOM() const noexcept = 0;
    virtual uint32_t            lenNull() const noexcept = 0;
    virtual [[nodiscard]] bool  get(const uint8_t* const buffer, const uint32_t size, unicode_t& unicode, uint32_t& bytes) const noexcept = 0;
    virtual [[nodiscard]] bool  set(uint8_t* const buffer, const uint32_t size, const unicode_t unicode, uint32_t& bytes) const noexcept = 0;
    virtual [[nodiscard]] bool  setBOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept = 0;
    virtual [[nodiscard]] bool  setNull(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) const noexcept = 0;
    virtual uint32_t            strsize(const uint8_t* const buffer) const noexcept = 0;
    virtual uint32_t            strlen(const uint8_t* const buffer) const noexcept = 0;
    virtual uint32_t            strlen(const uint8_t* const buffer, const uint32_t size) const noexcept = 0;
    //  non-virtual utility functions:
    [[nodiscard]] bool          get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept;
    [[nodiscard]] bool          set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept;
    [[nodiscard]] bool          setBOM(utf_text& text, uint32_t& bytes) const noexcept;
    [[nodiscard]] bool          setNull(utf_text& text, uint32_t& bytes) const noexcept;
    [[nodiscard]] bool          read(utf_text& text, unicode_t& unicode) const noexcept;
    [[nodiscard]] bool          write(utf_text& text, const unicode_t unicode) const noexcept;
    [[nodiscard]] bool          writeBOM(utf_text& text) const noexcept;
    [[nodiscard]] bool          writeNull(utf_text& text) const noexcept;
    [[nodiscard]] bool          validate(const utf_text& text) const noexcept;
    //  non-virtual normalised line-feed functions (0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x85, 0x2028, 0x2029, {0x0d, 0x0a} and {0x0a,0x0d} are all translated to 0x0a):
    [[nodiscard]] bool          getNLF(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept;
    [[nodiscard]] bool          readNLF(utf_text& text, unicode_t& unicode) const noexcept;
    [[nodiscard]] bool          getLine(const utf_text& text, utf_text& line, uint32_t& bytes) const noexcept;
    [[nodiscard]] bool          readLine(utf_text& text, utf_text& line) const noexcept;
};

// ==== inline function bodies ====

constexpr uint32_t lenBYTE(const unicode_t unicode, const bool use_ascii) noexcept
{
    return (static_cast<uint32_t>(unicode) <= (use_ascii ? 0x0000007fu : 0x000000ffu)) ? 1 : 0;
}

constexpr uint32_t lenUTF8(const unicode_t unicode, const bool use_java) noexcept
{
    uint32_t bytes = 0;
    if (static_cast<uint32_t>(unicode) <= 0x0010ffffu)
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
            if ((unicode & 0xfffff800u) != 0x0000d800u)
            {
                bytes = 3;
            }
        }
        else
        {   //  4 bytes (21 bits)
            bytes = 4;
        }
    }
    return bytes;
}

constexpr uint32_t lenUTF16(const unicode_t unicode) noexcept
{
    return (static_cast<uint32_t>(unicode) <= 0x0000ffffu) ? (((unicode & 0xfffff800u) != 0x0000d800u) ? 2 : 0) : ((static_cast<uint32_t>(unicode) <= 0x0010ffffu) ? 4 : 0);
}

constexpr uint32_t lenUTF32(const unicode_t unicode) noexcept
{
    return (static_cast<uint32_t>(unicode) <= 0x0010ffffu) ? 4 : 0;
}

[[nodiscard]] bool setUTF8_BOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept
{
    if ((buffer != nullptr) && (size >= 3))
    {
        buffer[0] = 0xefu;
        buffer[1] = 0xbbu;
        buffer[2] = 0xbfu;
        bytes = 3;
        return true;
    }
    bytes = 0;
    return false;
}

[[nodiscard]] bool setUTF16le_BOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept
{
    if ((buffer != nullptr) && (size >= 2))
    {
        buffer[0] = 0xffu;
        buffer[1] = 0xfeu;
        bytes = 2;
        return true;
    }
    bytes = 0;
    return false;
}

[[nodiscard]] bool setUTF16be_BOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept
{
    if ((buffer != nullptr) && (size >= 2))
    {
        buffer[0] = 0xfeu;
        buffer[1] = 0xffu;
        bytes = 2;
        return true;
    }
    bytes = 0;
    return false;
}

[[nodiscard]] bool setUTF32le_BOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept
{
    if ((buffer != nullptr) && (size >= 4))
    {
        buffer[0] = 0xffu;
        buffer[1] = 0xfeu;
        buffer[2] = buffer[3] = 0x00u;
        bytes = 4;
        return true;
    }
    bytes = 0;
    return false;
}

[[nodiscard]] bool setUTF32be_BOM(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept
{
    if ((buffer != nullptr) && (size >= 4))
    {
        buffer[0] = buffer[1] = 0x00u;
        buffer[2] = 0xfeu;
        buffer[3] = 0xffu;
        bytes = 4;
        return true;
    }
    bytes = 0;
    return false;
}

[[nodiscard]] bool setUTF8_NULL(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept
{
    if ((buffer != nullptr) && (size >= 1))
    {
        buffer[0] = 0;
        bytes = 1;
        return true;
    }
    bytes = 0;
    return false;
}

[[nodiscard]] bool setUTF16_NULL(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept
{
    if ((buffer != nullptr) && (size >= 2))
    {
        buffer[0] = buffer[1] = 0;
        bytes = 2;
        return true;
    }
    bytes = 0;
    return false;
}

[[nodiscard]] bool setUTF32_NULL(uint8_t* const buffer, const uint32_t size, uint32_t& bytes) noexcept
{
    if ((buffer != nullptr) && (size >= 4))
    {
        buffer[0] = buffer[1] = buffer[2] = buffer[3] = 0;
        bytes = 4;
        return true;
    }
    bytes = 0;
    return false;
}

};  //  namespace std

};  //  namespace utf

};  //  namespace unicode

#endif  //  #ifndef __UTF_STD_INCLUDED__
