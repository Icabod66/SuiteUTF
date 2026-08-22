
//  Copyright (c) 2010-2026 Ritchie Brannan
//  License: MIT (see LICENSE file in repository root)
//
//  SuiteUTF
//  Original design 2010-2016; maintained and extended 2024-2025.
//  Copyright (c) 2010-2025 Ritchie Brannan.
//  MIT License. See LICENSE.txt. Project history: docs/History.md.
//
//  File:   unicode_utilities.h
//  Author: Ritchie Brannan
//  Date:   11 July 16
//  
//  Description:
//  
//      Unicode utility functions.

#pragma once

#ifndef __UNICODE_UTILITIES_INCLUDED__
#define __UNICODE_UTILITIES_INCLUDED__

#include "unicode_type.h"

namespace unicode
{

// ==== unicode general utility functions ====
int32_t   unicodeToHex(const unicode_t unicode) noexcept;       //! convert a hexadecimal unicode code-point to a 4-bit hex value returning -1 if the character is not valid hexadecimal
unicode_t hexToLowerUnicode(const int32_t hex) noexcept;        //! convert a 4-bit hex value (0x0-0xf) to a unicode code-point ("0123456789abcdef")
unicode_t hexToUpperUnicode(const int32_t hex) noexcept;        //! convert a 4-bit hex value (0x0-0xf) to a unicode code-point ("0123456789ABCDEF")
unicode_t toShortEscape(const unicode_t unicode) noexcept;      //! convert unicode to a standard short escape code character (returns -1 if no conversion)
unicode_t fromShortEscape(const unicode_t unicode) noexcept;    //! convert a standard short escape code character to unicode (returns -1 if no conversion)

// ==== unicode JSON utility functions ====
// ==== note: JSON only uses a subset of the standard short escapes ====
unicode_t toShortEscapeJSON(const unicode_t unicode) noexcept;      //! convert unicode to a JSON short escape code character (returns -1 if no conversion)
unicode_t fromShortEscapeJSON(const unicode_t unicode) noexcept;    //! convert a JSON short escape code character to unicode (returns -1 if no conversion)

// ==== Unicode/Windows code-page 1252 transcoding types and functions ====

enum class CP1252Strictness : uint8_t { WindowsCompatible = 0, StrictUndefined };

//! convert a Windows code-page 1252 code-point to a unicode code-point
bool cp1252ToUnicode(const uint8_t cp1252, unicode_t& unicode, const CP1252Strictness strictness = CP1252Strictness::WindowsCompatible) noexcept;

//! convert a unicode code-point to a Windows code-page 1252 code-point (returns false if not convertible)
bool unicodeToCP1252(const unicode_t unicode, uint8_t& cp1252, const CP1252Strictness strictness = CP1252Strictness::WindowsCompatible) noexcept;

};  //  namespace unicode

#endif  //  #ifndef __UNICODE_UTILITIES_INCLUDED__
