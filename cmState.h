/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmState_h
#define cmState_h

#include <map>
#include <string>
#include <vector>

#include "cmStateTypes.h"

class cmState
{
public:
  struct CacheEntry
  {
    std::string Value;
    cmStateEnums::CacheEntryType Type;
    std::string HelpString;  // string to show as tooltip
    std::string Strings;     // dropdown values, separated by ;
    bool IsAdvanced = false; // hidden per default
    bool IsModified = false; // value was modified, show in bold
    bool IsRemoved = false;  // value was flagged for removal
  };

  std::map<std::string, CacheEntry>& GetCache() { return this->Cache; }

  std::vector<std::string> GetCacheEntryKeys() const;

  const char* GetCacheEntryValue(std::string const& key) const;

  cmStateEnums::CacheEntryType GetCacheEntryType(std::string const& key) const;

  const char* GetCacheEntryProperty(
    std::string const& key, std::string const& propertyName) const;

  bool GetCacheEntryPropertyAsBool(
    std::string const& key, std::string const& propertyName) const;

  void RemoveCacheEntry(std::string const& key);

  void SetCacheEntryBoolProperty(
    std::string const& key, std::string const& propertyName, bool value);

  void SetCacheEntryValue(std::string const& key, std::string const& value);

private:
  std::map<std::string, CacheEntry> Cache;
};

#endif
