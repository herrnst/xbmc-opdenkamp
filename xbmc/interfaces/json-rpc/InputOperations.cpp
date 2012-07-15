/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "InputOperations.h"
#include "Application.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "input/ButtonTranslator.h"
#include "input/XBMC_keyboard.h"
#include "input/XBMC_vkeys.h"
#include "threads/SingleLock.h"
#include "utils/CharsetConverter.h"

using namespace JSONRPC;

CCriticalSection CInputOperations::m_critSection;
CKey CInputOperations::m_key(KEY_INVALID);

CKey CInputOperations::GetKey()
{
  CSingleLock lock(m_critSection);
  CKey currentKey = m_key;
  m_key = CKey(KEY_INVALID);
  return currentKey;
}

//TODO the breakage of the screensaver should be refactored
//to one central super duper place for getting rid of
//1 million dupes
bool CInputOperations::handleScreenSaver()
{
  bool screenSaverBroken = false; //true if screensaver was active and we did reset him

  g_application.ResetScreenSaver();
  
  if(g_application.IsInScreenSaver())
  {
    g_application.WakeUpScreenSaverAndDPMS();
    screenSaverBroken = true;
  }
  return screenSaverBroken;
}

JSONRPC_STATUS CInputOperations::SendKey(uint32_t keyCode, bool unicode /* = false */)
{
  if (keyCode == KEY_INVALID)
    return InternalError;

  CSingleLock lock(m_critSection);
  if (unicode)
    m_key = CKey(0, (wchar_t)keyCode, 0, 0, 0);
  else
    m_key = CKey(keyCode | KEY_VKEY);
  return ACK;
}

JSONRPC_STATUS CInputOperations::SendAction(int actionID, bool wakeScreensaver /* = true */, bool waitResult /* = false */)
{
  if(!wakeScreensaver || !handleScreenSaver())
  {
    g_application.ResetSystemIdleTimer();
    g_audioManager.PlayActionSound(actionID);
    g_application.getApplicationMessenger().SendAction(CAction(actionID), WINDOW_INVALID, waitResult);
  }
  return ACK;
}

JSONRPC_STATUS CInputOperations::activateWindow(int windowID)
{
  if(!handleScreenSaver())
    g_application.getApplicationMessenger().ActivateWindow(windowID, std::vector<CStdString>(), false);

  return ACK;
}

JSONRPC_STATUS CInputOperations::SendText(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string text = parameterObject["text"].asString();
  if (text.empty())
    return InvalidParams;

  CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());
  if (!window)
    return InternalError;

  CGUIMessage msg(GUI_MSG_SET_TEXT, 0, 0);
  msg.SetLabel(text);
  msg.SetParam1(parameterObject["done"].asBoolean() ? 1 : 0);
  g_application.getApplicationMessenger().SendGUIMessage(msg, window->GetID());
  return ACK;
}

JSONRPC_STATUS CInputOperations::ExecuteAction(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int action;
  if (!CButtonTranslator::TranslateActionString(parameterObject["action"].asString().c_str(), action))
    return InvalidParams;

  return SendAction(action);
}

JSONRPC_STATUS CInputOperations::Left(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendKey(XBMCVK_LEFT);
}

JSONRPC_STATUS CInputOperations::Right(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendKey(XBMCVK_RIGHT);
}

JSONRPC_STATUS CInputOperations::Down(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendKey(XBMCVK_DOWN);
}

JSONRPC_STATUS CInputOperations::Up(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendKey(XBMCVK_UP);
}

JSONRPC_STATUS CInputOperations::Select(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendKey(XBMCVK_RETURN);
}

JSONRPC_STATUS CInputOperations::Back(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendKey(XBMCVK_BACK);
}

JSONRPC_STATUS CInputOperations::ContextMenu(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_CONTEXT_MENU);
}

JSONRPC_STATUS CInputOperations::Info(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_SHOW_INFO);
}

JSONRPC_STATUS CInputOperations::Home(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return activateWindow(WINDOW_HOME);
}

JSONRPC_STATUS CInputOperations::ShowCodec(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_SHOW_CODEC);
}

JSONRPC_STATUS CInputOperations::ShowOSD(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_SHOW_OSD);
}
