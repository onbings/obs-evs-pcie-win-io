/*
obs-evs-pcie-win-io plugin
Copyright (C) 2026 Bernard HARMEL b.harmel@gmail.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/
#pragma once
//#include <string>
#include "BoardResourceManager.h"

int64_t ConvertStringToInt(const char *_pInputString_c);
bool LoadCliArgFromJson(const std::string &_rFilePath_S, CLIARG &_rCliArg_X);
class ObsLogger : public EvsHwLGPL::CLoggerSink
{
public:
  ObsLogger();
  ~ObsLogger();

private:
  int Message(uint32_t _Severity_U32, const char *_pMsg_c, uint32_t _Size_U32);
  int Message(uint32_t _Severity_U32, uint32_t _Module_U32, const char *_pMsg_c, uint32_t _Size_U32);
};
extern ObsLogger *GL_pObsLogger;
extern EvsHwLGPL::CMsgLogger *GL_pEvsPcieWinIoLogger;
extern std::unique_ptr<BoardResourceManager> GL_puBoardResourceManager;

void on_menu_click(void *private_data);
