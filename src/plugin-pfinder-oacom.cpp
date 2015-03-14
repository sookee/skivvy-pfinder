/*
 * plugin-pfinder-oacom.h
 *
 *  Created on: 17 Jan 2013
 *      Author: oasookee@googlemail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oasookee@googlemail.com               |
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

#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <thread>
#include <chrono>
#include <map>

#include <sstream>

#include <sookee/log.h>
#include <sookee/bug.h>
#include <sookee/ios.h>

//#include <skivvy/logrep.h>
#include <skivvy/plugin-pfinder-oacom.h>
#include <sookee/types/basic.h>

#define TIMEOUT 1000
#define MASTER_TIMEOUT 10000

namespace skivvy { namespace oacom {

using namespace skivvy;
//using namespace skivvy::utils;
using namespace sookee::types;
using namespace sookee::log;
using namespace sookee::bug;
using namespace sookee::ios;

/**
 * IPv4 IPv6 agnostic OOB (out Of Band) comms
 * @param cmd
 * @param packets Returned packets
 * @param host Host to connect to
 * @param port Port to connect on
 * @param wait Timeout duration in milliseconds
 * @return false if failed to connect/send or receive else true
 */
bool aocom(const str& cmd, str_vec& packets, const str& host, int port
	, siz wait = TIMEOUT)
{
	addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6
	hints.ai_socktype = SOCK_DGRAM;

	int status;
	addrinfo* res;
	if((status = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res)) != 0)
	{
		log(gai_strerror(status));
		return false;
	}

	st_time_point timeout = st_clk::now() + std::chrono::milliseconds(wait);

	// try to connect to each
	int cs;
	addrinfo* p;
	for(p = res; p; p = p->ai_next)
	{
		if((cs = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;
		if(!connect(cs, p->ai_addr, p->ai_addrlen))
			break;
		::close(cs);
	}

	freeaddrinfo(res);

	if(!p)
	{
		log("aocom: failed to connect: " << host << ":" << port);
		return false;
	}

	// cs good

	const str msg = "\xFF\xFF\xFF\xFF" + cmd;

	int n = 0;
	if((n = send(cs, msg.c_str(), msg.size(), 0)) < 0 || n < (int)msg.size())
	{
		log("cs send: " << strerror(errno));
		return false;
	}

	packets.clear();

	char buf[2048];

	n = sizeof(buf);
	while(n == sizeof(buf))
	{
		while((n = recv(cs, buf, sizeof(buf), MSG_DONTWAIT)) ==  -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
		{
			if(st_clk::now() > timeout)
			{
				log("socket timed out connecting to: " << host << ":" << port);
				return false;
			}
//			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		if(n < 0)
			log("cs recv: " << strerror(errno));
		if(n > 0)
			packets.push_back(str(buf, n));
	}

	close(cs);

	return true;
}

typedef unsigned char byte;

typedef std::vector<oa_server_t> oa_server_vec;

bool getservers(oa_server_vec& servers)
{
	str_vec packets;
	if(!aocom("getservers 71 empty full", packets, "dpmaster.deathmask.net", 27950, MASTER_TIMEOUT))
		return false;

	const str header = "\xFF\xFF\xFF\xFFgetserversResponse";

//	bug_var(header.size());

	if(packets.empty())
	{
		log("Empty response.");
		return false;
	}

	for(const str& packet: packets)
	{
//		bug_var(packet.size());

		if(packet.find(header) != 0)
		{
			log("Unrecognised response.");
			return false;
		}

		servers.clear();

		soss oss;
		byte srv[7];
		siss iss(packet.substr(header.size()), std::ios::binary);
		while(iss.read((char*)srv, 7))
		{
			if(srv[0] != '\\')
			{
				log("Expected '\\' aborting.");
				return false;
			};
			oss.clear();
			oss.str("");
			oss << std::dec << siz(srv[1]) << "." << siz(srv[2]) << "." << siz(srv[3]) << "." << siz(srv[4]);
			servers.push_back({oss.str(), siz(srv[5] << 8) | srv[6]});
		}
	}

	return true;
}

bool getstatus(const str& host, siz port, str_map& cvars, str_vec& players)
{
	str status;
	if(!getstatus(host, port, status))
		return false;

	str line;
	siss iss(status);
	if(sgl(iss, line) && !line.empty())
	{
		str key, val;
		siss iss(line.substr(1)); // skip initial '\\';
		while(sgl(sgl(iss, key, '\\'), val, '\\'))
		{
//			bug_var(key);
//			bug_var(val);
			cvars[key] = val;
		}
	}

	while(sgl(iss, line))
		players.push_back(line);

	return true;
}

bool getstatus(const str& host, siz port, str& status)
{
	str_vec packets;
	if(!aocom("getstatus\x0A", packets, host, port, TIMEOUT))
		return false;

	const str header = "\xFF\xFF\xFF\xFFstatusResponse\x0A";

	//bug_var(header.size());

	if(packets.empty())
	{
		log("Empty response.");
		return false;
	}

	status.clear();
	for(const str& packet: packets)
	{
		//bug_var(packet.size());

		if(packet.find(header) != 0)
		{
			log("Unrecognised response.");
			return false;
		}

		status.append(packet.substr(header.size()));
	}

	return true;
}

//int main()
//{
//	oa_server_vec servers;
//	getservers(servers);
//
//	bug_var(servers.size());
//
//	str status;
//	for(siz i = 0; i < servers.size() && i < 10; ++i)
//	{
//		std::cout << servers[i].host << ":" << servers[i].port << '\n';
//		if(getstatus(servers[i].host, servers[i].port, status))
//		{
//			str info, line;
//			siss iss(status);
//			sgl(iss, info);
//			while(sgl(iss, line))
//				std::cout << "status: " << line << '\n';
//		}
//	}
//}

}} // skivvy::oacom

