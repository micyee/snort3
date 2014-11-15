/*
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
// config_ignore_ports.cc author Josh Rosenbaum <jrosenba@cisco.com>

#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>

#include "conversion_state.h"
#include "helpers/converter.h"
#include "helpers/s2l_util.h"
#include "helpers/util_binder.h"

namespace config
{

namespace {

constexpr uint16_t MAX_PORTS = 0xFFFF; // == 65535

class IgnorePorts : public ConversionState
{
public:
    IgnorePorts(Converter& c) : ConversionState(c) {};
    virtual ~IgnorePorts() {};
    virtual bool convert(std::istringstream& data_stream);
};


} // namespace

bool IgnorePorts::convert(std::istringstream& data_stream)
{
    Binder bind(table_api);
    bool retval = true;
    std::string keyword;
    std::string port;


    std::streamoff pos = data_stream.tellg();
    std::string port_string = data_stream.str();
    port_string = port_string.substr(pos);
    port_string = data_api.expand_vars(port_string);


    // if the keyword is not 'tcp' or 'udp', return false;
    if (!(data_stream >> keyword) ||
        (keyword.compare("udp") && keyword.compare("tcp")) )
    {
        data_api.failed_conversion(data_stream, keyword);
        return false;
    }

    bind.set_when_proto(keyword);

    while (data_stream >> port)
    {
        try
        {
            const std::size_t colon_pos = port.find(':');

            if (!port.compare("any"))
            {
                // Possible Snort bug, but only port zero is ignrored
                bind.add_when_port("0");
            }
            else if (colon_pos == std::string::npos)
            {
                bind.add_when_port(port);
            }
            else if (colon_pos == 0)
            {
                int high = std::stoi(port.substr(1));

                if (high > MAX_PORTS)
                    throw std::out_of_range("");

                for (int i = 0; i <= high; i++)
                    bind.add_when_port(std::to_string(i));
            }

            else if ((colon_pos+1)  == port.size())
            {
                int low = std::stoi(port.substr(0, colon_pos));

                if (low > MAX_PORTS)
                    throw std::out_of_range("");

                for (int i = low; i <= MAX_PORTS; i++)
                    bind.add_when_port(std::to_string(i));
            }
            else
            {
                int low = std::stoi(port.substr(0, colon_pos));
                int high = std::stoi(port.substr(colon_pos + 1));

                if (low > MAX_PORTS || high > MAX_PORTS || low > high)
                    throw std::out_of_range("");

                for (int i = low; i <= high; i++)
                    bind.add_when_port(std::to_string(i));
            }
        }
        catch(std::invalid_argument)
        {
            data_api.failed_conversion(data_stream, "can't convert " + port);
            retval = false;
        }
        catch(std::out_of_range)
        {
            data_api.failed_conversion(data_stream, "Port" + port + " must be <= 65535");
            retval = false;
        }
    }

    bind.set_use_action("allow");
    return retval;
}

/**************************
 *******  A P I ***********
 **************************/

static ConversionState* ctor(Converter& c)
{ return new IgnorePorts(c); }

static const ConvertMap config_ignore_ports =
{
    "ignore_ports",
    ctor,
};

const ConvertMap* ignore_ports_map = &config_ignore_ports;

} // namespace config
