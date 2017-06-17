/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmStateTypes_h
#define cmStateTypes_h

namespace cmStateEnums {

enum CacheEntryType
{
  BOOL = 0,
  PATH,
  FILEPATH,
  STRING,
  INTERNAL,
  STATIC,
  UNINITIALIZED
};

} // namespace cmStateEnums

#endif
