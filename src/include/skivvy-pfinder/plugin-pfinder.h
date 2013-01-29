#pragma once
#ifndef _SOOKEE_IRCBOT_PFINDER_H_
#define _SOOKEE_IRCBOT_PFINDER_H_
/*
 * ircbot-pfinder.h
 *
 *  Created on: 07 Jul 2011
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oaskivvy@gmail.com               |
'------------------------------------------------------------------'

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

http://www.gnu.org/licenses/gpl-2.0.html

'-----------------------------------------------------------------*/

#include <skivvy/ircbot.h>

#include <skivvy/network.h>

#include <chrono>

namespace skivvy { namespace ircbot {

str html_handle_to_irc(str html);

/**
 *
 */
class PFinderIrcBotPlugin
: public BasicIrcBotPlugin
, public IrcBotRPCService
{
private:

	struct player
	{
		str ping;
		str frags;
		str handle;
	};

	struct server
	{
		siz uid;
		str address;
		str players;
		str map;
		str gametype;
		str name;

		bool operator<(const server& s) const { return name < s.name; }
	};

	struct scache
	{
		// TODO: add "last seen" stamp
		siz uid;
		str name;
		str match;
		str print;
		str address;
		str gametype;

		scache& operator=(const server& s)
		{
			name = s.name;
			match = net::html_to_text(s.name);
			print = html_handle_to_irc(s.name);
			address = s.address;
			gametype = s.gametype;
			return *this;
		}

		friend std::ostream& operator<<(std::ostream& os, const scache& s)
		{
			os << s.uid << '\n';
			os << s.name << '\n';
			os << s.match << '\n';
			os << s.print << '\n';
			os << s.address << '\n';
			os << s.gametype;
			return os;
		}
		friend std::istream& operator>>(std::istream& is, scache& s)
		{
			is >> s.uid >> std::ws;
			sgl(is, s.name);
			sgl(is, s.match);
			sgl(is, s.print);
			sgl(is, s.address);
			sgl(is, s.gametype);
			return is;
		}
	};

	struct oasdata
	{
		str host;
		siz port;
		siz uid;
		str name;
		str sv_hostname;
		str hostname; // sv_hostname stripped of OA color codes
		siz attempts; // connection attempts
		siz us; // average connection time in mucroseconds
		oasdata(): attempts(0), us(0) {}

		bool operator<(const oasdata& oasd) const
		{
			return uid < oasd.uid;
		}
		bool operator==(const oasdata& oasd) const
		{
			return uid == oasd.uid;
		}
		bool operator!=(const oasdata& oasd) const
		{
			return !operator==(oasd);
		}
	};

	typedef std::vector<oasdata> oasdata_vec;
	typedef std::map<str, oasdata> oasdata_map;
	typedef std::pair<const str, oasdata> oasdata_pair;

	bool read_servers(const message& msg, oasdata_map& m);
	bool read_servers(const message& msg, oasdata_map& m, siz& uid);
	bool write_servers(const message& msg, const oasdata_map& m, siz uid);
	// RPC Services

	/**
	 * The supplied parameter may resolve into a list of handles
	 * or just one
	 *
	 * @return true if a substitution took place or false if the original
	 * search term was returned in the vector.
	 **/
	bool lookup_players(const str& search, std::vector<str>& handles);
	std::vector<str> oafind(const str handle);
	std::vector<str> xoafind(const str handle);
	void check_tell();

	/**
	 * perform exact matching on substitutions
	 * and case insensitive substring matching otherwise
	 **/
	bool match_player(bool substitution, const str& name1, const str& name2);

	// utilities

	str::size_type extract_server(const str& line, server& s, str::size_type pos = 0) const;
	str::size_type extract_player(const str& line, player& p, str::size_type pos = 0) const;

	void read_links_file(str_set_map& links);
	void write_links_file(const str_set_map& links);

	// Bot Commands

//	void oarcon(const message& msg);
//	void oarconmsg(const message& msg);

	bool links_changed = false;
	st_time_point servers_cached;

	void cvar(const message& msg);
	void oafind(const message& msg);
	void oalink(const message& msg);
	void oaunlink(const message& msg);
	void oalist(const message& msg);
	void oatell(const message& msg);
	bool oaserver(const message& msg);
	bool oaslist(const message& msg);
	bool oasinfo(const message& msg);
	bool oasname(const message& msg);

public:
	PFinderIrcBotPlugin(IrcBot& bot);
	virtual ~PFinderIrcBotPlugin();

	// INTERFACE: BasicIrcBotPlugin

	virtual bool initialize();

	// INTERFACE: IrcBotPlugin

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;
	virtual void exit();

	// INTERFACE RPC

	virtual bool rpc(rpc::call& c);
	virtual call_ptr create_call() const { return call_ptr(new rpc::local_call); }
};

}} // sookee::ircbot

#endif // _SOOKEE_IRCBOT_PFINDER_H_
