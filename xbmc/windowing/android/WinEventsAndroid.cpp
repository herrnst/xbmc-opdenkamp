/*
*      Copyright (C) 2010 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include "system.h"
#include "windowing/WinEvents.h"
#include "WinEventsAndroid.h"
#include "input/XBMC_vkeys.h"
#include "Application.h"
#include "windowing/WindowingFactory.h"
#include "threads/CriticalSection.h"
#include "utils/log.h"

static CCriticalSection g_inputCond;

PHANDLE_EVENT_FUNC CWinEventsBase::m_pEventFunc = NULL;

static std::vector<XBMC_Event> events;

void CWinEventsAndroid::DeInit()
{
}

void CWinEventsAndroid::Init()
{
}

void CWinEventsAndroid::MessagePush(XBMC_Event *newEvent)
{
  CSingleLock lock(g_inputCond);
  events.push_back(*newEvent);
}

bool CWinEventsAndroid::MessagePump()
{
  bool ret = false;
  std::vector<XBMC_Event> copy_events;
  { // double-buffered events to avoid constant locking for OnEvent().
    CSingleLock lock(g_inputCond);
    copy_events = events;
    events.clear();
  }

  for (std::vector<XBMC_Event>::iterator iter = copy_events.begin(); iter != copy_events.end(); iter++)
  {
    ret |= g_application.OnEvent(*iter);
  }

  return ret;
}
