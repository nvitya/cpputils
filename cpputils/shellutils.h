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
 *  file:     shellutils.h
 *  brief:    Shell command runner
 *  created:  2026-08-31
 *  authors:  nvitya
*/

#pragma once

#include <string>
#include <thread>

using namespace std;

class OShellRunner
{
public:
  int     exitcode = 0;
  bool    finished = true;
  string  out = "";
  string  err = "";
  string  output = "";  // concatenated std_out + std_err

  int Run(const char * pcmd);      // start, wait until finishes
  int StartBg(const char * pcmd);  // starts in the background
  int StartBgWithInput(const char * pcmd, const string & input);  // starts in the background
  int RunWithInput(const char * pcmd, const string & input);

  bool Finished();
  bool WaitFinishMs(int ms);
  void Kill();

protected:
  pid_t pid = 0;  // background task pid
  int in_fd = -1;
  int out_fd = -1;
  int err_fd = -1;
  string in_buffer = "";
  size_t in_buffer_pos = 0;
  bool provide_stdin = false;

  int StartProcess(const char * pcmd);
  void MonitorProcess();
};
