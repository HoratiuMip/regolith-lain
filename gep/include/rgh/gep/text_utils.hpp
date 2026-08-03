#pragma once /*
# FILE: gep/text_utils.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Package of string utility functions.
*/

#include <rgh/gep/core.hpp>

namespace rgh {

/*
# Levenshtein distance between two strings each of any length.
*/
int lev_dist( std::string_view s_, std::string_view t_ );


/*
# A quick string hasher, usable also for mapping and switching.
*/
constexpr uint32_t txt_hash( std::string_view str_ ) {
    uint32_t h = 2166136261U;
    for( char c : str_ ) {
        h ^= (uint32_t)c;
        h *= 16777619U;
    }
    return h;
}
constexpr uint32_t txt_hash( char c_ ) {
    return (2166136261U ^ ( uint32_t )c_) * 16777619U;
}
static_assert( txt_hash( 'R' ) == txt_hash( "R" ) );

};
