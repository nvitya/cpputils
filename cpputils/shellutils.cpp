/* -----------------------------------------------------------------------------
 * This file was originated here: https://github.com/nvitya/cpputils
 * Copyright (c) 2023 Viktor Guath-Nagy, nvitya
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software. Permission is granted to anyone to use this
 * software for any purpose, including commercial applications, and to alter
 * it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 * --------------------------------------------------------------------------- */
/*
 *  file:     shellutils.cpp
 *  brief:    Shell command runner
 *  created:  2026-08-31
 *  authors:  nvitya
*/

#include "shellutils.h"
#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>
#include <string.h>
#include <cerrno>
#include <thread>
#include <chrono>
#include <signal.h>

int OShellRunner::StartProcess(const char * pcmd)
{
  int stdin_pipe[2];
  int stdout_pipe[2];
  int stderr_pipe[2];

  if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0)
  {
    return -1;
  }

  if (provide_stdin)
  {
    if (pipe(stdin_pipe) != 0)
    {
      close(stdout_pipe[0]);
      close(stdout_pipe[1]);
      close(stderr_pipe[0]);
      close(stderr_pipe[1]);
      return -1;
    }
  }

  pid = fork();
  if (pid == -1)
  {
    return -1;
  }

  if (pid == 0)
  {
    // Child process
    setpgid(0, 0);
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);
    
    dup2(stdout_pipe[1], STDOUT_FILENO);
    dup2(stderr_pipe[1], STDERR_FILENO);
    
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    
    if (provide_stdin)
    {
      close(stdin_pipe[1]);
      dup2(stdin_pipe[0], STDIN_FILENO);
      close(stdin_pipe[0]);
    }
    
    execl("/bin/sh", "sh", "-c", pcmd, nullptr);
    _exit(127);
  }

  // Parent process
  close(stdout_pipe[1]);
  close(stderr_pipe[1]);
  
  out_fd = stdout_pipe[0];
  err_fd = stderr_pipe[0];
  
  if (provide_stdin)
  {
    close(stdin_pipe[0]);
    in_fd = stdin_pipe[1];
  }
  
  return 0;
}

void OShellRunner::MonitorProcess()
{
  struct pollfd fds[3];
  int nfds = 2;
  
  fds[0].fd = out_fd;
  fds[0].events = POLLIN;
  fds[1].fd = err_fd;
  fds[1].events = POLLIN;
  
  if (provide_stdin && in_fd >= 0)
  {
    fds[2].fd = in_fd;
    fds[2].events = POLLOUT;
    nfds = 3;
  }

  char buffer[4096];
  
  while (fds[0].fd >= 0 || fds[1].fd >= 0 || (nfds == 3 && fds[2].fd >= 0))
  {
    int ret = poll(fds, nfds, -1);
    if (ret < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      break;
    }

    for (int i = 0; i < 2; ++i)
    {
      if (fds[i].fd >= 0 && (fds[i].revents & (POLLIN | POLLERR | POLLHUP)))
      {
        ssize_t count = read(fds[i].fd, buffer, sizeof(buffer));
        if (count > 0)
        {
          if (i == 0)
          {
            out.append(buffer, count);
            output.append(buffer, count);
          }
          else
          {
            err.append(buffer, count);
            output.append(buffer, count);
          }
        }
        else if (count == 0 || (count < 0 && errno != EINTR))
        {
          close(fds[i].fd);
          fds[i].fd = -1;
        }
      }
    }
    
    if (nfds == 3 && fds[2].fd >= 0 && (fds[2].revents & POLLOUT))
    {
      size_t remaining = in_buffer.length() - in_buffer_pos;
      if (remaining > 0)
      {
        ssize_t count = write(fds[2].fd, in_buffer.c_str() + in_buffer_pos, remaining);
        if (count > 0)
        {
          in_buffer_pos += count;
        }
        else if (count < 0 && errno != EINTR && errno != EAGAIN)
        {
          close(fds[2].fd);
          fds[2].fd = -1;
        }
      }
      
      if (in_buffer_pos >= in_buffer.length())
      {
        close(fds[2].fd);
        fds[2].fd = -1;
      }
    }
    else if (nfds == 3 && fds[2].fd >= 0 && (fds[2].revents & (POLLERR | POLLHUP)))
    {
      close(fds[2].fd);
      fds[2].fd = -1;
    }
  }

  if (fds[0].fd >= 0)
  {
    close(fds[0].fd);
  }
  
  if (fds[1].fd >= 0)
  {
    close(fds[1].fd);
  }
  
  if (nfds == 3 && fds[2].fd >= 0)
  {
    close(fds[2].fd);
  }
  
  out_fd = -1;
  err_fd = -1;
  in_fd = -1;

  int status;
  waitpid(pid, &status, 0);
  
  if (WIFEXITED(status))
  {
    exitcode = WEXITSTATUS(status);
  }
  else
  {
    exitcode = -1; // Or somehow encode termination by signal
  }

  finished = true;
}

int OShellRunner::Run(const char * pcmd)
{
  out.clear();
  err.clear();
  output.clear();
  exitcode = -1;
  finished = false;
  provide_stdin = false;

  if (StartProcess(pcmd) != 0)
  {
    finished = true;
    return -1;
  }

  MonitorProcess();

  return exitcode;
}

int OShellRunner::RunWithInput(const char * pcmd, const string& input)
{
  out.clear();
  err.clear();
  output.clear();
  exitcode = -1;
  finished = false;
  
  provide_stdin = true;
  in_buffer = input;
  in_buffer_pos = 0;

  if (StartProcess(pcmd) != 0)
  {
    finished = true;
    return -1;
  }

  MonitorProcess();

  return exitcode;
}

int OShellRunner::StartBg(const char * pcmd)
{
  out.clear();
  err.clear();
  output.clear();
  exitcode = -1;
  finished = false;
  provide_stdin = false;

  if (StartProcess(pcmd) != 0)
  {
    finished = true;
    return -1;
  }

  thread([this]()
  {
    MonitorProcess();
  }).detach();

  return 0;
}

bool OShellRunner::Finished()
{
  return finished;
}

bool OShellRunner::WaitFinishMs(int ms)
{
  auto start = chrono::steady_clock::now();
  while (!finished)
  {
    auto now = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - start).count();
    if (elapsed >= ms)
    {
      return false; // Timeout
    }
    this_thread::sleep_for(chrono::milliseconds(10));
  }
  return true; // Finished within time
}

void OShellRunner::Kill()
{
  if (!finished && pid > 0)
  {
    kill(-pid, SIGKILL);
    // Wait for the background thread to reap the process and set finished = true
    auto start = chrono::steady_clock::now();
    while (!finished)
    {
      auto now = chrono::steady_clock::now();
      if (chrono::duration_cast<chrono::milliseconds>(now - start).count() > 2000)
      {
        break; // safeguard timeout
      }
      this_thread::sleep_for(chrono::milliseconds(10));
    }
  }
}
