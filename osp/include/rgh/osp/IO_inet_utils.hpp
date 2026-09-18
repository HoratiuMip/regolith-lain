#pragma once /*
# FILE: osp/IO_inet_utils.hpp
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2024-2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Internet/Network utility.
*/
#include <rgh/brp/IO_port.hpp>
#include <rgh/brp/IO_utils.hpp>
#include <rgh/gep/dispenser.hpp>
#include <rgh/osp/core.hpp>

#include <expected>

namespace rgh::io {

std::expected< std::vector< ipv4_addr_t >, ret_t > ipv4_hosts_of( std::string_view domain_ ) noexcept; 

std::expected< ntp_packet_t, ret_t > ntp_get( Port& port_, bool make_unix_ = true ) noexcept; 

}//#namespace rgh::io