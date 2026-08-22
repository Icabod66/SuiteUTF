
//  Copyright (c) 2010-2026 Ritchie Brannan
//  License: MIT (see LICENSE file in repository root)
//
//  SuiteUTF
//  Original design 2010-2016; maintained and extended 2024-2025.
//  Copyright (c) 2010-2025 Ritchie Brannan.
//  MIT License. See LICENSE.txt. Project history: docs/History.md.
//
//  File:   utf_toolkit.h
//  Author: Ritchie Brannan
//  Date:   11 July 16
//  
//  Description:
//  
//      Robust UTF code-point handling.
//  
//  Important:
//  
//      Care should be always be taken to validate text buffers of unknown provenance before use.
//  
//  Decoder standards compliance:
//  
//      The UTF_SUB_TYPE_UTF8ns and UTF_SUB_TYPE_JUTF8ns types will pass Markus Kuhn's decoder stress test
//      with correct alignment.
//  
//      The UTF_SUB_TYPE_UTF8st and UTF_SUB_TYPE_JUTF8st types will pass Markus Kuhn's decoder stress test
//      using the malformed sequence as single bytes variation as specified in the introduction to the test.
//      These decoders produce results similar to the ATOM text editor and most web browsers.
//  
//      For all these types, a replacement character should be used when use_replacement_character() is true
//      for the error code returned by the decoder.
//  
//  Notes:
//  
//      Function control flags:
//  
//          use_ascii   : limits byte decoding to 7-bits.
//          use_ucs2    : disables UTF16 surrogate pairs and limits unicode to the BMP (U+0000 to U+FFFF).
//          use_ucs4    : enables UTF32 extended UCS4 range (U+00110000 to U+7FFFFFFF) as a standard encoding.
//          use_cesu    : enables use of surrogate pairs for supplemental plane code-points (U+10000 to U+10FFFF).
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

#pragma once

#ifndef __UTF_TOOLKIT_INCLUDED__
#define __UTF_TOOLKIT_INCLUDED__

#include "utf_std.h"
#include <type_traits>

namespace unicode
{

namespace utf
{

namespace toolkit
{

// ==== UTF encoding sub-type enumeration ====

//  Notes:
//
//      The CESU prefix:
//
//          Indicates 'Compatibility Encoding Scheme for UTF-16'.
//          Suffix digits indicate the code-unit size (8, 16 and 32 indicate bits; 1, 2 and 4 indicate bytes).
//          Uses UTF16 style surrogates for the supplementary planes range (U+00010000 to U+0010FFFF).
//
//      The UCS prefix and suffix digits 2 and 4:
//
//          Indicates 'Universal Character Set'.
//          Suffix digits indicate the code-unit size (2 or 4 bytes).
//          UCS2 restricts unicode to the basic multi-lingual plane (U+0000 to U+FFFF)
//          UCS4 treats the extended UCS4 range (U+00110000 to U+7FFFFFFF) as standards compliant.
//
//      The J prefix:
//
//          Indicates 'Modified-UTF8' as used by Java.
//          Uses a 2-byte overlong encoding of U+0000 (NULL) allowing a zero byte to be used as an end of string sentinel.
//          This allows multiple strings to be stored in a single zero byte terminated buffer.
//
//      The le suffix:
//
//          Indicates little-endian variants of encodings with multi-byte code-units.
//          If endianness is not explicit it should be assumed to be big-endian.
//
//      The be suffix:
//
//          Indicates big-endian variants of encodings with multi-byte code-units.
//          If endianness is not explicit it should be assumed to be big-endian.
//
//      The ns suffix:
//
//          Indicates 'non-skipping' variants of byte length code-unit encodings.
//          Sequences beginning with an illegal or invalid byte are treated as 1 byte sequences.
//          The standard decoders coalesce multiple adjacent illegal and invalid bytes into a single sequence.
//
//      The st suffix:
//
//          Indicates strict variants of UTF8 encodings.
//          Irregular forms are marked as decode fails and all decode fails are treated as 1 byte sequences.

enum class UTF_SUB_TYPE : int32_t
{

    UTF8        = 0,    //  type: UTF8,  sub-type: UTF8 (permissive)
    UTF8ns      = 1,    //  type: UTF8,  sub-type: UTF8 (non-skipping)
    UTF8st      = 2,    //  type: UTF8,  sub-type: UTF8 (strict)
    JUTF8       = 3,    //  type: UTF8,  sub-type: Java style UTF8 (permissive)
    JUTF8ns     = 4,    //  type: UTF8,  sub-type: Java style UTF8 (non-skipping)
    JUTF8st     = 5,    //  type: UTF8,  sub-type: Java style UTF8 (strict)
    CESU8       = 6,    //  type: UTF8,  sub-type: CESU8 (permissive)
    CESU8ns     = 7,    //  type: UTF8,  sub-type: CESU8 (non-skipping)
    CESU8st     = 8,    //  type: UTF8,  sub-type: CESU8 (strict)
    JCESU8      = 9,    //  type: UTF8,  sub-type: Java style CESU8 (permissive)
    JCESU8ns    = 10,   //  type: UTF8,  sub-type: Java style CESU8 (non-skipping)
    JCESU8st    = 11,   //  type: UTF8,  sub-type: Java style CESU8 (strict)
    UTF16le     = 12,   //  type: UTF16, sub-type: UTF16 (little endian)
    UTF16be     = 13,   //  type: UTF16, sub-type: UTF16 (big endian)
    UCS2le      = 14,   //  type: UTF16, sub-type: UCS2 (little endian)
    UCS2be      = 15,   //  type: UTF16, sub-type: UCS2 (big endian)
    UTF32le     = 16,   //  type: UTF32, sub-type: UTF32 (little endian)
    UTF32be     = 17,   //  type: UTF32, sub-type: UTF32 (big endian)
    UCS4le      = 18,   //  type: UTF32, sub-type: UCS4 (little endian)
    UCS4be      = 19,   //  type: UTF32, sub-type: UCS4 (big endian)
    CESU32le    = 20,   //  type: UTF32, sub-type: CESU UTF32 (little endian)
    CESU32be    = 21,   //  type: UTF32, sub-type: CESU UTF32 (big endian)
    CESU4le     = 22,   //  type: UTF32, sub-type: CESU UCS4 (little endian)
    CESU4be     = 23,   //  type: UTF32, sub-type: CESU UCS4 (big endian)
    BYTE        = 24,   //  type: other, sub-type: ISO-8859-1
    BYTEns      = 25,   //  type: other, sub-type: ISO-8859-1 (non-skipping)
    ASCII       = 26,   //  type: other, sub-type: ASCII
    ASCIIns     = 27,   //  type: other, sub-type: ASCII (non-skipping)
    CP1252      = 28,   //  type: other, sub-type: Windows Code-Page 1252 (permissive)
    CP1252ns    = 29,   //  type: other, sub-type: Windows Code-Page 1252 (non-skipping)
    CP1252st    = 30,   //  type: other, sub-type: Windows Code-Page 1252 (strict)
    COUNT       = 31    //  count of sub-types
};

/// code-point encode and decode functions return data type
class cp_errors
{
public:
    using underlying_type = uint32_t;
    enum class bits : underlying_type
    {
        None                = 0,
        Failed              = (1u << 31),   //  the operation failed (one or more errors were encountered)
        InvalidBuffer       = (1u << 30),   //  error       : r/w   : buffer        : the utf_text buffer pointer is NULL
        InvalidOffset       = (1u << 29),   //  error       : r/w   : buffer        : the utf_text offset is greater than the utf_text length
        MisalignedOffset    = (1u << 28),   //  error       : r/w   : buffer        : the utf_text offset is misaligned
        MisalignedLength    = (1u << 27),   //  error       : r/w   : buffer        : the utf_text length is misaligned
        WriteOverflow       = (1u << 26),   //  error       : w     : buffer        : the write operation would overflow the buffer
        ReadTruncated       = (1u << 25),   //  error       : r     : buffer        : the read operation would overrun the buffer (returned unicode is the lead byte)
        ReadExhausted       = (1u << 24),   //  warning     : r     : buffer        : the read operation is at the end of the buffer (returned unicode is 0)
        NotEncodable        = (1u << 23),   //  error       : w     : general       : the unicode code-point is not encodable using the specified encoding
        NotDecodable        = (1u << 22),   //  error       : r     : general       : the unicode code-point is not decodable using the specified encoding
        InvalidPoint        = (1u << 21),   //  warning     : r/w   : general       : the unicode code-point is in the invalid range (U+80000000 to U+FFFFFFFF)
        ExtendedUCS4        = (1u << 20),   //  warning     : r/w   : general       : the unicode code-point is in the extended UCS4 range (U+00110000 to U+7FFFFFFF)
        Supplementary       = (1u << 19),   //  warning     : r/w   : general       : the unicode code-point is in the supplementary planes range (U+00010000 to U+0010FFFF)
        NonCharacter        = (1u << 18),   //  warning     : r/w   : general       : the unicode code-point is a non-character
        TruncatedPair       = (1u << 17),   //  warning     : r     : general       : the unicode code-point is a high surrogate from a truncated pair (U+D800 to U+DBFF)
        SurrogatePair       = (1u << 16),   //  warning     : r/w   : general       : the unicode code-point is encoded as a surrogate pair
        HighSurrogate       = (1u << 15),   //  warning     : r/w   : general       : the unicode code-point is an unpaired high surrogate (U+D800 to U+DBFF)
        LowSurrogate        = (1u << 14),   //  warning     : r/w   : general       : the unicode code-point is an unpaired low surrogate (U+DC00 to U+DFFF)
        DelimitString       = (1u << 13),   //  warning     : r/w   : general       : the unicode code-point is a string delimiter (U+00000 NULL, if UTF8, it is neither overlong nor modified)
        IrregularForm       = (1u << 12),   //  warning     : r/w   : general       : the encoding can be processed but is not compliant with the specified encoding standard
        BadSizeUTF8         = (1u << 11),   //  error       : w     : UTF8          : the specified encoding length is invalid (the valid range is 1-6)
        ModifiedUTF8        = (1u << 10),   //  warning     : r/w   : UTF8          : the encoding is modified 2-byte 0
        OverlongUTF8        = (1u << 9),    //  warning     : r/w   : UTF8          : the encoding is overlong
        ExtendedUTF8        = (1u << 8),    //  warning     : r/w   : UTF8          : the encoding is more than 4 bytes long
        Untransformable     = (1u << 7),    //  error       : r/w   : diagnostic    : the code-point cannot be transformed between unicode and the specified encoding
        NotEnoughBits       = (1u << 6),    //  error       : w     : diagnostic    : the code-point requires more bits than are supported by the specified encoding
        DisallowedByte      = (1u << 5),    //  error       : r     : diagnostic    : found an illegal byte (not allowed by the encoding specification)
        UnexpectedByte      = (1u << 4),    //  error       : r     : diagnostic    : found an invalid byte (allowed by the encoding specification but in an unexpected position)
        ReservedBit3        = (1u << 3),    //  n/a         : n/a   : reserved      : unused
        ReservedBit2        = (1u << 2),    //  n/a         : n/a   : reserved      : used by the relative index of the illegal or invalid byte
        ReservedBit1        = (1u << 1),    //  n/a         : n/a   : reserved      : used by the relative index of the illegal or invalid byte
        ReservedBit0        = (1u << 0)     //  n/a         : n/a   : reserved      : used by the relative index of the illegal or invalid byte
    };
    constexpr cp_errors() noexcept : state(0) {}
    constexpr cp_errors(const bits bit) noexcept : state(static_cast<underlying_type>(bit)) {}
    explicit constexpr cp_errors(const underlying_type raw) noexcept : state(raw) {}
    constexpr underlying_type raw() const noexcept { return state; }
    constexpr [[nodiscard]] bool all(const cp_errors mask) const noexcept { return (state & mask.state) == mask.state; }
    constexpr [[nodiscard]] bool all(const underlying_type mask) const noexcept { return (state & mask) == mask; }
    constexpr [[nodiscard]] bool any() const noexcept { return (state & NonReservedMask) != 0; }
    constexpr [[nodiscard]] bool any(const bits bit) const noexcept { return (state & u(bit)) != 0; }
    constexpr [[nodiscard]] bool any(const cp_errors mask) const noexcept { return (state & mask.state) != 0; }
    constexpr [[nodiscard]] bool any(const underlying_type mask) const noexcept { return (state & mask) != 0; }
    constexpr [[nodiscard]] bool none() const noexcept { return !any(); }
    constexpr [[nodiscard]] bool none(const bits bit) const noexcept { return !any(bit); }
    constexpr [[nodiscard]] bool none(const cp_errors mask) const noexcept { return !any(mask); }
    constexpr [[nodiscard]] bool none(const underlying_type mask) const noexcept { return !any(mask); }
    constexpr void clear() noexcept { state = 0; }
    constexpr void clear(const bits bit) noexcept { state &= ~u(bit); }
    constexpr void clear(const cp_errors mask) noexcept { state &= ~mask.state; }
    constexpr void clear(const underlying_type mask) noexcept { state &= ~mask; }
    constexpr void reset() noexcept { clear(); }
    constexpr bool operator==(const cp_errors rhs) const noexcept { return state == rhs.state; }
    constexpr bool operator!=(const cp_errors rhs) const noexcept { return state != rhs.state; }
    constexpr [[nodiscard]] cp_errors operator~() const noexcept { return cp_errors(~state); }
    constexpr [[nodiscard]] cp_errors operator&(const bits rhs) const noexcept { return cp_errors(state & u(rhs)); }
    constexpr [[nodiscard]] cp_errors operator|(const bits rhs) const noexcept { return cp_errors(state | u(rhs)); }
    constexpr [[nodiscard]] cp_errors operator^(const bits rhs) const noexcept { return cp_errors(state ^ u(rhs)); }
    constexpr [[nodiscard]] cp_errors operator&(const cp_errors rhs) const noexcept { return cp_errors(state & rhs.state); }
    constexpr [[nodiscard]] cp_errors operator|(const cp_errors rhs) const noexcept { return cp_errors(state | rhs.state); }
    constexpr [[nodiscard]] cp_errors operator^(const cp_errors rhs) const noexcept { return cp_errors(state ^ rhs.state); }
    constexpr cp_errors& operator&=(const bits rhs) noexcept { state &= u(rhs); return *this; }
    constexpr cp_errors& operator|=(const bits rhs) noexcept { state |= u(rhs); return *this; }
    constexpr cp_errors& operator^=(const bits rhs) noexcept { state ^= u(rhs); return *this; }
    constexpr cp_errors& operator&=(const cp_errors rhs) noexcept { state &= rhs.state; return *this; }
    constexpr cp_errors& operator|=(const cp_errors rhs) noexcept { state |= rhs.state; return *this; }
    constexpr cp_errors& operator^=(const cp_errors rhs) noexcept { state ^= rhs.state; return *this; }
    explicit constexpr operator bool() const noexcept { return any(); }
    constexpr [[nodiscard]] bool failed() const noexcept { return any(bits::Failed); }
    constexpr [[nodiscard]] bool error() const noexcept { return any(ErrorsMask); }
    constexpr [[nodiscard]] bool no_error() const noexcept { return !error(); }
    constexpr [[nodiscard]] bool buffer_error() const noexcept { return any(BufferErrorsMask); }
    constexpr [[nodiscard]] bool is_rune_value() const noexcept { return none(ScalarDisallowedMask); }
    constexpr [[nodiscard]] bool is_strict_rune(const UTF_SUB_TYPE utfSubType) const noexcept
    {
        switch (utfSubType) {
            case UTF_SUB_TYPE::UTF8ns:
            case UTF_SUB_TYPE::UTF8st:
            case UTF_SUB_TYPE::UTF32le:
            case UTF_SUB_TYPE::UTF32be:
                return none(NonUTF16RuneDisallowedMask);
            case UTF_SUB_TYPE::UTF16le:
            case UTF_SUB_TYPE::UTF16be:
                return none(UTF16RuneDisallowedMask);
            default:
                return false;
        }
    }
    constexpr [[nodiscard]] bool use_replacement_character() const noexcept { return any(UseReplacementCharacterMask); }
    constexpr [[nodiscard]] cp_errors errors_only() const noexcept { return cp_errors(state & ErrorsMask); }
    constexpr [[nodiscard]] cp_errors warnings_only() const noexcept { return cp_errors(state & WarningsMask); }
    constexpr [[nodiscard]] cp_errors buffer_errors_only() const noexcept { return cp_errors(state & BufferErrorsMask); }
    constexpr [[nodiscard]] uint32_t get_byte_index() const noexcept { return uint32_t(state & ByteIndexMask); }
    constexpr void set_byte_index(const uint32_t index) noexcept { state = (state & ~ByteIndexMask) | (index & ByteIndexMask); }
private:
#define U(b) static_cast<underlying_type>(b)
    underlying_type state;
    static constexpr underlying_type u(bits b) noexcept { return U(b); }
    static constexpr underlying_type ErrorsMask =
        U(bits::Failed) | U(bits::InvalidBuffer) | U(bits::InvalidOffset) | U(bits::MisalignedOffset) |
        U(bits::MisalignedLength) | U(bits::WriteOverflow) | U(bits::ReadTruncated) | U(bits::NotEncodable) |
        U(bits::NotDecodable) | U(bits::BadSizeUTF8) | U(bits::Untransformable) | U(bits::NotEnoughBits) |
        U(bits::DisallowedByte) | U(bits::UnexpectedByte);
    static constexpr underlying_type WarningsMask =
        U(bits::ReadExhausted) | U(bits::InvalidPoint) | U(bits::ExtendedUCS4) | U(bits::Supplementary) |
        U(bits::NonCharacter) | U(bits::TruncatedPair) | U(bits::SurrogatePair) | U(bits::HighSurrogate) |
        U(bits::LowSurrogate) | U(bits::DelimitString) | U(bits::IrregularForm) | U(bits::ModifiedUTF8) |
        U(bits::OverlongUTF8) | U(bits::ExtendedUTF8);
    static constexpr underlying_type ReservedMask = U(bits::ReservedBit3) | U(bits::ReservedBit2) | U(bits::ReservedBit1) | U(bits::ReservedBit1);
    static constexpr underlying_type NonReservedMask = ~ReservedMask;
    static constexpr underlying_type ByteIndexMask = U(bits::ReservedBit2) | U(bits::ReservedBit1) | U(bits::ReservedBit0);
    static constexpr underlying_type BufferErrorsMask = U(bits::InvalidBuffer) | U(bits::InvalidOffset) | U(bits::MisalignedOffset) | U(bits::MisalignedLength);
    static constexpr underlying_type ScalarDisallowedMask = ~(
        U(bits::Supplementary) | U(bits::NonCharacter) | U(bits::SurrogatePair) |
        U(bits::IrregularForm) | U(bits::ModifiedUTF8) | U(bits::OverlongUTF8) | U(bits::ExtendedUTF8));
    static constexpr underlying_type UTF16RuneDisallowedMask = ~(U(bits::Supplementary) | U(bits::NonCharacter) | U(bits::SurrogatePair));
    static constexpr underlying_type NonUTF16RuneDisallowedMask = ~(U(bits::Supplementary) | U(bits::NonCharacter));
    static constexpr underlying_type UseReplacementCharacterMask = U(bits::NotDecodable) | U(bits::NonCharacter) | U(bits::IrregularForm);
#undef U
};

constexpr [[nodiscard]] cp_errors operator|(const cp_errors::bits lhs, const cp_errors rhs) noexcept
{
    return cp_errors(static_cast<cp_errors::underlying_type>(lhs) | rhs.raw());
}

constexpr [[nodiscard]] cp_errors operator|(const cp_errors::bits lhs, const cp_errors::bits rhs) noexcept
{
    return cp_errors(static_cast<cp_errors::underlying_type>(lhs) | static_cast<cp_errors::underlying_type>(rhs));
}

// ==== stand alone utf_text structure error checking ====
inline [[nodiscard]] cp_errors get_errors(const utf_text& text) noexcept;
inline [[nodiscard]] cp_errors get_errors(const utf_text& text, const uint32_t alignment_mask) noexcept;

// ==== code-point encoded length functions ====
uint32_t lenUTF8(const unicode_t unicode, const bool use_cesu = false, const bool use_java = false) noexcept;
uint32_t lenUTF16(const unicode_t unicode, const bool use_ucs2 = false) noexcept;
uint32_t lenUTF32(const unicode_t unicode, const bool use_cesu = false, const bool use_ucs4 = false) noexcept;

// ==== low level code-point encoding functions ====
[[nodiscard]] cp_errors encodeBYTE(utf_text& text, const unicode_t unicode, uint32_t& bytes, const bool use_ascii = false) noexcept;
[[nodiscard]] cp_errors encodeUTF8(utf_text& text, const unicode_t unicode, uint32_t& bytes, const bool use_cesu = false, const bool use_java = false) noexcept;
[[nodiscard]] cp_errors encodeUTF8n(utf_text& text, const unicode_t unicode, const uint32_t bytes, const bool use_java = false) noexcept;
[[nodiscard]] cp_errors encodeUTF16(utf_text& text, const unicode_t unicode, uint32_t& bytes, const bool le = false, const bool use_ucs2 = false) noexcept;
[[nodiscard]] cp_errors encodeUTF32(utf_text& text, const unicode_t unicode, uint32_t& bytes, const bool le = false, const bool use_cesu = false, const bool use_ucs4 = false) noexcept;
[[nodiscard]] cp_errors encodeCP1252(utf_text& text, const unicode_t unicode, uint32_t& bytes, const bool strict = false) noexcept;

// ==== low level code-point decoding functions ====
[[nodiscard]] cp_errors decodeBYTE(const utf_text& text, unicode_t& unicode, uint32_t& bytes, const bool use_ascii = false, const bool coalesce = true) noexcept;
[[nodiscard]] cp_errors decodeUTF8(const utf_text& text, unicode_t& unicode, uint32_t& bytes, const bool use_cesu = false, const bool use_java = false, const bool strict = false, const bool coalesce = true) noexcept;
[[nodiscard]] cp_errors decodeUTF16(const utf_text& text, unicode_t& unicode, uint32_t& bytes, const bool le = false, const bool use_ucs2 = false) noexcept;
[[nodiscard]] cp_errors decodeUTF32(const utf_text& text, unicode_t& unicode, uint32_t& bytes, const bool le = false, const bool use_cesu = false, const bool use_ucs4 = false) noexcept;
[[nodiscard]] cp_errors decodeCP1252(const utf_text& text, unicode_t& unicode, uint32_t& bytes, const bool strict = false, const bool coalesce = true) noexcept;

// ==== low level utf byte order marker and NULL code-point fast encoding functions ====
[[nodiscard]] cp_errors encodeUTF8_BOM(utf_text& text, uint32_t& bytes) noexcept;
[[nodiscard]] cp_errors encodeUTF16_BOM(utf_text& text, uint32_t& bytes, const bool le = false) noexcept;
[[nodiscard]] cp_errors encodeUTF32_BOM(utf_text& text, uint32_t& bytes, const bool le = false) noexcept;
[[nodiscard]] cp_errors encodeUTF8_NULL(utf_text& text, uint32_t& bytes) noexcept;
[[nodiscard]] cp_errors encodeUTF16_NULL(utf_text& text, uint32_t& bytes) noexcept;
[[nodiscard]] cp_errors encodeUTF32_NULL(utf_text& text, uint32_t& bytes) noexcept;

// ==== encoded code-point data stream forward and backward skip functions ====
// ==== note: These functions return the count of code-points (not bytes) that were successfully skipped ====
uint32_t backBYTE(utf_text& text, const uint32_t count, const bool use_ascii = false, const bool coalesce = true) noexcept;
uint32_t stepBYTE(utf_text& text, const uint32_t count, const bool use_ascii = false, const bool coalesce = true) noexcept;
uint32_t backUTF8(utf_text& text, const uint32_t count, const bool use_cesu = false, const bool use_java = false, const bool strict = false, const bool coalesce = true) noexcept;
uint32_t stepUTF8(utf_text& text, const uint32_t count, const bool use_cesu = false, const bool use_java = false, const bool strict = false, const bool coalesce = true) noexcept;
uint32_t backUTF16(utf_text& text, const uint32_t count, const bool le = false, const bool use_ucs2 = false) noexcept;
uint32_t stepUTF16(utf_text& text, const uint32_t count, const bool le = false, const bool use_ucs2 = false) noexcept;
uint32_t backUTF32(utf_text& text, const uint32_t count, const bool le = false, const bool use_cesu = false) noexcept;
uint32_t stepUTF32(utf_text& text, const uint32_t count, const bool le = false, const bool use_cesu = false) noexcept;
uint32_t backCP1252(utf_text& text, const uint32_t count, const bool strict = false, const bool coalesce = true) noexcept;
uint32_t stepCP1252(utf_text& text, const uint32_t count, const bool strict = false, const bool coalesce = true) noexcept;

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

inline constexpr [[nodiscard]] bool isOverlongUTF8(const unicode_t unicode, const uint32_t bytes) noexcept;
[[nodiscard]] bool overlongToIndexUTF8(const unicode_t unicode, const uint32_t bytes, uint32_t& index) noexcept;
[[nodiscard]] bool indexToOverlongUTF8(const uint32_t index, unicode_t& unicode, uint32_t& bytes) noexcept;

/// encoded unicode code-point handling functions abstraction interface with utility functions
struct IUTFTK
{
protected:
    inline                          IUTFTK() {};
    inline                          ~IUTFTK() {};
public:
    static const IUTFTK&            getHandler(const UTF_TYPE utfType = UTF_TYPE::OTHER) noexcept;
    static const IUTFTK&            getHandler(const UTF_SUB_TYPE utfSubType) noexcept;
    static const IUTFTK&            getHandler(const UTF_OTHER_TYPE utfOtherType) noexcept;
    virtual UTF_TYPE                utfType(void) const noexcept = 0;
    virtual UTF_SUB_TYPE            utfSubType(void) const noexcept = 0;
    virtual uint32_t                unitSize(void) const noexcept = 0;
    virtual uint32_t                len(const unicode_t unicode) const noexcept = 0;
    virtual uint32_t                lenBOM() const noexcept = 0;
    virtual uint32_t                lenNull() const noexcept = 0;
    virtual [[nodiscard]] cp_errors get(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept = 0;
    virtual [[nodiscard]] cp_errors set(utf_text& text, const unicode_t unicode, uint32_t& bytes) const noexcept = 0;
    virtual [[nodiscard]] cp_errors setBOM(utf_text& text, uint32_t& bytes) const noexcept = 0;
    virtual [[nodiscard]] cp_errors setNull(utf_text& text, uint32_t& bytes) const noexcept = 0;
    virtual uint32_t                back(utf_text& text, const uint32_t count) const noexcept = 0;
    virtual uint32_t                step(utf_text& text, const uint32_t count) const noexcept = 0;
    //  non-virtual utility functions:
    [[nodiscard]] cp_errors         read(utf_text& text, unicode_t& unicode) const noexcept;
    [[nodiscard]] cp_errors         write(utf_text& text, const unicode_t unicode) const noexcept;
    [[nodiscard]] cp_errors         writeBOM(utf_text& text) const noexcept;
    [[nodiscard]] cp_errors         writeNull(utf_text& text) const noexcept;
    [[nodiscard]] cp_errors         validate(const utf_text& text) const noexcept;
    //  non-virtual normalised line-feed functions (0x0a, 0x0b, 0x0c, 0x0d, 0x85, 0x2028, 0x2029, {0x0d, 0x0a} and {0x0a,0x0d} are all translated to 0x0a):
    [[nodiscard]] cp_errors         getNLF(const utf_text& text, unicode_t& unicode, uint32_t& bytes) const noexcept;
    [[nodiscard]] cp_errors         readNLF(utf_text& text, unicode_t& unicode) const noexcept;
    [[nodiscard]] cp_errors         getLine(const utf_text& text, utf_text& line, uint32_t& bytes) const noexcept;
    [[nodiscard]] cp_errors         readLine(utf_text& text, utf_text& line) const noexcept;
};

// ==== inline function bodies ====

[[nodiscard]] cp_errors get_errors(const utf_text& text) noexcept
{
    cp_errors errors;
    if (text.buffer == nullptr)
    {
        errors |= (cp_errors::bits::Failed | cp_errors::bits::InvalidBuffer);
    }
    if (text.offset > text.length)
    {
        errors |= (cp_errors::bits::Failed | cp_errors::bits::InvalidOffset);
    }
    return errors;
}

[[nodiscard]] cp_errors get_errors(const utf_text& text, const uint32_t alignment_mask) noexcept
{
    cp_errors errors = get_errors(text);
    if (text.offset & alignment_mask)
    {
        errors |= (cp_errors::bits::Failed | cp_errors::bits::MisalignedOffset);
    }
    if (text.length & alignment_mask)
    {
        errors |= (cp_errors::bits::Failed | cp_errors::bits::MisalignedLength);
    }
    return errors;
}

constexpr [[nodiscard]] bool isOverlongUTF8(const unicode_t unicode, const uint32_t bytes) noexcept
{
    bool overlong = false;
    if (static_cast<uint32_t>(bytes - 2) < 5)
    {
        int32_t n = static_cast<int32_t>((bytes | (bytes << 2)) - 11);
        overlong = ((uint32_t(unicode) < (uint32_t(1) << static_cast<uint32_t>((n & ((~n) >> 31)) + 7))) ? true : false);
    }
    return overlong;
}

};  //  namespace toolkit

};  //  namespace utf

};  //  namespace unicode

#endif  //  #ifndef __UTF_TOOLKIT_INCLUDED__
