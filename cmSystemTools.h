/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmSystemTools_h
#define cmSystemTools_h

#include "cmsys/SystemTools.hxx"

#include <string>
#include <vector>

class cmSystemTools : public cmsys::SystemTools
{
public:
  static bool IsOn(std::string const& val);
  static bool IsOff(std::string const& val);

  typedef void (*MessageCallback)(const char*, const char*, bool&, void*);
  static void SetMessageCallback(MessageCallback f, void* clientData);

  static void Error(const char* m, const char* m2);
  static bool GetErrorOccuredFlag();
  static void ResetErrorOccuredFlag();

  static void ExpandListArgument(
    const std::string& arg, std::vector<std::string>& argsOut);

  static bool SimpleGlob(
    const std::string& glob, std::vector<std::string>& files, int type);

  static void FindCMakeResources(const char* argv0);
  static std::string const& GetCMakeCursesCommand();

  static void DisableRunCommandOutput() {}
};

#endif
