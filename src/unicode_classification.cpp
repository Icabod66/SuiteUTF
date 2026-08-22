
//  Copyright (c) 2010-2026 Ritchie Brannan
//  License: MIT (see LICENSE file in repository root)
//
//  SuiteUTF
//  Original design 2010-2016; maintained and extended 2024-2025.
//  Copyright (c) 2010-2025 Ritchie Brannan.
//  MIT License. See LICENSE.txt. Project history: docs/History.md.
//
//  File:   unicode_classification.cpp
//  Author: Ritchie Brannan
//  Date:   11 July 16
//  
//  Description:
//  
//      Classification of unicode code-points usages.

#include "unicode_classification.h"

namespace unicode
{

// ==== unicode general classification functions ====

//! determine if a unicode code-point is a byte order mark
bool isBOM(const unicode_t unicode) noexcept
{
	return unicode == 0xfeffu;
}

//! determine if a unicode code-point is valid (a Rune)
bool isUnicode(const unicode_t unicode) noexcept
{
	return (static_cast<uint32_t>(unicode) <= 0x0010ffffu) && ((unicode & 0xfffff800u) != 0xd800u);
}

//! determine if a unicode code-point is a character
bool isCharacter(const unicode_t unicode) noexcept
{
	return (static_cast<uint32_t>(unicode) <= 0x0010ffffu) && ((unicode & 0xfffff800u) != 0xd800u) && ((unicode & 0xfffeu) != 0xfffeu) && (((unicode - 0xfdd0u) & 0xffffffe0u) != 0x0000u);
}

//! determine if a unicode code-point is a non-character
bool isNonCharacter(const unicode_t unicode) noexcept
{
	return (unicode >= 0xfdd0u) && ((unicode <= 0xfdefu) || ((unicode <= 0x0010ffffu) && ((unicode & 0xfffeu) == 0xfffeu)));
}

//! determine if a unicode code-point is a combining character
bool isCombining(const unicode_t unicode) noexcept
{   //  U+0300-036F, 1AB0-1AFF, 1DC0-1DFF, 20D0-20FF, FE20-FE2F
	return (unicode >= 0x0300u) && ((unicode <= 0x036fu) || (((unicode >= 0x1ab0u) && (unicode <= 0x20ffu)) && ((unicode <= 0x1affu) || (unicode >= 0x20d0u) || ((unicode & 0xffffffc0u) == 0x1dc0u))) || ((unicode & 0xfffffff0u) == 0xfe20u));
}

//! determine if a unicode code-point is private use
bool isPrivateUse(const unicode_t unicode) noexcept
{
	return (unicode >= 0xe000u) && ((unicode <= 0xf8ffu) || ((unicode >= 0x000f0000u) && (unicode <= 0x0010ffffu) && ((unicode & 0xfffeu) != 0xfffeu)));
}

//! determine if a unicode code-point is a special
bool isSpecial(const unicode_t unicode) noexcept
{
	return (unicode & 0xfffffff0u) == 0xfff0u;
}

//! determine if a unicode code-point is a surrogate value
bool isSurrogate(const unicode_t unicode) noexcept
{
	return (unicode & 0xfffff800u) == 0xd800u;
}

//! determine if a unicode code-point is a high surrogate value
bool isHighSurrogate(const unicode_t unicode) noexcept
{
	return (unicode & 0xfffffc00u) == 0xd800u;
}

//! determine if a unicode code-point is a low surrogate value
bool isLowSurrogate(const unicode_t unicode) noexcept
{
	return (unicode & 0xfffffc00u) == 0xdc00u;
}

//! determine if a unicode code-point is a c0 control character
bool isC0(const unicode_t unicode) noexcept
{   //  c0 control code characters
	return (unicode & 0xffffffe0u) == 0x0000u;
}

//! determine if a unicode code-point is a c1 control character
bool isC1(const unicode_t unicode) noexcept
{   //  c1 control code characters
	return (unicode & 0xffffffe0u) == 0x0080u;
}

//! determine if a unicode code-point is a c0, c1 or delete control character
bool isCC(const unicode_t unicode) noexcept
{   //  c0, c1 or delete control code characters
	return ((unicode & 0xffffff60u) == 0x0000u) || (unicode == 0x007fu);
}

//! determine if a unicode code-point is a breaking white space character
bool isBreakingWhite(const unicode_t unicode) noexcept
{
	if (unicode >= 0x2000u)
	{
		if (unicode <= 0x200au)
		{
			return unicode != 0x2007u;
		}
		switch (unicode)
		{
			case(0x2028u): //  line separator
			case(0x2029u): //  paragraph separator
			case(0x205fu): //  medium mathematical space
			case(0x3000u): //  ideographic space
			{
				return true;
			}
			default:
			{
				return false;
			}
		}
	}
	else
	{
		if (unicode <= 0x0008u)
		{
			return false;
		}
		if (unicode <= 0x000du)
		{   //  0x0009 (tab), ox000a (line-feed), 0x000b (vertical tab), 0x000c (form-feed), 0x000d (carriage return)
			return true;
		}
		switch (unicode)
		{
			case(0x0020u): //  space
			case(0x0085u): //  next line
			case(0x1680u): //  Ogham space mark
			{
				return true;
			}
			default:
			{
				return false;
			}
		}
	}
}

//! determine if a unicode code-point is a trivial white-space character
bool isTrivialWhite(const unicode_t unicode) noexcept
{   //  ' ', '\t', '\r', '\n'
	return (unicode == 0x0020u) || (unicode == 0x0009u) || (unicode == 0x000au) || (unicode == 0x000du);
}

// ==== unicode ASCII classification functions ====

//! determine if a unicode code-point is an ascii control character
bool isAsciiCC(const unicode_t unicode) noexcept
{
	return (unicode >= 0x00u) && ((unicode <= 0x1fu) || (unicode == 0x7fu));
}

//! determine if a unicode code-point is standard ascii text
bool isAsciiText(const unicode_t unicode) noexcept
{
	return (unicode <= 0x7eu) && (unicode >= 0x09u) && ((unicode >= 0x20u) || (unicode <= 0x0du));
}

//! determine if a unicode code-point is ascii white space
bool isAsciiWhite(const unicode_t unicode) noexcept
{
	return (unicode == 0x20u) || ((unicode >= 0x09u) && (unicode <= 0x0du));
}

//! determine if a unicode code-point is an ascii black character
bool isAsciiBlack(const unicode_t unicode) noexcept
{
	return (unicode <= 0x7eu) && (unicode >= 0x21u);
}

//! determine if a unicode code-point is strict ascii text (excludes vertical-tab and form-feed)
bool isStrictAsciiText(const unicode_t unicode) noexcept
{
	return (unicode <= 0x7eu) && (unicode >= 0x09u) && ((unicode >= 0x20u) || (unicode <= 0x0au) || (unicode == 0x0du));
}

//! determine if a unicode code-point is ascii white space (excludes vertical-tab and form-feed)
bool isStrictAsciiWhite(const unicode_t unicode) noexcept
{
	return (unicode == 0x20u) || (unicode == 0x09u) || (unicode == 0x0au) || (unicode == 0x0du);
}

// ==== unicode XML classification functions ====

//! determine if a unicode code-point is an XML name start character
bool isNameStartXML(const unicode_t unicode) noexcept
{
	if (unicode < 0x2000u)
	{
		if (unicode >= 0x00c0u)
		{
			if (unicode >= 0x0300u)
			{
				if ((unicode >= 0x0370u) && (unicode != 0x037eu))
				{	//	[0x0370-0x37d] [0x037f-0x1fff]
					return true;
				}
			}
			else
			{
				if ((unicode != 0x00d7u) && (unicode != 0x00f7u))
				{	//	[0xc0-0xd6] [0xd8-0xf6] [0xf8-0x02ff]
					return true;
				}
			}
		}
		else if (unicode <= 0x005au)	//  ( unicode <= 'Z' )
		{
			if ((unicode >= 0x0041u) || (unicode == 0x003au))  //  ( ( unicode >= 'A' ) || ( unicode == ':' ) )
			{	//	':' [A-Z]
				return true;
			}
		}
		else if (unicode <= 0x007au)   //  ( unicode <= 'z' )
		{
			if ((unicode >= 0x0061u) || (unicode == 0x005fu))  //  ( ( unicode >= 'a' ) || ( unicode == '_' ) )
			{	//	'_' [a-z]
				return true;
			}
		}
	}
	else if (unicode >= 0x2070u)
	{
		if (unicode >= 0xf900u)
		{
			if (unicode >= 0xfffeu)
			{
				if ((unicode >= 0x00010000u) && (unicode <= 0x000effffu))
				{	//	[0x00010000-0x000effff]
					return true;
				}
			}
			else
			{
				if ((unicode <= 0xfdcfu) || (unicode >= 0xfdf0u))
				{	//	[0xf900-0xfdcf] [0xfdf0-0xfffd]
					return true;
				}
			}
		}
		else
		{
			if (unicode >= 0x2ff0u)
			{
				if ((unicode >= 0x3001u) && (unicode <= 0xd7ffu))
				{	//	[0x3001-0xd7ff]
					return true;
				}
			}
			else
			{
				if ((unicode <= 0x218fu) || (unicode >= 0x2c00u))
				{	//	[0x2070-0x218f] [0x2c00-0x2fef]
					return true;
				}
			}
		}
	}
	else if ((unicode == 0x200cu) || (unicode == 0x200du))
	{	//	[0x200c-0x200d]
		return true;
	}
	return false;
}

//! determine if a unicode code-point is an XML name extra character
bool isNameExtraXML(const unicode_t unicode) noexcept
{
	if (unicode <= 0x2040u)
	{
		if (unicode >= 0x002du)
		{
			if (unicode >= 0x0300u)
			{
				if ((unicode <= 0x036fu) || (unicode >= 0x203fu))
				{   //  [#x0300-#x036F] | [#x203F-#x2040
					return true;
				}
			}
			else if (unicode <= 0x0039u)
			{
				if (unicode != 0x002fu)
				{   //  "-" | "." | [0-9]
					return true;
				}
			}
			else if (unicode == 0x00b7u)
			{   //  #xB7
				return true;
			}
		}
	}
	return false;
}

//! determine if a unicode code-point is an XML name character
bool isNameXML(const unicode_t unicode) noexcept
{
	if (unicode < 0x2000u)
	{
		if (unicode >= 0x00c0u)
		{
			if ((unicode != 0x00d7u) && (unicode != 0x00f7u) && (unicode != 0x037eu))
			{	//	[0xc0-0xd6] [0xd8-0xf6] [0xf8-0x037d] [0x037f-0x1fff]
				return true;
			}
		}
		else if (unicode >= 0x0061u)   //  ( unicode >= 'a' )
		{
			if ((unicode <= 0x007au) || (unicode == 0x00b7u))  //  ( ( unicode <= 'z' ) || ( unicode == 0x00b7 ) )
			{	//	[a-z] [0xb7]
				return true;
			}
		}
		else if (unicode >= 0x0041u)   //  ( unicode >= 'A' )
		{
			if ((unicode <= 0x005au) || (unicode == 0x005fu))  //  ( ( unicode <= 'Z' ) || ( unicode == '_' ) )
			{	//	[A-Z] '_'
				return true;
			}
		}
		else if (unicode >= 0x002du)   //  ( unicode >= '-' )
		{
			if ((unicode <= 0x003au) && (unicode != 0x002fu))   //  ( ( unicode <= ':' ) && ( unicode != '/' ) )
			{	//	'-' '.' [0-9] ':'
				return true;
			}
		}
	}
	else if (unicode >= 0xf900u)
	{
		if (unicode >= 0xfffeu)
		{
			if ((unicode >= 0x00010000u) && (unicode <= 0x000effffu))
			{	//	[0x00010000-0x000effff]
				return true;
			}
		}
		else
		{
			if ((unicode <= 0xfdcfu) || (unicode >= 0xfdf0u))
			{	//	[0xf900-0xfdcf] [0xfdf0-0xfffd]
				return true;
			}
		}
	}
	else if (unicode >= 0x2070u)
	{
		if (unicode >= 0x2ff0u)
		{
			if ((unicode >= 0x3001u) && (unicode <= 0xd7ffu))
			{	//	[0x3001-0xd7ff]
				return true;
			}
		}
		else
		{
			if ((unicode <= 0x218fu) || (unicode >= 0x2c00u))
			{	//	[0x2070-0x218f] [0x2c00-0x2fef]
				return true;
			}
		}
	}
	else if ((unicode >= 0x200cu) && (unicode <= 0x2040u) && ((unicode <= 0x200du) || (unicode >= 0x203fu)))
	{	//	[0x200c-0x200d] [0x203f-0x2040]
		return true;
	}
	return false;
}

//!	determine if a unicode code-point is an XML post-name character
bool isPostNameXML(const unicode_t unicode) noexcept
{
	switch (unicode)
	{
		case(0x0009u):  //  '\t'
		case(0x000au):  //  '\n'
		case(0x000du):  //  '\r'
		case(0x0020u):  //  ' '
		case(0x0023u):  //  '#'
		case(0x0026u):  //  '&'
		case(0x002fu):  //  '/'
		case(0x003du):  //  '='
		case(0x003eu):  //  '>'
		case(0x003fu):  //  '?'
		case(0x005bu):  //  '['
		case(0x005du):  //  ']'
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

//! determine if a unicode code-point is an XML white-space character
bool isWhiteXML(const unicode_t unicode) noexcept
{   //  ' ', '\t', '\r', '\n'
	return (unicode == 0x0020u) || (unicode == 0x0009u) || (unicode == 0x000au) || (unicode == 0x000du);
}

//!	determine if a unicode code-point is unrestricted XML (in the allowed list and not in the discouraged list)
bool isCleanXML(const unicode_t unicode) noexcept
{
	if (unicode >= 0x0000u)
	{
		if (unicode <= 0x007eu)
		{
			if ((unicode >= 0x0020u) || (unicode == 0x0009u) || (unicode == 0x000au) || (unicode == 0x000du))
			{
				return true;
			}
		}
		else if (unicode >= 0xfdf0u)
		{
			if ((unicode & 0xffffu) <= 0xfffdu)
			{
				return true;
			}
		}
		else if (unicode >= 0x00a0u)
		{
			if ((unicode <= 0xd7ffu) || (unicode >= 0xe000u))
			{
				return true;
			}
		}
		else if (unicode == 0x0085u)
		{
			return true;
		}
	}
	return false;
}

// ==== unicode JSON classification functions ====

//! determine if a unicode code-point is a JSON white-space character (from RFC 7159)
bool isWhiteJSON(const unicode_t unicode) noexcept
{   //  ' ', '\t', '\r', '\n'
	return (unicode == 0x0020u) || (unicode == 0x0009u) || (unicode == 0x000au) || (unicode == 0x000du);
}

//! determine if a unicode code-point requires a JSON hex escape
bool isHexEscapedJSON(const unicode_t unicode) noexcept
{   //  needs output of the form "\uxxxx" where x is a hexadecimal character
	if (unicode >= 0x0000u)
	{
		if (unicode <= 0x009fu)
		{   //  possible c0 or c1 control code
			if (unicode <= 0x001fu)
			{   //  c0 control codes
				if ((unicode <= 0x0007u) || (unicode >= 0x000eu) || (unicode == 0x000bu))
				{   //  is c0 control code and not short-form escapable 
					return true;
				}
			}
			if (unicode >= 0x007fu)
			{   //  c1 control codes
				return true;
			}
		}
		else if ((unicode == 0x2028u) || (unicode == 0x2029u))
		{   //  unicode line terminators (for compatability with JavaScript)
			return true;
		}
	}
	return false;
}

};    //  namespace unicode
