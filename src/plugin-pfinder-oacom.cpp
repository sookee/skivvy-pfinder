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

#include <skivvy/logrep.h>
#include <skivvy/plugin-pfinder-oacom.h>
#include <skivvy/types.h>

#define TIMEOUT 500
#define MASTER_TIMEOUT 10000

namespace skivvy { namespace oacom {

using namespace skivvy;
using namespace skivvy::utils;
using namespace skivvy::types;

bool aocom(const str& cmd, str_vec& packets, const str& host, int port
	, siz wait = TIMEOUT)
{
//	bug_func();
//	bug_var(cmd);
//	bug_var(host);
//	bug_var(port);
	// One mutex per server:port to ensure that all threads
	// accessing the same server:port pause for a minimum time
	// between calls to avoid flood protection.
	static std::map<str, std::unique_ptr<std::mutex>> mtxs;

	int cs = socket(PF_INET, SOCK_DGRAM, 0);
	int cs_flags = fcntl(cs, F_GETFL, 0);

	if(cs <= 0)
	{
		log(strerror(errno));
		return false;
	}

	sockaddr_in sin;
	hostent* he = gethostbyname(host.c_str());
	std::copy(he->h_addr, he->h_addr + he->h_length
		, reinterpret_cast<char*>(&sin.sin_addr.s_addr));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

//	bug("Setting non-blocking");
	fcntl(cs, F_SETFL, cs_flags | O_NONBLOCK);

//	bug("connecting to : " << host << ":" << port);

	st_time_point timeout = st_clk::now() + std::chrono::milliseconds(wait);

	int n = 0;
	while((n = connect(cs, (struct sockaddr *) &sin, sizeof(sin))) < 0 && errno == EINPROGRESS)
	{
		if(st_clk::now() > timeout)
		{
			log("socket timed out connecting to: " << host << ":" << port);
			return false;
		}
		std::this_thread::yield();
	}

	if(n < 0)
	{
		log(strerror(errno) << ": " << host << ":" << port);
		return false;
	}

//	bug("Non-blocking off");
	fcntl(cs, F_SETFL, cs_flags);


//	bug("Setting non-blocking");
//	fcntl(cs, F_SETFL, O_NONBLOCK);

//	const str key = host + ":" + std::to_string(port);

//	if(!mtxs[key].get())
//		mtxs[key].reset(new std::mutex);

	// keep out all threads for the same server:port until the minimum time
	// has elapsed
//	lock_guard lock(*mtxs[key]);
//	st_time_point pause = st_clk::now() + std::chrono::milliseconds(1000);

	const str msg = "\xFF\xFF\xFF\xFF" + cmd;

//	bug("about to send...");
	if((n = send(cs, msg.c_str(), msg.size(), 0)) < 0 || n < (int)msg.size())
	{
		log("cs send: " << strerror(errno));
		return false;
	}
//	bug("done!");

	packets.clear();

	char buf[2048];

//	bug("about to recv...");
	n = sizeof(buf);
	while(n == sizeof(buf))
	{
		timeout = st_clk::now() + std::chrono::milliseconds(wait);
		while((n = recv(cs, buf, sizeof(buf), MSG_DONTWAIT)) ==  -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
		{
			if(st_clk::now() > timeout)
			{
				log("socket timed out connecting to: " << host << ":" << port);
				return false;
			}
			std::this_thread::yield();
		}
	//		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if(n < 0)
			log("cs recv: " << strerror(errno));
		if(n > 0)
			packets.push_back(str(buf, n));
	}
//	bug("done!");
//	bug(packets.size());

	close(cs);

//	std::this_thread::sleep_until(pause);

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

	bug_var(header.size());

	if(packets.empty())
	{
		log("Empty response.");
		return false;
	}

	for(const str& packet: packets)
	{
		bug_var(packet.size());

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
//		status.assign(packet.substr(header.size()));
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

