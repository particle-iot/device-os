/**
  ******************************************************************************
  Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.

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

#ifndef SPARK_WIRING_PROCESS_H
#define	SPARK_WIRING_PROCESS_H

#include "spark_wiring_platform.h"

#if Wiring_Process == 1

#include "spark_wiring_stream.h"
#include <signal.h>
#include <memory>
#include <deque>

class FdStream;
class FdPrint;

class Process
{
public:
  Process();
  static Process run(String command);

  int pid();
  void kill(int signal = SIGTERM);
  bool exited();
  uint8_t wait();
  uint8_t exitCode();
  FdStream& out();
  FdStream& err();
  FdPrint& in();

  static const uint8_t COMMAND_NOT_FOUND;
protected:
  void start(String command);
  int fork();
  void processStatus(int status);

  int _pid;
  int _outFd;
  int _errFd;
  int _inFd;
  uint8_t _code;

  std::shared_ptr<FdStream> _out;
  std::shared_ptr<FdStream> _err;
  std::shared_ptr<FdPrint> _in;
};

class FdStream : public Stream
{
public:
  FdStream(int fd);
  virtual ~FdStream();
  void close();
  virtual int available() override;
  virtual int read() override;
  virtual int peek() override;
  virtual void flush() override;
  virtual size_t write(uint8_t data) override;
protected:
  int fd;
  std::deque<char> buffer;

  void fillBuffer();
};

class FdPrint : public Print
{
public:
  FdPrint(int fd);
  virtual ~FdPrint();
  void close();
  virtual size_t write(uint8_t data) override;
  virtual const char *endline(void) override;
protected:
  int fd;
};

#endif /* Wiring_Process == 1 */

#endif	/* SPARK_WIRING_PROCESS_H */

