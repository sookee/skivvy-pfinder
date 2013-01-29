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

#ifndef _SKIVVY_PLUGIN_PFINDER_OACOM_H
#define _SKIVVY_PLUGIN_PFINDER_OACOM_H

#include <skivvy/types.h>

namespace skivvy { namespace oacom {

using namespace skivvy::types;

bool aocom(const str& cmd, str_vec& packets, const str& host, int port);

struct oa_server_t
{
	str host;
	siz port;
};

typedef std::vector<oa_server_t> oa_server_vec;

bool getservers(oa_server_vec& servers);
bool getstatus(const str& host, siz port, str& status);

}} // skivvy::oacom

#endif // _SKIVVY_PLUGIN_PFINDER_OACOM_H
