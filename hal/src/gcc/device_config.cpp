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
#include "ota_flash_hal.h"
#include "../../../system/inc/system_info.h" // FIXME

#include <cstdlib>
#include <fstream>
#include <istream>
#include <iostream>

#include <boost/format.hpp>
#include <boost/algorithm/hex.hpp>
#include "boost_program_options_wrap.h"
#include "boost_json.h"

using namespace particle;
using namespace particle::config;
using namespace particle::system;

namespace po = boost::program_options;

DeviceConfig deviceConfig;

namespace {

const char* CMD_HELP = "help";
const char* CMD_VERSION = "version";

// These are still supported by the Cloud
const int PHOTON_PLATFORM_ID = 6;
const int P1_PLATFORM_ID = 8;

std::istream& operator>>(std::istream& in, ProtocolFactory& pf)
{
	std::string value;
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

    std::string parse_options(int ac, char* av[], po::options_description& command_line_options)
    {
        std::string command;

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
            ("help,h", po::value<std::string>(&command)->implicit_value(CMD_HELP), "display the available options")
            ("version", po::value<std::string>(&command)->implicit_value(CMD_VERSION), "display the program version")
        ;

        static_assert(NO_LOG_LEVEL==70, "need to update the range below.");
        device_options.add_options()
            ("verbosity,v", po::value<uint16_t>(&config.log_level)->default_value(0)->notifier(range(0,NO_LOG_LEVEL,"verbosity")), "verbosity (0-70)")
            ("device_id,id", po::value<std::string>(&config.device_id), "the device ID")
            ("platform_id", po::value<uint16_t>(&config.platform_id)->default_value(PLATFORM_ID), "the platform ID")
            ("device_key,dk", po::value<std::string>(&config.device_key)->default_value("device_key.der"), "the filename containing the device private key")
            ("server_key,sk", po::value<std::string>(&config.server_key)->default_value("server_key.der"), "the filename containing the server public key")
            ("describe", po::value<std::string>(&config.describe), "the filename containing the device description")
			("protocol,p", po::value<ProtocolFactory>(&config.protocol)->default_value(PROTOCOL_LIGHTSSL), "the cloud communication protocol to use")
			;

        command_line_options.add(program_options).add(device_options);

        po::options_description config_file_options;
        config_file_options.add(device_options);

        po::options_description environment_options;
        environment_options.add(device_options);

        std::string config_file = "vdev.conf";
        std::ifstream ifs(config_file.c_str());
        if (ifs) {
            po::store(po::parse_config_file(ifs, config_file_options), vm);
        }

        po::store(po::parse_command_line(ac, av, command_line_options), vm);
        po::store(po::parse_environment(environment_options, "VDEV_"), vm);
        po::notify(vm);

        return command;
    }
};

uint8_t hex2dec(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0xFF;
}


size_t hex2bin(const std::string& hex, uint8_t* dest, size_t destLen)
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

} // namespace

std::string Describe::toString() const {
    if (!isValid()) {
        throw std::runtime_error("Cannot serialize an invalid object");
    }
    boost::json::object jsonDesc;
    // Platform ID
    jsonDesc["p"] = platformId_.value();
    // Modules
    boost::json::array jsonModules;
    for (auto& module: modules_) {
        boost::json::object jsonModule;
        // Function
        jsonModule["f"] = module_function_string(module.function());
        // Index
        jsonModule["n"] = std::to_string(module.index());
        // Version
        jsonModule["v"] = module.version();
        // Dependencies
        boost::json::array jsonDeps;
        for (auto& dep: module.dependencies()) {
            boost::json::object jsonDep;
            jsonDep["f"] = module_function_string(dep.function());
            jsonDep["n"] = std::to_string(dep.index());
            jsonDep["v"] = dep.version();
            jsonDeps.push_back(jsonDep);
        }
        jsonModule["d"] = jsonDeps;
        // Store
        jsonModule["l"] = module_store_string(module.store());
        // Maximum size
        jsonModule["s"] = module.maximumSize();
        // Validity flags
        jsonModule["vc"] = module.validityChecked();
        jsonModule["vv"] = module.validityResult();
        // Hash
        jsonModule["u"] = boost::algorithm::hex(module.hash());
        jsonModules.push_back(jsonModule);
    }
    jsonDesc["m"] = jsonModules;
    return boost::json::serialize(jsonDesc);
}

Describe Describe::fromString(std::string_view str) {
    auto json = boost::json::parse(std::string(str)); // std::string_view is not convertible to boost::json::string_view for some reason
    auto& jsonDesc = json.as_object();
    Describe desc;
    // Platform ID
    desc.platformId(jsonDesc.at("p").as_int64());
    auto& jsonModules = jsonDesc.at("m").as_array();
    // Modules
    Describe::Modules modules;
    for (size_t i = 0; i < jsonModules.size(); ++i) {
        auto& jsonModule = jsonModules.at(i).as_object();
        ModuleInfo module;
        // Function
        auto& jsonFunc = jsonModule.at("f").as_string();
        int func = module_function_from_string(jsonFunc.data());
        if (func < 0) {
            throw std::runtime_error("Unknown module function");
        }
        module.function((module_function_t)func);
        // Index
        auto& jsonIndexStr = jsonModule.at("n").as_string();
        module.index(std::stoi(std::string(jsonIndexStr)));
        // Version
        module.version(jsonModule.at("v").as_int64());
        // Dependencies
        auto& jsonDeps = jsonModule.at("d").as_array();
        ModuleInfo::Dependencies deps;
        for (size_t i = 0; i < jsonDeps.size(); ++i) {
            auto& jsonDep = jsonDeps.at(i).as_object();
            ModuleDependencyInfo dep;
            auto& jsonFunc = jsonDep.at("f").as_string();
            int func = module_function_from_string(jsonFunc.data());
            if (func < 0) {
                throw std::runtime_error("Unknown module function");
            }
            dep.function((module_function_t)func);
            auto& jsonIndexStr = jsonDep.at("n").as_string();
            dep.index(std::stoi(std::string(jsonIndexStr)));
            module.version(jsonModule.at("v").as_int64());
            deps.push_back(dep);
        }
        // Store (optional)
        if (jsonModule.contains("l")) {
            auto& jsonStore = jsonModule.at("l").as_string();
            int store = module_store_from_string(jsonStore.data());
            if (store < 0) {
                throw std::runtime_error("Unknown module store");
            }
            module.store((module_store_t)store);
        } else {
            module.store(MODULE_STORE_MAIN);
        }
        // Maximum size (optional)
        if (jsonModule.contains("s")) {
            module.maximumSize(jsonModule.at("s").as_int64());
        } else {
            module.maximumSize(HAL_OTA_FlashLength());
        }
        // Validity flags (optional)
        if (jsonModule.contains("vc")) {
            module.validityChecked(jsonModule.at("vc").as_int64());
        } else {
            module.validityChecked(MODULE_VALIDATION_INTEGRITY | MODULE_VALIDATION_DEPENDENCIES | MODULE_VALIDATION_RANGE |
                    MODULE_VALIDATION_PLATFORM);
        }
        if (jsonModule.contains("vv")) {
            module.validityResult(jsonModule.at("vv").as_int64());
        } else {
            module.validityResult(module.validityChecked());
        }
        // Hash (optional)
        if (jsonModule.contains("u")) {
            auto& jsonHash = jsonModule.at("u").as_string();
            module.hash(boost::algorithm::unhex(jsonHash));
        } else {
            module.hash(std::string(32, '\0')); // SHA-256
        }
        modules.push_back(module);
    }
    desc.modules(modules);
    return desc;
}

bool read_device_config(int argc, char* argv[])
{
    ConfigParser parser;

    po::options_description options;
    std::string command = parser.parse_options(argc, argv, options);

    if (command==CMD_HELP) {
        std::cout << options << std::endl;
        return false;
    }
    else if (command==CMD_VERSION) {
        std::cout << boost::format("virtual device version 0.0.1 [build date %s %s]") % __DATE__ % __TIME__ << std::endl;
        return false;
    }

    deviceConfig.read(parser.config);
    return true;
}

void DeviceConfig::read(Configuration& configuration)
{
    size_t length = configuration.device_id.length();
    if (length!=24) {
        throw std::invalid_argument(std::string("expected device ID of length 24, got ") + '\'' + configuration.device_id + '\'');
    }

    hex2bin(configuration.device_id, device_id, sizeof(device_id));

    read_file(configuration.device_key.c_str(), device_key, sizeof(device_key));
    read_file(configuration.server_key.c_str(), server_key, sizeof(server_key));

    this->protocol = configuration.protocol;
    this->platform_id = configuration.platform_id;

    if (!configuration.describe.empty()) {
        auto s = read_file(configuration.describe);
        this->describe = Describe::fromString(s);
        this->platform_id = this->describe.platformId();
        if (this->platform_id == PLATFORM_GCC || this->platform_id == PHOTON_PLATFORM_ID || this->platform_id == P1_PLATFORM_ID) {
            this->protocol = PROTOCOL_LIGHTSSL;
        } else {
            this->protocol = PROTOCOL_DTLS;
        }
    }

    setLoggerLevel(LoggerOutputLevel(NO_LOG_LEVEL-configuration.log_level));
}
