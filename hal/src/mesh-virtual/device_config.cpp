/**
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "device_config.h"
#include "core_msg.h"
#include "filesystem.h"
#include <cstdlib>
#include <fstream>
#include <istream>
#include <iostream>

#include "boost_program_options_wrap.h"
#include <boost/format.hpp>

namespace po = boost::program_options;

using namespace std;

const char* DEVICE_ID = "device_id";
const char* STATE_DIR = "state";

DeviceConfig deviceConfig;

const char* CMD_HELP = "help";
const char* CMD_VERSION = "version";

std::istream& operator>>(std::istream& in, ProtocolFactory& pf)
{
	string value;
	in >> value;
	if (value=="tcp")
		pf = PROTOCOL_LIGHTSSL;
	else if (value=="udp")
		pf = PROTOCOL_DTLS;
	else
		throw boost::program_options::invalid_option_value(value);
	return in;
}

class ConfigParser
{

public:

    po::variables_map vm;
    Configuration config;


    ConfigParser()
    {
    }

    string parse_options(int ac, char* av[], po::options_description& command_line_options)
    {
        string command;

        po::options_description program_options("Program options");
        po::options_description device_options("Device Options");

        auto range = [](int min, int max, char const * const opt_name){
            return [opt_name, min, max](unsigned short v){
              if(v < min || v > max){
                throw po::validation_error
                  (po::validation_error::invalid_option_value,
                   opt_name, std::to_string(v));
              }
            };
          };

        program_options.add_options()
            ("help,h", po::value<string>(&command)->implicit_value(CMD_HELP), "display the available options")
            ("version", po::value<string>(&command)->implicit_value(CMD_VERSION), "display the program version")
        ;

        static_assert(NO_LOG_LEVEL==70, "need to update the range below.");
        device_options.add_options()
            ("verbosity,v", po::value<uint16_t>(&config.log_level)->default_value(0)->notifier(range(0,NO_LOG_LEVEL,"verbosity")), "verbosity (0-70)")
            ("device_id,id", po::value<string>(&config.device_id), "the device ID")
            ("device_key,dk", po::value<string>(&config.device_key)->default_value("device_key.der"), "the filename containing the device private key")
            ("server_key,sk", po::value<string>(&config.server_key)->default_value("server_key.der"), "the filename containing the server public key")
            ("state,s", po::value<string>(&config.periph_directory)->default_value("state"), "the directory where device state and peripherals is stored")
			("protocol,p", po::value<ProtocolFactory>(&config.protocol)->default_value(PROTOCOL_LIGHTSSL), "the cloud communication protocol to use")
			;

        command_line_options.add(program_options).add(device_options);

        po::options_description config_file_options;
        config_file_options.add(device_options);

        po::options_description environment_options;
        environment_options.add(device_options);

        string config_file = "vdev.conf";
        ifstream ifs(config_file.c_str());
        if (ifs) {
            po::store(po::parse_config_file(ifs, config_file_options), vm);
        }

        po::store(po::parse_command_line(ac, av, command_line_options), vm);
        po::store(po::parse_environment(environment_options, "VDEV_"), vm);
        po::notify(vm);

        return command;
    }
};



using std::string;

string get_configuration_value(const char* name, string default_value)
{
    char* value = getenv(name);
    return value ? value : default_value;
}

void read_config_file(const char* config_name, void* data, size_t length)
{
    string file_name = get_configuration_value(config_name, "");
    if (file_name.length())
    {
        read_file(file_name.c_str(), data, length);
    }
}

uint8_t hex2dec(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0xFF;
}


size_t DeviceConfig::hex2bin(const std::string& hex, uint8_t* dest, size_t destLen)
{
    uint8_t len = hex.length()>>1;
    if (dest && destLen<len)
        len = destLen;
    if (dest) {
        for (int i=0; i<len; i++) {
            char c1 = hex[i*2];
            char c2 = hex[i*2+1];
            uint8_t b = hex2dec(c1) << 4 | hex2dec(c2);
            dest[i] = b;
        }
    }
    return len;
}


bool read_device_config(int argc, char* argv[])
{
    ConfigParser parser;

    po::options_description options;
    string command = parser.parse_options(argc, argv, options);

    if (command==CMD_HELP) {
        cout << options << endl;
        return false;
    }
    else if (command==CMD_VERSION) {
        cout << boost::format("virtual device version 0.0.1 [build date %s %s]") % __DATE__ % __TIME__ << endl;
        return false;
    }

    deviceConfig.read(parser.config);
    return true;
}

void DeviceConfig::read(Configuration& configuration)
{
#ifndef SPARK_NO_CLOUD
    size_t length = configuration.device_id.length();
    if (length!=24) {
        throw std::invalid_argument(std::string("expected device ID of length 24 from config ") + DEVICE_ID + ", got: '"+configuration.device_id+ "'");
    }

    hex2bin(configuration.device_id, device_id, sizeof(device_id));

    read_file(configuration.device_key.c_str(), device_key, sizeof(device_key));
    read_file(configuration.server_key.c_str(), server_key, sizeof(server_key));
#endif

    setLoggerLevel(LoggerOutputLevel(NO_LOG_LEVEL-configuration.log_level));

    this->protocol = configuration.protocol;
}

