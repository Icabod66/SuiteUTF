
//  Copyright (c) 2010-2026 Ritchie Brannan
//  License: MIT (see LICENSE file in repository root)
//
//  SuiteUTF
//  Original design 2010-2016; maintained and extended 2024-2025.
//  Copyright (c) 2010-2025 Ritchie Brannan.
//  MIT License. See LICENSE.txt. Project history: docs/History.md.
//
//  File:   unicode_utilities.cpp
//  Author: Ritchie Brannan
//  Date:   11 July 16
//  
//  Description:
//  
//      Unicode support functions.

#include "unicode_utilities.h"

namespace unicode
{

// ==== unicode general utility functions ====

//! convert a hexadecimal unicode code-point to a 4-bit hex value returning -1 if the character is not valid hexadecimal
int32_t unicodeToHex(const unicode_t unicode) noexcept
{   //  converts "0123456789abcdef" or "0123456789ABCDEF"
    int32_t n = (unicode - '0');
    int32_t a = ((unicode | 0x20u) - 'a');
    return (n | ((n | (9 - n)) >> 31)) & ((a + 10) | ((a | (5 - a)) >> 31));
}

//! convert a 4-bit hex value (0x0-0xf) to a unicode code-point ("0123456789abcdef")
unicode_t hexToLowerUnicode(const int32_t hex) noexcept
{
    int32_t a = ((hex & 15) - 10);
    return a + 'a' - ((a >> 31) & 39);
}

//! convert a 4-bit hex value (0x0-0xf) to a unicode code-point ("0123456789ABCDEF")
unicode_t hexToUpperUnicode(const int32_t hex) noexcept
{
    int32_t a = ((hex & 15) - 10);
    return a + 'A' - ((a >> 31) & 7);
}

//! convert unicode to a standard short escape code character (returns -1 if no conversion)
unicode_t toShortEscape(const unicode_t unicode) noexcept
{
    switch (unicode)
    {
        case(0x0007u):  return 0x0061u; //  '\a' (bell) -> 'a'
        case(0x0008u):  return 0x0062u; //  '\b' (back-space) -> 'b'
        case(0x0009u):  return 0x0074u; //  '\t' (tab) -> 't'
        case(0x000au):  return 0x006eu; //  '\n' (new-line/line-feed) -> 'n'
        case(0x000bu):  return 0x0076u; //  '\v' (vertical tab) -> 'v'
        case(0x000cu):  return 0x0066u; //  '\f' (form-feed) -> 'f'
        case(0x000du):  return 0x0072u; //  '\r' (carriage return) -> 'r'
        case(0x0022u):  return 0x0022u; //  '"' (double quote) -> '"'
        case(0x0027u):  return 0x0027u; //  '\'' (quote) -> '\''
        case(0x002fu):  return 0x002fu; //  '/' (slash) -> '/'
        case(0x003fu):  return 0x003fu; //  '?' (question mark) -> '?'
        case(0x005cu):  return 0x005cu; //  '\' (back-slash) -> '\'
        default:        return -1;      //  no conversion
    }
}

//! convert a standard short escape code character to unicode (returns -1 if no conversion)
unicode_t fromShortEscape(const unicode_t unicode) noexcept
{
    switch (unicode)
    {
        case(0x0022u):  return 0x0022u; //  '"' -> '"' (double quote)
        case(0x0027u):  return 0x0027u; //  '\'' -> '\'' (quote)
        case(0x002fu):  return 0x002fu; //  '/' -> '/' (slash)
        case(0x003fu):  return 0x003fu; //  '?' -> '?' (question mark)
        case(0x005cu):  return 0x005cu; //  '\' -> '\' (back-slash)
        case(0x0061u):  return 0x0007u; //  'a' -> '\a' (bell)
        case(0x0062u):  return 0x0008u; //  'b' -> '\b' (back-space)
        case(0x0066u):  return 0x000cu; //  'f' -> '\f' (form-feed)
        case(0x006eu):  return 0x000au; //  'n' -> '\n' (new-line/line-feed)
        case(0x0072u):  return 0x000du; //  'r' -> '\r' (carriage return)
        case(0x0074u):  return 0x0009u; //  't' -> '\t' (tab)
        case(0x0076u):  return 0x000bu; //  'v' -> '\v' (vertical tab)
        default:        return -1;      //  no conversion
    }
}

// ==== unicode JSON utility functions ====
// ==== note: JSON only uses a subset of the standard short escapes ====

//! convert unicode to a JSON short escape code character (returns -1 if no conversion)
unicode_t toShortEscapeJSON(const unicode_t unicode) noexcept
{
    switch (unicode)
    {
        case(0x0008u):  return 0x0062u; //  '\b' (back-space) -> 'b'
        case(0x0009u):  return 0x0074u; //  '\t' (tab) -> 't'
        case(0x000au):  return 0x006eu; //  '\n' (new-line/line-feed) -> 'n'
        case(0x000cu):  return 0x0066u; //  '\f' (form-feed) -> 'f'
        case(0x000du):  return 0x0072u; //  '\r' (carriage return) -> 'r'
        case(0x0022u):  return 0x0022u; //  '"' (double quote) -> '"'
        case(0x002fu):  return 0x002fu; //  '/' (slash) -> '/'
        case(0x005cu):  return 0x005cu; //  '\' (back-slash) -> '\'
        default:        return -1;      //  no conversion
    }
}

//! convert a JSON short escape code character to unicode (returns -1 if no conversion)
unicode_t fromShortEscapeJSON(const unicode_t unicode) noexcept
{
    switch (unicode)
    {
        case(0x0022u):  return 0x0022u; //  '"' -> '"' (double quote)
        case(0x002fu):  return 0x002fu; //  '/' -> '/' (slash)
        case(0x005cu):  return 0x005cu; //  '\' -> '\' (back-slash)
        case(0x0062u):  return 0x0008u; //  'b' -> '\b' (back-space)
        case(0x0066u):  return 0x000cu; //  'f' -> '\f' (form-feed)
        case(0x006eu):  return 0x000au; //  'n' -> '\n' (new-line/line-feed)
        case(0x0072u):  return 0x000du; //  'r' -> '\r' (carriage return)
        case(0x0074u):  return 0x0009u; //  't' -> '\t' (tab)
        default:        return -1;      //  no conversion
    }
}

// ==== Unicode/Windows code-page 1252 transcoding functions ====

//! unicode/CP1252 undefined mappings helper function
inline bool isCP1252UndefinedC1(unicode_t unicode) noexcept
{
    return
        (unicode == 0x0081u) || (unicode == 0x008du) ||
        (unicode == 0x008fu) || (unicode == 0x0090u) ||
        (unicode == 0x009du);
}

//! convert a Windows code-page 1252 code-point to a unicode code-point
bool cp1252ToUnicode(const uint8_t cp1252, unicode_t& unicode, const CP1252Strictness strictness) noexcept
{
    static const uint16_t translate[32] = {
        0x20acu, 0x0081u, 0x201au, 0x0192u, 0x201eu, 0x2026u, 0x2020u, 0x2021u,
        0x02c6u, 0x2030u, 0x0160u, 0x2039u, 0x0152u, 0x008du, 0x017du, 0x008fu,
        0x0090u, 0x2018u, 0x2019u, 0x201cu, 0x201du, 0x2022u, 0x2013u, 0x2014u,
        0x02dcu, 0x2122u, 0x0161u, 0x203au, 0x0153u, 0x009du, 0x017eu, 0x0178u };
    const uint8_t index = (cp1252 ^ 0x80u);
    unicode = ((index < 32) ? static_cast<unicode_t>(translate[index]) : static_cast<unicode_t>(cp1252));
    if ((strictness == CP1252Strictness::StrictUndefined) && isCP1252UndefinedC1(unicode))
    {
        unicode = 0;
        return false;
    }
    return true;
}

//! convert a unicode code-point to a Windows code-page 1252 code-point (returns false if not convertible)
bool unicodeToCP1252(const unicode_t unicode, uint8_t& cp1252, const CP1252Strictness strictness) noexcept
{
    if (static_cast<uint32_t>(unicode) <= 0x00ffu)
    {
        if ((unicode <= 0x007fu) || (unicode >= 0x00a0u) || ((strictness == CP1252Strictness::WindowsCompatible) && isCP1252UndefinedC1(unicode)))
        {
            cp1252 = static_cast<uint8_t>(unicode);
            return true;
        }
        cp1252 = 0x00u;
        return false;
    }
    switch (unicode)
    {
        case(0x0152u):  cp1252 = 0x8cu; return true;
        case(0x0153u):  cp1252 = 0x9cu; return true;
        case(0x0160u):  cp1252 = 0x8au; return true;
        case(0x0161u):  cp1252 = 0x9au; return true;
        case(0x0178u):  cp1252 = 0x9fu; return true;
        case(0x017Du):  cp1252 = 0x8eu; return true;
        case(0x017Eu):  cp1252 = 0x9eu; return true;
        case(0x0192u):  cp1252 = 0x83u; return true;
        case(0x02C6u):  cp1252 = 0x88u; return true;
        case(0x02DCu):  cp1252 = 0x98u; return true;
        case(0x2013u):  cp1252 = 0x96u; return true;
        case(0x2014u):  cp1252 = 0x97u; return true;
        case(0x2018u):  cp1252 = 0x91u; return true;
        case(0x2019u):  cp1252 = 0x92u; return true;
        case(0x201Au):  cp1252 = 0x82u; return true;
        case(0x201Cu):  cp1252 = 0x93u; return true;
        case(0x201Du):  cp1252 = 0x94u; return true;
        case(0x201Eu):  cp1252 = 0x84u; return true;
        case(0x2020u):  cp1252 = 0x86u; return true;
        case(0x2021u):  cp1252 = 0x87u; return true;
        case(0x2022u):  cp1252 = 0x95u; return true;
        case(0x2026u):  cp1252 = 0x85u; return true;
        case(0x2030u):  cp1252 = 0x89u; return true;
        case(0x2039u):  cp1252 = 0x8bu; return true;
        case(0x203Au):  cp1252 = 0x9bu; return true;
        case(0x20ACu):  cp1252 = 0x80u; return true;
        case(0x2122u):  cp1252 = 0x99u; return true;
        default:        cp1252 = 0x00u; return false;
    }
}

};    //  namespace unicode
