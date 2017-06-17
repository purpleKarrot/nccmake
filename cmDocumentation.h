/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmDocumentation_h
#define cmDocumentation_h

#include <iosfwd>
#include <string>
#include <vector>

struct cmDocumentationEntry;

class cmDocumentation
{
public:
  /** Set the program name for standard document generation.  */
  void SetName(const std::string& name) {}

  bool CheckOptions(int argc, const char* const* argv);

  void addCMakeStandardDocSections() {}
  void AppendSection(const char*, const char* [][2]) {}
  void PrependSection(const char*, const char* [][2]) {}
  void SetSection(const char*, std::vector<cmDocumentationEntry> const&) {}
  void SetSection(const char*, const char* [][2]) {}

  bool PrintRequestedDocumentation(std::ostream& os);

private:
  bool ShowUsage = false;
  bool ShowVersion = false;
};

#endif
