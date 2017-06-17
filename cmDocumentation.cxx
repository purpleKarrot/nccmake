/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmDocumentation.h"

#include <cstring>
#include <ostream>

bool cmDocumentation::CheckOptions(int argc, const char* const* argv)
{
  if (argc == 1) {
    this->ShowUsage = true;
    return true;
  }

  for (int i = 1; i < argc; ++i) {
    if (
      (strcmp(argv[i], "-help") == 0) || (strcmp(argv[i], "--help") == 0) ||
      (strcmp(argv[i], "/?") == 0) || (strcmp(argv[i], "-usage") == 0) ||
      (strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "-H") == 0)) {
      return true;
    }

    if (
      (strcmp(argv[i], "--version") == 0) ||
      (strcmp(argv[i], "-version") == 0) || (strcmp(argv[i], "/V") == 0)) {
      this->ShowVersion = true;
      return true;
    }
  }

  return false;
}

bool cmDocumentation::PrintRequestedDocumentation(std::ostream& os)
{
  if (this->ShowVersion) {
    os << "nccmake version 1.0\n";
    return true;
  }

  os << //
    "Usage\n"
    "\n"
    "  nccmake <path-to-source>\n"
    "  nccmake <path-to-existing-build>\n"
    "\n"
    "Specify a source directory to (re-)generate a build system for it in\n"
    "the current working directory.  Specify an existing build directory to\n"
    "re-generate its build system.\n";

  if (this->ShowUsage) {
    os << "\nRun 'nccmake --help' for more information.\n";
    return false;
  }

  os << //
    "\nOptions\n"
    "  -C <initial-cache>           = Pre-load a script to populate the "
    "cache.\n"
    "  -D <var>[:<type>]=<value>    = Create a cmake cache entry.\n"
    "  -U <globbing_expr>           = Remove matching entries from CMake "
    "cache.\n"
    "  -G <generator-name>          = Specify a build system generator.\n"
    "  -T <toolset-name>            = Specify toolset name if supported by\n"
    "                                 generator.\n"
    "  -A <platform-name>           = Specify platform name if supported by\n"
    "                                 generator.\n"
    "  --help,-help,-usage,-h,-H,/? = Print usage information and exit.\n"
    "  --version,-version,/V [<f>]  = Print version number and exit.\n";
  return true;
}
