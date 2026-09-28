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
/// Retrieve all available hosts for the given domain.
///
/// @param domain_ The domain for which to retrieve the hosts.
/// @return An expected vector of IPv4 addresses, error code otherwise.
std::expected< std::vector< ipv4_addr_t >, ret_t > ipv4_hosts_of( std::string_view domain_ ) noexcept; 

/// Make a simple query using Network Time Protocol over the given port.
///
/// The retrieved packet is populated entirely by the NTP server, therefore to read the time you need to read
///   the server transmission timestamp.
///
/// @param port_ The port on which to attempt the query.
/// @param make_unix_ Whether to make the timestamps inside the NTP packet UNIX.
/// @return An expected NTP packet, error code otherwise.
std::expected< ntp_packet_t, ret_t > ntp_get( Port& port_, bool make_unix_ = true ) noexcept; 

}//#namespace rgh::io