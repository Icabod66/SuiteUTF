
//  Copyright (c) 2010-2026 Ritchie Brannan
//  License: MIT (see LICENSE file in repository root)
//
//  SuiteUTF
//  Original design 2010-2016; maintained and extended 2024-2025.
//  Copyright (c) 2010-2025 Ritchie Brannan.
//  MIT License. See LICENSE.txt. Project history: docs/History.md.
//
//  File:   unicode_classification.h
//  Author: Ritchie Brannan
//  Date:   11 July 16
//  
//  Description:
//  
//      Classification of unicode code-points usages.

#pragma once

#ifndef __UNICODE_CLASSIFICATION_INCLUDED__
#define __UNICODE_CLASSIFICATION_INCLUDED__

#include "unicode_type.h"

namespace unicode
{

// ==== unicode general classification functions ====
bool isBOM(const unicode_t unicode) noexcept;               //! a byte order mark
bool isUnicode(const unicode_t unicode) noexcept;           //! valid unicode (Rune compatible)
bool isCharacter(const unicode_t unicode) noexcept;         //! a character
bool isNonCharacter(const unicode_t unicode) noexcept;      //! a non-character
bool isCombining(const unicode_t unicode) noexcept;         //! a combining character
bool isPrivateUse(const unicode_t unicode) noexcept;        //! private use
bool isSpecial(const unicode_t unicode) noexcept;           //! a special
bool isSurrogate(const unicode_t unicode) noexcept;         //! a surrogate value
bool isHighSurrogate(const unicode_t unicode) noexcept;     //! a high surrogate value
bool isLowSurrogate(const unicode_t unicode) noexcept;      //! a low surrogate value
bool isC0(const unicode_t unicode) noexcept;                //! a c0 control character
bool isC1(const unicode_t unicode) noexcept;                //! a c1 control character
bool isCC(const unicode_t unicode) noexcept;                //! a c0, c1 or delete control character
bool isBreakingWhite(const unicode_t unicode) noexcept;     //! a breaking white space character
bool isTrivialWhite(const unicode_t unicode) noexcept;      //! a trivial white space character

// ==== unicode ASCII classification functions ====
bool isAsciiCC(const unicode_t unicode) noexcept;           //! an ascii control character
bool isAsciiText(const unicode_t unicode) noexcept;         //! standard ascii text
bool isAsciiWhite(const unicode_t unicode) noexcept;        //! ascii white space
bool isAsciiBlack(const unicode_t unicode) noexcept;        //! an ascii black character
bool isStrictAsciiText(const unicode_t unicode) noexcept;   //! strict ascii text (excludes vertical-tab and form-feed)
bool isStrictAsciiWhite(const unicode_t unicode) noexcept;  //! ascii white space (excludes vertical-tab and form-feed)

// ==== unicode XML classification functions ====
bool isNameStartXML(const unicode_t unicode) noexcept;      //! an XML name start character
bool isNameExtraXML(const unicode_t unicode) noexcept;      //! an XML name extra character
bool isNameXML(const unicode_t unicode) noexcept;           //! an XML name character
bool isPostNameXML(const unicode_t unicode) noexcept;       //! an XML post-name character
bool isWhiteXML(const unicode_t unicode) noexcept;          //! an XML white-space character
bool isCleanXML(const unicode_t unicode) noexcept;          //! unrestricted XML (in the allowed list and not in the discouraged list)

// ==== unicode JSON classification functions ====
bool isWhiteJSON(const unicode_t unicode) noexcept;         //! a JSON white-space character (from RFC 7159)
bool isHexEscapedJSON(const unicode_t unicode) noexcept;    //! JSON requires the code-point to use a hex escape

};  //  namespace unicode

#endif  //  #ifndef __UNICODE_CLASSIFICATION_INCLUDED__
