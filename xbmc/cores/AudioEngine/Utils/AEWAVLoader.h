#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
 *      http://xbmc.org
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

#include "utils/StdString.h"
#include "AEAudioFormat.h"

class CAEWAVLoader
{
public:
  CAEWAVLoader();
  ~CAEWAVLoader();

  bool Initialize  (const std::string &filename, unsigned int resampleRate = 0);
  void DeInitialize();

  bool IsValid() { return m_valid; }
  bool Remap(CAEChannelInfo to, enum AEStdChLayout stdChLayout = AE_CH_LAYOUT_INVALID);
  unsigned int GetChannelCount();
  unsigned int GetSampleRate();
  unsigned int GetSampleCount();
  unsigned int GetFrameCount();
  float* GetSamples();

private:
  std::string  m_filename;
  bool         m_valid;

  unsigned int m_sampleRate;
  unsigned int m_channelCount;
  unsigned int m_frameCount;
  unsigned int m_sampleCount;
  float       *m_samples;
};

