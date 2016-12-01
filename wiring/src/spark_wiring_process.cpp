/**
 ******************************************************************************
 * @file    spark_wiring_process.cpp
 * @author  Julien Vanier
 * @version V1.0.0
 * @date    30-Nov-2016
 * @brief   Wiring class for UNIX process management
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "spark_wiring_process.h"

#if Wiring_Process == 1

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>

// constants for read/write ends of pipe created by pipe2
enum { RD, WR };

const uint8_t Process::COMMAND_NOT_FOUND = 127;

Process::Process() :
  _pid(0),
  _outFd(-1),
  _errFd(-1),
  _inFd(-1),
  _code(0)
{

}

Process Process::run(String command)
{
  Process proc;
  proc.start(command);
  return proc;
}

void Process::start(String command)
{
  int _pid = fork();
  if (_pid > 0)
  {
    // parent

    _out = std::make_shared<FdStream>(_outFd);
    _err = std::make_shared<FdStream>(_errFd);
    _in = std::make_shared<FdPrint>(_inFd);

  }
  else if (_pid == 0)
  {
    // child

    // Run the command
    execl("/bin/sh", "sh", "-c", command.c_str(), NULL);

    // Can only get here if execl fails
    // Use _exit to avoid runnig static C++ destructors
    ::_exit(errno);
  }
}

int Process::fork()
{
  int inPipes[2] = { -1, -1 };
  int outPipes[2] = { -1, -1 };
  int errPipes[2] = { -1, -1 };

  if (::pipe2(inPipes, 0) < 0 ||
      ::pipe2(outPipes, O_NONBLOCK) < 0 ||
      ::pipe2(errPipes, O_NONBLOCK) < 0)
  {
    perror("Error opening pipes for Process command");
    return -1;
  }

  _pid = ::fork();
  if (_pid > 0)
  {
    // parent

    _inFd = inPipes[WR];
    ::close(inPipes[RD]);
    _outFd = outPipes[RD];
    ::close(outPipes[WR]);
    _errFd = errPipes[RD];
    ::close(errPipes[WR]);

  }
  else if (_pid == 0)
  {
    // child
    ::dup2(inPipes[RD], STDIN_FILENO);
    ::close(inPipes[WR]);
    ::dup2(outPipes[WR], STDOUT_FILENO);
    ::close(outPipes[RD]);
    ::dup2(errPipes[WR], STDERR_FILENO);
    ::close(errPipes[RD]);

  }
  else
  {
    perror("Error forking in Process command");
  }

  return _pid;
}

uint8_t Process::wait()
{
  if (_pid <= 0)
  {
    return 0;
  }

  int status;
  ::waitpid(_pid, &status, 0);
  processStatus(status);

  return exitCode();
}

void Process::kill(int signal)
{
  if (_pid <= 0)
  {
    return;
  }
  ::kill(_pid, signal);
}

bool Process::exited()
{
  if (_pid <= 0)
  {
    return true;
  }

  int status;
  int changedPid = ::waitpid(_pid, &status, WNOHANG);
  if (changedPid == _pid)
  {
    processStatus(status);
    return true;
  }
  else
  {
    return false;
  }
}

void Process::processStatus(int status)
{
  if (WIFEXITED(status))
  {
    _code = WEXITSTATUS(status);
  }
  else if (WIFSIGNALED(status))
  {
    // Bash convention: signal n gives exit _code 128+n
    _code = 128 + WTERMSIG(status);
  }
  _pid = 0;
}

int Process::pid()
{
  return _pid;
}

uint8_t Process::exitCode()
{
  return _code;
}

FdStream& Process::out()
{
  return *_out;
}

FdStream& Process::err()
{
  return *_err;
}

FdPrint& Process::in()
{
  return *_in;
}

FdStream::FdStream(int fd) : fd(fd)
{
  _timeout = 1;
}

FdStream::~FdStream()
{
  close();
}

void FdStream::close()
{
  ::close(fd);
}

int FdStream::available()
{
  fillBuffer();
  return buffer.size();
}

int FdStream::read()
{
  fillBuffer();

  if (buffer.empty())
  {
    return -1;
  }

  char c = buffer.front();
  buffer.pop_front();
  return c;
}

int FdStream::peek()
{
  fillBuffer();

  if (buffer.empty())
  {
    return -1;
  }

  return buffer.front();
}


void FdStream::flush()
{
  while (available())
  {
    read();
  }
}

size_t FdStream::write(uint8_t data)
{
  return 0;
}
void FdStream::fillBuffer()
{
  if (!buffer.empty())
  {
    return;
  }

  char tmp[100];
  size_t count = ::read(fd, tmp, sizeof(tmp));
  if (count == (size_t)-1)
  {
    perror("FdStream::read failed");
    return;
  }
  buffer.insert(buffer.end(), &tmp[0], &tmp[count]);
}


FdPrint::FdPrint(int fd) : fd(fd)
{
}

FdPrint::~FdPrint()
{
  close();
}

void FdPrint::close()
{
  ::close(fd);
}

size_t FdPrint::write(uint8_t data)
{
  return ::write(fd, &data, sizeof(data));
}

const char *FdPrint::endline(void)
{
  return "\n";
}

#endif /* Wiring_Process == 1 */
