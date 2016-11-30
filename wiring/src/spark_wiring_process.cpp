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

// pipe2 returns 2 file descriptors, one for reading, one for writing
const auto READ_END = 0;
const auto WRITE_END = 1;

Process::Process() :
  pid(0),
  outFd(-1),
  errFd(-1),
  inFd(-1),
  code(0)
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
  int inPipes[2];
  int outPipes[2];
  int errPipes[2];

  if (pipe2(inPipes, 0) < 0 ||
      pipe2(outPipes, O_NONBLOCK) < 0 ||
      pipe2(errPipes, O_NONBLOCK) < 0)
  {
    perror("Error opening pipes for Process command");
    return;
  }

  pid = fork();
  if (pid > 0) {
    // parent

    inFd = inPipes[WRITE_END];
    close(inPipes[READ_END]);
    outFd = outPipes[READ_END];
    close(outPipes[WRITE_END]);
    errFd = errPipes[READ_END];
    close(errPipes[WRITE_END]);

    outPtr = std::make_shared<FdStream>(outFd);
    errPtr = std::make_shared<FdStream>(errFd);
    inPtr = std::make_shared<FdPrint>(inFd);

  } else if (pid == 0) {
    // child
    dup2(inPipes[READ_END], STDIN_FILENO);
    close(inPipes[WRITE_END]);
    dup2(outPipes[WRITE_END], STDOUT_FILENO);
    close(outPipes[READ_END]);
    dup2(errPipes[WRITE_END], STDERR_FILENO);
    close(errPipes[READ_END]);

    // Run the command
    execl("/bin/sh", "sh", "-c", command.c_str(), NULL);

  } else {
    perror("Error forking in Process command");
  }
}

uint8_t Process::wait()
{
  if (pid <= 0) {
    return 0;
  }

  int status;
  waitpid(pid, &status, 0);
  processStatus(status);

  return exitCode();
}

bool Process::exited()
{
  if (pid <= 0) {
    return true;
  }

  int status;
  int changedPid = waitpid(pid, &status, WNOHANG);
  if (changedPid == pid)
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
  if (WIFEXITED(status)) {
    code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    // Bash convention: signal n gives exit code 128+n
    code = 128 + WTERMSIG(status);
  }
  pid = 0;
}

uint8_t Process::exitCode()
{
  return code;
}

FdStream& Process::out()
{
  return *outPtr;
}

FdStream& Process::err()
{
  return *errPtr;
}

FdPrint& Process::in()
{
  return *inPtr;
}

FdStream::FdStream(int fd) : fd(fd)
{
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

  if (buffer.empty()) {
    return -1;
  }

  char c = buffer.front();
  buffer.pop_front();
  return c;
}

int FdStream::peek()
{
  fillBuffer();

  if (buffer.empty()) {
    return -1;
  }

  return buffer.front();
}


void FdStream::flush()
{
  while (available()) {
    read();
  }
}

size_t FdStream::write(uint8_t data)
{
  return 0;
}
void FdStream::fillBuffer()
{
  if (!buffer.empty()) {
    return;
  }

  char tmp[100];
  size_t count = ::read(fd, tmp, sizeof(tmp));
  if (count == (size_t)-1) {
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

#endif /* Wiring_Process == 1 */
