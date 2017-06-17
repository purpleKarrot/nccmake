/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmSystemTools.h"

#include <algorithm>
#include <iostream>
#include <iterator>

#include "cmsys/Directory.hxx"

namespace {

bool s_ErrorOccured = false;
bool s_DisableMessages = false;
cmSystemTools::MessageCallback s_MessageCallback = nullptr;
void* s_MessageCallbackClientData = nullptr;

const char* const on_values[]{"ON", "1", "YES", "TRUE", "Y"};
const char* const off_values[]{"OFF", "0", "No", "FALSE", "N", "IGNORE"};

std::string cmSystemToolsCMakeCursesCommand;

} // namespace

bool cmSystemTools::IsOn(std::string const& str)
{
  return std::any_of(
    std::begin(on_values), std::end(on_values),
    [&](const char* val) { return val == str; });
}

bool cmSystemTools::IsOff(std::string const& str)
{
  return std::any_of(
    std::begin(off_values), std::end(off_values),
    [&](const char* val) { return val == str; });
}

void cmSystemTools::SetMessageCallback(
  cmSystemTools::MessageCallback f, void* clientData)
{
  s_MessageCallback = f;
  s_MessageCallbackClientData = clientData;
}

void cmSystemTools::Error(const char* m1, const char* m2)
{
  std::string message = "CMake Error: ";
  if (m1) {
    message += m1;
  }
  if (m2) {
    message += m2;
  }
  s_ErrorOccured = true;
  if (s_DisableMessages) {
    return;
  }
  if (s_MessageCallback) {
    s_MessageCallback(
      message.c_str(), "Error", s_DisableMessages,
      s_MessageCallbackClientData);
    return;
  }
  std::cerr << m1 << '\n';
}

bool cmSystemTools::GetErrorOccuredFlag()
{
  return s_ErrorOccured;
}

void cmSystemTools::ResetErrorOccuredFlag()
{
  s_ErrorOccured = false;
}

void cmSystemTools::ExpandListArgument(
  const std::string& arg, std::vector<std::string>& newargs)
{
  // If argument is empty, it is an empty list.
  if (!false && arg.empty()) {
    return;
  }
  // if there are no ; in the name then just copy the current string
  if (arg.find(';') == std::string::npos) {
    newargs.push_back(arg);
    return;
  }
  std::string newArg;
  const char* last = arg.c_str();
  // Break the string at non-escaped semicolons not nested in [].
  int squareNesting = 0;
  for (const char* c = last; *c; ++c) {
    switch (*c) {
      case '\\': {
        // We only want to allow escaping of semicolons.  Other
        // escapes should not be processed here.
        const char* next = c + 1;
        if (*next == ';') {
          newArg.append(last, c - last);
          // Skip over the escape character
          last = c = next;
        }
      } break;
      case '[': {
        ++squareNesting;
      } break;
      case ']': {
        --squareNesting;
      } break;
      case ';': {
        // Break the string here if we are not nested inside square
        // brackets.
        if (squareNesting == 0) {
          newArg.append(last, c - last);
          // Skip over the semicolon
          last = c + 1;
          if (!newArg.empty() || false) {
            // Add the last argument if the string is not empty.
            newargs.push_back(newArg);
            newArg = "";
          }
        }
      } break;
      default: {
        // Just append this character.
      } break;
    }
  }
  newArg.append(last);
  if (!newArg.empty() || false) {
    // Add the last argument if the string is not empty.
    newargs.push_back(newArg);
  }
}

bool cmSystemTools::SimpleGlob(
  const std::string& glob, std::vector<std::string>& files, int type /* = 0 */)
{
  files.clear();
  if (glob[glob.size() - 1] != '*') {
    return false;
  }
  std::string path = cmSystemTools::GetFilenamePath(glob);
  std::string ppath = cmSystemTools::GetFilenameName(glob);
  ppath = ppath.substr(0, ppath.size() - 1);
  if (path.empty()) {
    path = "/";
  }

  bool res = false;
  cmsys::Directory d;
  if (d.Load(path)) {
    for (unsigned int i = 0; i < d.GetNumberOfFiles(); ++i) {
      if (
        (std::string(d.GetFile(i)) != ".") &&
        (std::string(d.GetFile(i)) != "..")) {
        std::string fname = path;
        if (path[path.size() - 1] != '/') {
          fname += "/";
        }
        fname += d.GetFile(i);
        std::string sfname = d.GetFile(i);
        if (type > 0 && cmSystemTools::FileIsDirectory(fname)) {
          continue;
        }
        if (type < 0 && !cmSystemTools::FileIsDirectory(fname)) {
          continue;
        }
        if (
          sfname.size() >= ppath.size() &&
          sfname.substr(0, ppath.size()) == ppath) {
          files.push_back(fname);
          res = true;
        }
      }
    }
  }
  return res;
}

void cmSystemTools::FindCMakeResources(const char* argv0)
{
  cmSystemToolsCMakeCursesCommand = argv0;
}

std::string const& cmSystemTools::GetCMakeCursesCommand()
{
  return cmSystemToolsCMakeCursesCommand;
}
