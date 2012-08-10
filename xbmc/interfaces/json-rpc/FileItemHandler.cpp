/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include <string.h>
#include "FileItemHandler.h"
#include "PlaylistOperations.h"
#include "AudioLibrary.h"
#include "VideoLibrary.h"
#include "FileOperations.h"
#include "utils/URIUtils.h"
#include "utils/ISerializable.h"
#include "utils/Variant.h"
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "video/VideoDatabase.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "TextureCache.h"
#include "ThumbLoader.h"
#include "Util.h"

using namespace MUSIC_INFO;
using namespace JSONRPC;
using namespace XFILE;

void CFileItemHandler::FillDetails(ISerializable* info, CFileItemPtr item, const CVariant& fields, CVariant &result)
{
  if (info == NULL || fields.size() == 0)
    return;

  CVariant serialization;
  info->Serialize(serialization);

  bool fetchedArt = false;

  for (unsigned int i = 0; i < fields.size(); i++)
  {
    CStdString field = fields[i].asString();

    if (item)
    {
      if (item->IsAlbum() && field.Equals("albumlabel"))
        field = "label";
      if (item->IsAlbum())
      {
        if (field == "label")
        {
          result["albumlabel"] = item->GetProperty("album_label");
          continue;
        }
        if (item->HasProperty("album_" + field + "_array"))
        {
          result[field] = item->GetProperty("album_" + field + "_array");
          continue;
        }
        if (item->HasProperty("album_" + field))
        {
          result[field] = item->GetProperty("album_" + field);
          continue;
        }
      }

      if (item->HasProperty("artist_" + field + "_array"))
      {
        result[field] = item->GetProperty("artist_" + field + "_array");
        continue;
      }
      if (item->HasProperty("artist_" + field))
      {
        result[field] = item->GetProperty("artist_" + field);
        continue;
      }

      if (field == "thumbnail")
      {
        if (item->HasVideoInfoTag())
        {
          if (!item->HasThumbnail() && !fetchedArt && item->GetVideoInfoTag()->m_iDbId > -1)
          {
            CVideoThumbLoader loader;
            loader.FillLibraryArt(item.get());
            fetchedArt = true;
          }
        }
        else if (item->HasPictureInfoTag())
        {
          if (!item->HasThumbnail())
            item->SetThumbnailImage(CTextureCache::GetWrappedThumbURL(item->GetPath()));
        }
        else if (item->HasMusicInfoTag())
        {
          if (!item->HasThumbnail() && !fetchedArt && item->GetMusicInfoTag()->GetDatabaseId() > -1)
          {
            CMusicThumbLoader loader;
            loader.FillLibraryArt(*item);
            fetchedArt = true;
          }
        }
        if (item->HasThumbnail())
          result["thumbnail"] = CTextureCache::GetWrappedImageURL(item->GetThumbnailImage());
        if (!result.isMember("thumbnail"))
          result["thumbnail"] = "";
        continue;
      }

      if (field == "fanart")
      {
        if (item->HasVideoInfoTag())
        {
          if (!item->HasProperty("fanart_image") && !fetchedArt && item->GetVideoInfoTag()->m_iDbId > -1)
          {
            CVideoThumbLoader loader;
            loader.FillLibraryArt(item.get());
            fetchedArt = true;
          }
          if (item->HasProperty("fanart_image"))
            result["fanart"] = CTextureCache::GetWrappedImageURL(item->GetProperty("fanart_image").asString());
        }
        else if (item->HasMusicInfoTag())
        {
          if (!item->HasProperty("fanart_image") && !fetchedArt && item->GetMusicInfoTag()->GetDatabaseId() > -1)
          {
            CMusicThumbLoader loader;
            loader.FillLibraryArt(*item);
            fetchedArt = true;
          }
          if (item->HasProperty("fanart_image"))
            result["fanart"] = CTextureCache::GetWrappedImageURL(item->GetProperty("fanart_image").asString());
        }
        if (!result.isMember("fanart"))
          result["fanart"] = "";
        continue;
      }

      if (item->HasVideoInfoTag() && item->GetVideoContentType() == VIDEODB_CONTENT_TVSHOWS)
      {
        if (item->GetVideoInfoTag()->m_iSeason < 0 && field == "season")
        {
          result[field] = (int)item->GetProperty("totalseasons").asInteger();
          continue;
        }
        if (field == "watchedepisodes")
        {
          result[field] = (int)item->GetProperty("watchedepisodes").asInteger();
          continue;
        }
      }

      if (field == "lastmodified" && item->m_dateTime.IsValid())
      {
        result[field] = item->m_dateTime.GetAsLocalizedDateTime();
        continue;
      }
    }

    if (serialization.isMember(field) && (!result.isMember(field) || result[field].empty()))
      result[field] = serialization[field];
  }
}

void CFileItemHandler::HandleFileItemList(const char *ID, bool allowFile, const char *resultname, CFileItemList &items, const CVariant &parameterObject, CVariant &result, bool sortLimit /* = true */)
{
  HandleFileItemList(ID, allowFile, resultname, items, parameterObject, result, items.Size(), sortLimit);
}

void CFileItemHandler::HandleFileItemList(const char *ID, bool allowFile, const char *resultname, CFileItemList &items, const CVariant &parameterObject, CVariant &result, int size, bool sortLimit /* = true */)
{
  int start = (int)parameterObject["limits"]["start"].asInteger();
  int end   = (int)parameterObject["limits"]["end"].asInteger();
  end = (end <= 0 || end > size) ? size : end;
  start = start > end ? end : start;

  if (sortLimit)
    Sort(items, parameterObject["sort"]);

  result["limits"]["start"] = start;
  result["limits"]["end"]   = end;
  result["limits"]["total"] = size;

  if (!sortLimit)
  {
    start = 0;
    end = items.Size();
  }

  for (int i = start; i < end; i++)
  {
    CVariant object;
    CFileItemPtr item = items.Get(i);
    HandleFileItem(ID, allowFile, resultname, item, parameterObject, parameterObject["properties"], result);
  }
}

void CFileItemHandler::HandleFileItem(const char *ID, bool allowFile, const char *resultname, CFileItemPtr item, const CVariant &parameterObject, const CVariant &validFields, CVariant &result, bool append /* = true */)
{
  CVariant object;
  bool hasFileField = false;

  if (item.get())
  {
    for (unsigned int i = 0; i < validFields.size(); i++)
    {
      CStdString field = validFields[i].asString();

      if (field == "file")
        hasFileField = true;
    }

    if (allowFile && hasFileField)
    {
      if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->GetPath().IsEmpty())
          object["file"] = item->GetVideoInfoTag()->GetPath().c_str();
      if (item->HasMusicInfoTag() && !item->GetMusicInfoTag()->GetURL().IsEmpty())
        object["file"] = item->GetMusicInfoTag()->GetURL().c_str();

      if (!object.isMember("file"))
        object["file"] = item->GetPath().c_str();
    }

    if (ID)
    {
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDatabaseId() > 0)
        object[ID] = (int)item->GetMusicInfoTag()->GetDatabaseId();
      else if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iDbId > 0)
        object[ID] = item->GetVideoInfoTag()->m_iDbId;

      if (stricmp(ID, "id") == 0)
      {
        if (item->HasMusicInfoTag())
        {
          if (item->m_bIsFolder && item->IsAlbum())
            object["type"] = "album";
          else
            object["type"] = "song";
        }
        else if (item->HasVideoInfoTag())
        {
          switch (item->GetVideoContentType())
          {
            case VIDEODB_CONTENT_EPISODES:
              object["type"] = "episode";
              break;

            case VIDEODB_CONTENT_MUSICVIDEOS:
              object["type"] = "musicvideo";
              break;

            case VIDEODB_CONTENT_MOVIES:
              object["type"] = "movie";
              break;

            case VIDEODB_CONTENT_TVSHOWS:
              object["type"] = "tvshow";
              break;

            default:
              break;
          }
        }
        else if (item->HasPictureInfoTag())
          object["type"] = "picture";

        if (!object.isMember("type"))
          object["type"] = "unknown";
      }
    }

    FillDetails(item.get(), item, validFields, object);

    if (item->HasVideoInfoTag())
      FillDetails(item->GetVideoInfoTag(), item, validFields, object);
    if (item->HasMusicInfoTag())
      FillDetails(item->GetMusicInfoTag(), item, validFields, object);
    if (item->HasPictureInfoTag())
      FillDetails(item->GetPictureInfoTag(), item, validFields, object);

    object["label"] = item->GetLabel().c_str();
  }
  else
    object = CVariant(CVariant::VariantTypeNull);

  if (resultname)
  {
    if (append)
      result[resultname].append(object);
    else
      result[resultname] = object;
  }
}

bool CFileItemHandler::FillFileItemList(const CVariant &parameterObject, CFileItemList &list)
{
  CAudioLibrary::FillFileItemList(parameterObject, list);
  CVideoLibrary::FillFileItemList(parameterObject, list);
  CFileOperations::FillFileItemList(parameterObject, list);

  CStdString file = parameterObject["file"].asString();
  if (!file.empty() && (URIUtils::IsURL(file) || (CFile::Exists(file) && !CDirectory::Exists(file))))
  {
    bool added = false;
    for (int index = 0; index < list.Size(); index++)
    {
      if (list[index]->GetPath() == file)
      {
        added = true;
        break;
      }
    }

    if (!added)
    {
      CFileItemPtr item = CFileItemPtr(new CFileItem(file, false));
      if (item->IsPicture())
      {
        CPictureInfoTag picture;
        picture.Load(item->GetPath());
        *item->GetPictureInfoTag() = picture;
      }
      if (item->GetLabel().IsEmpty())
        item->SetLabel(CUtil::GetTitleFromPath(file, false));
      list.Add(item);
    }
  }

  return (list.Size() > 0);
}

bool CFileItemHandler::ParseSorting(const CVariant &parameterObject, SortBy &sortBy, SortOrder &sortOrder, SortAttribute &sortAttributes)
{
  CStdString method = parameterObject["sort"]["method"].asString();
  CStdString order = parameterObject["sort"]["order"].asString();
  method.ToLower();
  order.ToLower();

  sortAttributes = SortAttributeNone;
  if (parameterObject["sort"]["ignorearticle"].asBoolean())
    sortAttributes = SortAttributeIgnoreArticle;
  else
    sortAttributes = SortAttributeNone;

  if (order.Equals("ascending"))
    sortOrder = SortOrderAscending;
  else if (order.Equals("descending"))
    sortOrder = SortOrderDescending;
  else
    return false;

  if (method.Equals("none"))
    sortBy = SortByNone;
  else if (method.Equals("label"))
    sortBy = SortByLabel;
  else if (method.Equals("date"))
    sortBy = SortByDate;
  else if (method.Equals("size"))
    sortBy = SortBySize;
  else if (method.Equals("file"))
    sortBy = SortByFile;
  else if (method.Equals("drivetype"))
    sortBy = SortByDriveType;
  else if (method.Equals("track"))
    sortBy = SortByTrackNumber;
  else if (method.Equals("duration") ||
           method.Equals("videoruntime"))
    sortBy = SortByTime;
  else if (method.Equals("title") ||
           method.Equals("videotitle"))
    sortBy = SortByTitle;
  else if (method.Equals("artist"))
    sortBy = SortByArtist;
  else if (method.Equals("album"))
    sortBy = SortByAlbum;
  else if (method.Equals("genre"))
    sortBy = SortByGenre;
  else if (method.Equals("country"))
    sortBy = SortByCountry;
  else if (method.Equals("year"))
    sortBy = SortByYear;
  else if (method.Equals("videorating") ||
           method.Equals("songrating"))
    sortBy = SortByRating;
  else if (method.Equals("dateadded"))
    sortBy = SortByDateAdded;
  else if (method.Equals("programcount"))
    sortBy = SortByProgramCount;
  else if (method.Equals("playlist"))
    sortBy = SortByPlaylistOrder;
  else if (method.Equals("episode"))
    sortBy = SortByEpisodeNumber;
  else if (method.Equals("sorttitle"))
    sortBy = SortBySortTitle;
  else if (method.Equals("productioncode"))
    sortBy = SortByProductionCode;
  else if (method.Equals("mpaarating"))
    sortBy = SortByMPAA;
  else if (method.Equals("studio"))
    sortBy = SortByStudio;
  else if (method.Equals("fullpath"))
    sortBy = SortByPath;
  else if (method.Equals("lastplayed"))
    sortBy = SortByLastPlayed;
  else if (method.Equals("playcount"))
    sortBy = SortByPlaycount;
  else if (method.Equals("listeners"))
    sortBy = SortByListeners;
  else if (method.Equals("unsorted"))
    sortBy = SortByRandom;
  else if (method.Equals("bitrate"))
    sortBy = SortByBitrate;
  else
    return false;

  return true;
}

void CFileItemHandler::ParseLimits(const CVariant &parameterObject, int &limitStart, int &limitEnd)
{
  limitStart = (int)parameterObject["limits"]["start"].asInteger();
  limitEnd = (int)parameterObject["limits"]["end"].asInteger();
}

bool CFileItemHandler::ParseSortMethods(const CStdString &method, const bool &ignorethe, const CStdString &order, SORT_METHOD &sortmethod, SortOrder &sortorder)
{
  if (order.Equals("ascending"))
    sortorder = SortOrderAscending;
  else if (order.Equals("descending"))
    sortorder = SortOrderDescending;
  else
    return false;

  if (method.Equals("none"))
    sortmethod = SORT_METHOD_NONE;
  else if (method.Equals("label"))
    sortmethod = ignorethe ? SORT_METHOD_LABEL_IGNORE_THE : SORT_METHOD_LABEL;
  else if (method.Equals("date"))
    sortmethod = SORT_METHOD_DATE;
  else if (method.Equals("size"))
    sortmethod = SORT_METHOD_SIZE;
  else if (method.Equals("file"))
    sortmethod = SORT_METHOD_FILE;
  else if (method.Equals("drivetype"))
    sortmethod = SORT_METHOD_DRIVE_TYPE;
  else if (method.Equals("track"))
    sortmethod = SORT_METHOD_TRACKNUM;
  else if (method.Equals("duration"))
    sortmethod = SORT_METHOD_DURATION;
  else if (method.Equals("title"))
    sortmethod = ignorethe ? SORT_METHOD_TITLE_IGNORE_THE : SORT_METHOD_TITLE;
  else if (method.Equals("artist"))
    sortmethod = ignorethe ? SORT_METHOD_ARTIST_IGNORE_THE : SORT_METHOD_ARTIST;
  else if (method.Equals("album"))
    sortmethod = ignorethe ? SORT_METHOD_ALBUM_IGNORE_THE : SORT_METHOD_ALBUM;
  else if (method.Equals("genre"))
    sortmethod = SORT_METHOD_GENRE;
  else if (method.Equals("country"))
    sortmethod = SORT_METHOD_COUNTRY;
  else if (method.Equals("year"))
    sortmethod = SORT_METHOD_YEAR;
  else if (method.Equals("videorating"))
    sortmethod = SORT_METHOD_VIDEO_RATING;
  else if (method.Equals("dateadded"))
    sortmethod = SORT_METHOD_DATEADDED;
  else if (method.Equals("programcount"))
    sortmethod = SORT_METHOD_PROGRAM_COUNT;
  else if (method.Equals("playlist"))
    sortmethod = SORT_METHOD_PLAYLIST_ORDER;
  else if (method.Equals("episode"))
    sortmethod = SORT_METHOD_EPISODE;
  else if (method.Equals("videotitle"))
    sortmethod = SORT_METHOD_VIDEO_TITLE;
  else if (method.Equals("sorttitle"))
    sortmethod = ignorethe ? SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE : SORT_METHOD_VIDEO_SORT_TITLE;
  else if (method.Equals("productioncode"))
    sortmethod = SORT_METHOD_PRODUCTIONCODE;
  else if (method.Equals("songrating"))
    sortmethod = SORT_METHOD_SONG_RATING;
  else if (method.Equals("mpaarating"))
    sortmethod = SORT_METHOD_MPAA_RATING;
  else if (method.Equals("videoruntime"))
    sortmethod = SORT_METHOD_VIDEO_RUNTIME;
  else if (method.Equals("studio"))
    sortmethod = ignorethe ? SORT_METHOD_STUDIO_IGNORE_THE : SORT_METHOD_STUDIO;
  else if (method.Equals("fullpath"))
    sortmethod = SORT_METHOD_FULLPATH;
  else if (method.Equals("lastplayed"))
    sortmethod = SORT_METHOD_LASTPLAYED;
  else if (method.Equals("playcount"))
    sortmethod = SORT_METHOD_PLAYCOUNT;
  else if (method.Equals("listeners"))
    sortmethod = SORT_METHOD_LISTENERS;
  else if (method.Equals("unsorted"))
    sortmethod = SORT_METHOD_UNSORTED;
  else if (method.Equals("bitrate"))
    sortmethod = SORT_METHOD_BITRATE;
  else
    return false;

  return true;
}

void CFileItemHandler::Sort(CFileItemList &items, const CVariant &parameterObject)
{
  CStdString method = parameterObject["method"].asString();
  CStdString order  = parameterObject["order"].asString();

  method = method.ToLower();
  order  = order.ToLower();

  SORT_METHOD sortmethod = SORT_METHOD_NONE;
  SortOrder   sortorder  = SortOrderAscending;

  if (ParseSortMethods(method, parameterObject["ignorearticle"].asBoolean(), order, sortmethod, sortorder))
    items.Sort(sortmethod, sortorder);
}
