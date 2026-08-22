
//  Copyright (c) 2010-2026 Ritchie Brannan
//  License: MIT (see LICENSE file in repository root)
//
//  SuiteUTF
//  Original design 2010-2016; maintained and extended 2024-2025.
//  Copyright (c) 2010-2025 Ritchie Brannan.
//  MIT License. See LICENSE.txt. Project history: docs/History.md.
//
//  File:   text_hash.h
//  Author: Ritchie Brannan
//  Date:   11 Nov 10
//  
//  Description:
//  
//  	CCITT-16 based text hashing functions.

#pragma once

#ifndef	__TEXT_HASH_INCLUDED__
#define	__TEXT_HASH_INCLUDED__

#include <cstdint>

// ==== 16-bit crc to 32-bit ascii hash transformation functions ====
bool is_valid_ascii_hash(const uint32_t ascii_hash) noexcept;
inline constexpr uint16_t ascii_hash_to_crc(const uint32_t ascii_hash) noexcept;
inline constexpr uint32_t crc_to_ascii_hash(const uint16_t crc) noexcept;

// ==== 16-bit crc ccitt false crc calculation ====
//  Use the null-terminated helpers for conventional strings and the
//  length-aware helper when data may include embedded null bytes.
uint16_t crc_ccitt_false(const uint8_t* const text) noexcept;
uint16_t crc_ccitt_false(const uint8_t* const text, const uint32_t length) noexcept;
inline uint32_t crc_ccitt_false_ascii_hash(const uint8_t* const text) noexcept { return crc_to_ascii_hash(crc_ccitt_false(text)); };
inline uint32_t crc_ccitt_false_ascii_hash(const uint8_t* const text, const uint32_t length) noexcept { return crc_to_ascii_hash(crc_ccitt_false(text, length)); };

// ==== inline pointer type conversion helper functions ====
inline uint16_t crc_ccitt_false(const char* const text) noexcept { return crc_ccitt_false(reinterpret_cast<const uint8_t* const>(text)); };
inline uint16_t crc_ccitt_false(const char* const text, const uint32_t length) noexcept { return crc_ccitt_false(reinterpret_cast<const uint8_t* const>(text), length); };
inline uint32_t crc_ccitt_false_ascii_hash(const char* const text) noexcept { return crc_ccitt_false_ascii_hash(reinterpret_cast<const uint8_t* const>(text)); };
inline uint32_t crc_ccitt_false_ascii_hash(const char* const text, const uint32_t length) noexcept { return crc_ccitt_false_ascii_hash(reinterpret_cast<const uint8_t* const>(text), length); };

// ==== test functions ====
bool test_ascii_hash();
bool test_crc_ccitt_false();

// ==== inline function bodies for the 16-bit crc to 32-bit ascii hash transformation functions ====

constexpr uint16_t ascii_hash_to_crc(const uint32_t ascii_hash) noexcept
{	//	converts 4 Ascii hex characters (0-9, A-F) stored in a uint32_t to a 16-bit CRC
	uint32_t decode = (ascii_hash & 0x0f0f0f0fu) + (((ascii_hash & 0x40404040u) >> 6u) * 9u);
	return static_cast<uint16_t>(((decode >> 12) & 0xf000u) | ((decode >> 8) & 0x0f00u) | ((decode >> 4) & 0x00f0u) | (decode & 0x000fu));
}

constexpr uint32_t crc_to_ascii_hash(const uint16_t crc) noexcept
{	//	converts a 16-bit CRC to 4 Ascii hex characters (0-9, A-F) stored in a uint32_t
	uint32_t hash = static_cast<uint32_t>(crc);
	hash = ((hash << 8) | hash) & 0x00ff00ffu;
	hash = ((hash << 4) | hash) & 0x0f0f0f0fu;
	return hash + 0x30303030u + ((((hash + 0x06060606u) >> 4) & 0x01010101u) * 7u);
}

#endif	//	#ifndef	__TEXT_HASH_INCLUDED__

