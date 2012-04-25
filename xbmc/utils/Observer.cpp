/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "Application.h"
#include "Observer.h"
#include "threads/SingleLock.h"
#include "utils/JobManager.h"

using namespace std;
using namespace ANNOUNCEMENT;

class ObservableMessageJob : public CJob
{
private:
  Observable              m_observable;
  std::vector<Observer *> m_observers;
  CStdString              m_strMessage;
public:
  ObservableMessageJob(const Observable &obs, const CStdString &strMessage);
  virtual ~ObservableMessageJob() {}
  virtual const char *GetType() const { return "observable-message-job"; }

  virtual bool DoWork();
};

Observer::~Observer(void)
{
  StopObserving();
}

void Observer::StopObserving(void)
{
  CSingleLock lock(m_obsCritSection);
  for (unsigned int iObsPtr = 0; iObsPtr < m_observables.size(); iObsPtr++)
    m_observables.at(iObsPtr)->UnregisterObserver(this);
  m_observables.clear();
}

bool Observer::IsObserving(const Observable &obs) const
{
  CSingleLock lock(m_obsCritSection);
  return find(m_observables.begin(), m_observables.end(), &obs) != m_observables.end();
}

void Observer::RegisterObservable(Observable *obs)
{
  CSingleLock lock(m_obsCritSection);
  if (!IsObserving(*obs))
    m_observables.push_back(obs);
}

void Observer::UnregisterObservable(Observable *obs)
{
  CSingleLock lock(m_obsCritSection);
  vector<Observable *>::iterator it = find(m_observables.begin(), m_observables.end(), obs);
  if (it != m_observables.end())
    m_observables.erase(it);
}

Observable::Observable() :
    m_bObservableChanged(false),
    m_bAsyncAllowed(true)
{
  CAnnouncementManager::AddAnnouncer(this);
}

Observable::~Observable()
{
  CAnnouncementManager::RemoveAnnouncer(this);
  StopObserver();
}

Observable &Observable::operator=(const Observable &observable)
{
  CSingleLock lock(m_obsCritSection);

  m_bObservableChanged = observable.m_bObservableChanged;
  m_observers.clear();
  for (unsigned int iObsPtr = 0; iObsPtr < observable.m_observers.size(); iObsPtr++)
    m_observers.push_back(observable.m_observers.at(iObsPtr));

  return *this;
}

void Observable::StopObserver(void)
{
  CSingleLock lock(m_obsCritSection);
  for (unsigned int iObsPtr = 0; iObsPtr < m_observers.size(); iObsPtr++)
    m_observers.at(iObsPtr)->UnregisterObservable(this);
  m_observers.clear();
}

bool Observable::IsObserving(const Observer &obs) const
{
  CSingleLock lock(m_obsCritSection);
  return find(m_observers.begin(), m_observers.end(), &obs) != m_observers.end();
}

void Observable::RegisterObserver(Observer *obs)
{
  CSingleLock lock(m_obsCritSection);
  if (!IsObserving(*obs))
  {
    m_observers.push_back(obs);
    obs->RegisterObservable(this);
  }
}

void Observable::UnregisterObserver(Observer *obs)
{
  CSingleLock lock(m_obsCritSection);
  vector<Observer *>::iterator it = find(m_observers.begin(), m_observers.end(), obs);
  if (it != m_observers.end())
  {
    obs->UnregisterObservable(this);
    m_observers.erase(it);
  }
}

void Observable::NotifyObservers(const CStdString& strMessage /* = "" */, bool bAsync /* = false */)
{
  CSingleLock lock(m_obsCritSection);
  if (m_bObservableChanged && !g_application.m_bStop)
  {
    if (bAsync && m_bAsyncAllowed)
      CJobManager::GetInstance().AddJob(new ObservableMessageJob(*this, strMessage), NULL);
    else
      SendMessage(this, &m_observers, strMessage);

    m_bObservableChanged = false;
  }
}

void Observable::SetChanged(bool SetTo)
{
  CSingleLock lock(m_obsCritSection);
  m_bObservableChanged = SetTo;
}

void Observable::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "ApplicationStop"))
  {
    CSingleLock lock(m_obsCritSection);
    m_bAsyncAllowed = false;
  }
}

void Observable::SendMessage(Observable *obs, const vector<Observer *> *observers, const CStdString &strMessage)
{
  for(unsigned int ptr = 0; ptr < observers->size(); ptr++)
  {
    Observer *observer = observers->at(ptr);
    if (observer)
      observer->Notify(*obs, strMessage);
  }
}

ObservableMessageJob::ObservableMessageJob(const Observable &obs, const CStdString &strMessage)
{
  m_strMessage = strMessage;
  m_observable = obs;
  m_observers  = obs.m_observers;
}

bool ObservableMessageJob::DoWork()
{
  Observable::SendMessage(&m_observable, &m_observers, m_strMessage);

  return true;
}
