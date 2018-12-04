/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "at_parser.h"

#include "at_command.h"
#include "at_parser_impl.h"

#include "check.h"

#include <cstdarg>

namespace particle {

using detail::AtParserImpl;

AtParser::AtParser() {
}

AtParser::AtParser(AtParser&& parser) :
        p_(std::move(parser.p_)) {
}

AtParser::~AtParser() {
}

int AtParser::init(AtParserConfig conf) {
    CHECK_FALSE(p_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(AtParserImpl::isConfigValid(conf), SYSTEM_ERROR_INVALID_ARGUMENT);
    p_.reset(new(std::nothrow) AtParserImpl(std::move(conf)));
    CHECK_TRUE(p_, SYSTEM_ERROR_NO_MEMORY);
    return 0;
}

void AtParser::destroy() {
    p_.reset();
}

AtCommand AtParser::command() {
    if (!p_) {
        return AtCommand(SYSTEM_ERROR_INVALID_STATE);
    }
    const int ret = p_->newCommand();
    if (ret < 0) {
        return AtCommand(ret);
    }
    return AtCommand(p_.get());
}

AtResponse AtParser::sendCommand(const char* fmt, ...) {
    AtCommand cmd = command();
    va_list args;
    va_start(args, fmt);
    cmd.vprintf(fmt, args);
    va_end(args);
    return cmd.send();
}

AtResponse AtParser::sendCommand(unsigned timeout, const char* fmt, ...) {
    AtCommand cmd = command();
    cmd.timeout(timeout);
    va_list args;
    va_start(args, fmt);
    cmd.vprintf(fmt, args);
    va_end(args);
    return cmd.send();
}

int AtParser::execCommand(const char* fmt, ...) {
    AtCommand cmd = command();
    va_list args;
    va_start(args, fmt);
    cmd.vprintf(fmt, args);
    va_end(args);
    return cmd.exec();
}

int AtParser::execCommand(unsigned timeout, const char* fmt, ...) {
    AtCommand cmd = command();
    cmd.timeout(timeout);
    va_list args;
    va_start(args, fmt);
    cmd.vprintf(fmt, args);
    va_end(args);
    return cmd.exec();
}

int AtParser::addUrcHandler(const char* prefix, UrcHandler handler, void* data) {
    CHECK_TRUE(p_, SYSTEM_ERROR_INVALID_STATE);
    return p_->addUrcHandler(prefix, handler, data);
}

void AtParser::removeUrcHandler(const char* prefix) {
    if (p_) {
        p_->removeUrcHandler(prefix);
    }
}

int AtParser::processUrc(unsigned timeout) {
    CHECK_TRUE(p_, SYSTEM_ERROR_INVALID_STATE);
    return p_->processUrc(timeout);
}

void AtParser::reset() {
    if (p_) {
        p_->reset();
    }
}

void AtParser::echoEnabled(bool enabled) {
    if (p_) {
        p_->echoEnabled(enabled);
    }
}

void AtParser::logEnabled(bool enabled) {
    if (p_) {
        p_->logEnabled(enabled);
    }
}

AtParserConfig AtParser::config() const {
    if (!p_) {
        return AtParserConfig();
    }
    return p_->config();
}

} // particle
