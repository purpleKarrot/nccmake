/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmState.h"

std::vector<std::string> cmState::GetCacheEntryKeys() const
{
  std::vector<std::string> definitions;
  definitions.reserve(this->Cache.size());
  for (auto const& entry : this->Cache) {
    definitions.push_back(entry.first);
  }
  return definitions;
}

const char* cmState::GetCacheEntryValue(std::string const& key) const
{
  auto const i = this->Cache.find(key);
  if (i == this->Cache.end()) {
    return nullptr;
  }
  return i->second.Value.c_str();
}

cmStateEnums::CacheEntryType cmState::GetCacheEntryType(
  std::string const& key) const
{
  auto const i = this->Cache.find(key);
  if (i == this->Cache.end()) {
    return cmStateEnums::UNINITIALIZED;
  }
  return i->second.Type;
}

const char* cmState::GetCacheEntryProperty(
  std::string const& key, std::string const& propertyName) const
{
  auto const i = this->Cache.find(key);
  if (i == this->Cache.end()) {
    return nullptr;
  }
  if (propertyName == "STRINGS") {
    return i->second.Strings.empty() ? nullptr : i->second.Strings.c_str();
  }
  if (propertyName == "HELPSTRING") {
    return i->second.HelpString.c_str();
  }
  return nullptr;
}

bool cmState::GetCacheEntryPropertyAsBool(
  std::string const& key, std::string const& propertyName) const
{
  auto const i = this->Cache.find(key);
  if (i == this->Cache.end()) {
    return false;
  }
  if (propertyName == "ADVANCED") {
    return i->second.IsAdvanced;
  }
  return false;
}

void cmState::RemoveCacheEntry(std::string const& key)
{
  auto const i = this->Cache.find(key);
  if (i != this->Cache.end()) {
    i->second.IsRemoved = true;
  }
}

void cmState::SetCacheEntryBoolProperty(
  std::string const& key, std::string const& propertyName, bool value)
{
  if (propertyName == "ADVANCED") {
    this->Cache[key].IsAdvanced = value;
  }
  if (propertyName == "MODIFIED") {
    this->Cache[key].IsModified = value;
  }
}

void cmState::SetCacheEntryValue(
  std::string const& key, std::string const& value)
{
  this->Cache[key].Value = value;
}
