
//  Copyright (c) 2010-2026 Ritchie Brannan
//  License: MIT (see LICENSE file in repository root)
//
//  SuiteUTF
//  Original design 2010-2016; maintained and extended 2024-2025.
//  Copyright (c) 2010-2025 Ritchie Brannan.
//  MIT License. See LICENSE.txt. Project history: docs/History.md.
//
//  File:   unicode_type.h
//  Author: Ritchie Brannan
//  Date:   11 July 16
//  
//  Description:
//  
//      Defines the unicode_t data type.

#pragma once

#ifndef __UNICODE_TYPE_INCLUDED__
#define __UNICODE_TYPE_INCLUDED__

#include <cstdint>

namespace unicode
{

/// unicode data type
using unicode_t = int32_t;

};  //  namespace unicode

#endif  //  #ifndef __UNICODE_TYPE_INCLUDED__
