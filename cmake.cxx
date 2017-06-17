/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmake.h"

#include <cstdio>
#include <iostream>
#include <json/reader.h>
#include <json/writer.h>
#include <map>

#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"

namespace {

void on_alloc(uv_handle_t*, size_t suggested_size, uv_buf_t* buf)
{
  buf->base = new char[suggested_size];
  buf->len = suggested_size;
}

void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
  if (nread > 0) {
    reinterpret_cast<cmake*>(stream->data)->ReadData(buf->base, nread);
  }

  if (nread < 0) {
    if (nread != UV_EOF) {
      fprintf(stderr, "Read error %s\n", uv_err_name(nread));
    }
    uv_close(reinterpret_cast<uv_handle_t*>(stream), nullptr);
  }

  delete[] buf->base;
}

void on_process_close(uv_handle_t* handle)
{
  delete reinterpret_cast<uv_process_t*>(handle);
}

void on_exit(uv_process_t* req, int64_t exit_status, int term_signal)
{
  std::cerr << "Process exited with status " << exit_status << ", signal "
            << term_signal << ".\n";
  uv_close(reinterpret_cast<uv_handle_t*>(req), on_process_close);
}

#define START_MAGIC "[== \"CMake Server\" ==["
#define END_MAGIC "]== \"CMake Server\" ==]"

} // namespace

cmake::cmake(Role role)
{
  if (role != RoleProject) {
    return;
  }

  this->State.reset(new cmState);

  uv_loop_t* loop = uv_default_loop();
  uv_pipe_init(loop, &this->ServerInput, 0);
  uv_pipe_init(loop, &this->ServerOutput, 0);
  this->ServerOutput.data = this;

  const char* args[]{"cmake",          "-E",      "server",
                     "--experimental", "--debug", nullptr};

  uv_process_options_t options{};
  options.exit_cb = on_exit;
  options.file = "cmake";
  options.args = const_cast<char**>(args);
  // options.cwd = this->BuildDirectory.c_str();

  uv_stdio_container_t stdio[3];
  stdio[0].flags =
    static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE);
  stdio[0].data.stream = reinterpret_cast<uv_stream_t*>(&this->ServerInput);
  stdio[1].flags =
    static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  stdio[1].data.stream = reinterpret_cast<uv_stream_t*>(&this->ServerOutput);
  stdio[2].flags = UV_INHERIT_FD;
  stdio[2].data.fd = 2;
  options.stdio = stdio;
  options.stdio_count = 3;

  auto* process = new uv_process_t;

  int r;
  if ((r = uv_spawn(loop, process, &options))) {
    fprintf(stderr, "%s\n", uv_strerror(r));
    // return 1;
  } else {
    fprintf(stderr, "Launched process with ID %d\n", process->pid);
  }

  uv_read_start(
    reinterpret_cast<uv_stream_t*>(&this->ServerOutput), on_alloc, on_read);

  uv_run(loop, UV_RUN_DEFAULT);
}

cmake::~cmake()
{
  uv_loop_close(uv_default_loop());
}

void cmake::SetArgs(std::vector<std::string> const& args)
{
  std::string generator;
  std::string extra_generator;
  std::string toolset;
  std::string platform;

  for (std::size_t i = 1; i < args.size(); ++i) {
    std::string const& arg = args[i];

    if (arg[0] == '-' && (arg[1] == 'C' || arg[1] == 'D' || arg[1] == 'U')) {
      this->CacheArguments.append(arg);
      if (arg.size() == 2) {
        i += 1;
        if (i >= args.size()) {
          cmSystemTools::Error("No argument specified for ", arg.c_str());
          return;
        }
        this->CacheArguments.append(args[i]);
      }
      continue;
    }

    if (arg.find("-G", 0) == 0) {
      std::string value = arg.substr(2);
      if (value.empty()) {
        ++i;
        if (i >= args.size()) {
          cmSystemTools::Error("No argument specified for ", "-G");
          return;
        }
        value = args[i];
      }
      if (!generator.empty()) {
        cmSystemTools::Error("-G", " may only be used once.");
        return;
      }
      // TODO: split extra_generator after -
      generator = value;
      continue;
    }

    if (arg.find("-T", 0) == 0) {
      std::string value = arg.substr(2);
      if (value.empty()) {
        ++i;
        if (i >= args.size()) {
          cmSystemTools::Error("No argument specified for ", "-T");
          return;
        }
        value = args[i];
      }
      if (!generator.empty()) {
        cmSystemTools::Error("-T", " may only be used once.");
        return;
      }
      toolset = value;
      continue;
    }

    if (arg.find("-A", 0) == 0) {
      std::string value = arg.substr(2);
      if (value.empty()) {
        ++i;
        if (i >= args.size()) {
          cmSystemTools::Error("No argument specified for ", "-A");
          return;
        }
        value = args[i];
      }
      if (!generator.empty()) {
        cmSystemTools::Error("-A", " may only be used once.");
        return;
      }
      platform = value;
      continue;
    }

    // no option assume it is the path to the source
    this->SetDirectoriesFromFile(arg);
  }

  Json::Value protocol_version = Json::objectValue;
  protocol_version["major"] = 1;
  protocol_version["minor"] = 0;

  Json::Value body = Json::objectValue;
  body["protocolVersion"] = protocol_version;

  if (!this->SourceDirectory.empty()) {
    body["sourceDirectory"] = this->SourceDirectory;
  }

  body["buildDirectory"] = this->BinaryDirectory;

  if (!generator.empty()) {
    body["generator"] = generator;
  }

  if (!extra_generator.empty()) {
    body["extraGenerator"] = extra_generator;
  }

  if (!platform.empty()) {
    body["platform"] = platform;
  }

  if (!toolset.empty()) {
    body["toolset"] = toolset;
  }

  this->SendRequest("handshake", body);
  this->SendRequest("cache");
}

void cmake::SetDirectoriesFromFile(std::string const& arg)
{
  // Check if the argument refers to a CMakeCache.txt or
  // CMakeLists.txt file.
  std::string listPath;
  std::string cachePath;
  bool argIsFile = false;
  if (cmSystemTools::FileIsDirectory(arg)) {
    std::string path = cmSystemTools::CollapseFullPath(arg);
    cmSystemTools::ConvertToUnixSlashes(path);
    std::string cacheFile = path;
    cacheFile += "/CMakeCache.txt";
    std::string listFile = path;
    listFile += "/CMakeLists.txt";
    if (cmSystemTools::FileExists(cacheFile.c_str())) {
      cachePath = path;
    }
    if (cmSystemTools::FileExists(listFile.c_str())) {
      listPath = path;
    }
  } else if (cmSystemTools::FileExists(arg)) {
    argIsFile = true;
    std::string fullPath = cmSystemTools::CollapseFullPath(arg);
    std::string name = cmSystemTools::GetFilenameName(fullPath);
    name = cmSystemTools::LowerCase(name);
    if (name == "cmakecache.txt") {
      cachePath = cmSystemTools::GetFilenamePath(fullPath);
    } else if (name == "cmakelists.txt") {
      listPath = cmSystemTools::GetFilenamePath(fullPath);
    }
  } else {
    // Specified file or directory does not exist.  Try to set things
    // up to produce a meaningful error message.
    std::string fullPath = cmSystemTools::CollapseFullPath(arg);
    std::string name = cmSystemTools::GetFilenameName(fullPath);
    name = cmSystemTools::LowerCase(name);
    if (name == "cmakecache.txt" || name == "cmakelists.txt") {
      argIsFile = true;
      listPath = cmSystemTools::GetFilenamePath(fullPath);
    } else {
      listPath = fullPath;
    }
  }

  // If there is a CMakeCache.txt file, use its settings.
  if (!cachePath.empty()) {
    this->SetHomeOutputDirectory(cachePath);
    return;
  }

  // If there is a CMakeLists.txt file, use it as the source tree.
  if (!listPath.empty()) {
    this->SetHomeDirectory(listPath);

    if (argIsFile) {
      // Source CMakeLists.txt file given.  It was probably dropped
      // onto the executable in a GUI.  Default to an in-source build.
      this->SetHomeOutputDirectory(listPath);
    } else {
      // Source directory given on command line.  Use current working
      // directory as build tree.
      std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();
      this->SetHomeOutputDirectory(cwd);
    }
    return;
  }

  // We didn't find a CMakeLists.txt or CMakeCache.txt file from the
  // argument.  Assume it is the path to the source tree, and use the
  // current working directory as the build tree.
  std::string full = cmSystemTools::CollapseFullPath(arg);
  std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();
  this->SetHomeDirectory(full);
  this->SetHomeOutputDirectory(cwd);
}

int cmake::Configure()
{
  for (auto const& entry : this->State->GetCache()) {
    if (entry.second.IsRemoved) {
      this->CacheArguments.append("-U" + entry.first);
    }
    if (entry.second.IsModified) {
      this->CacheArguments.append(
        "-D" + entry.first + "=" + entry.second.Value);
    }
  }

  Json::Value data = Json::objectValue;
  data["cacheArguments"] = this->CacheArguments;
  this->SendRequest("configure", data);
  this->SendRequest("cache");
  this->CacheArguments.clear();
  return 0;
}

int cmake::Generate()
{
  this->SendRequest("compute");
  return 0;
}

void cmake::ReadData(const char* data, ssize_t len)
{
  this->RawReadBuffer.append(data, len);

  for (;;) {
    auto needle = this->RawReadBuffer.find('\n');
    if (needle == std::string::npos) {
      return;
    }

    std::string line = this->RawReadBuffer.substr(0, needle);
    const auto ls = line.size();
    if (ls > 1 && line.at(ls - 1) == '\r') {
      line.erase(ls - 1, 1);
    }

    this->RawReadBuffer.erase(
      this->RawReadBuffer.begin(),
      this->RawReadBuffer.begin() + static_cast<long>(needle) + 1);

    if (line == START_MAGIC) {
      this->RequestBuffer.clear();
      continue;
    }

    if (line == END_MAGIC) {
      this->HandleResponse(this->RequestBuffer);
      this->RequestBuffer.clear();
    } else {
      this->RequestBuffer += line;
      this->RequestBuffer += "\n";
    }
  }
}

void cmake::HandleResponse(std::string const& input)
{
  Json::Value value;
  Json::Reader reader;
  if (!reader.parse(input, value)) {
    // this->WriteParseError("Failed to parse JSON input.");
    return;
  }

  std::string const type = value["type"].asString();
  if (type == "hello") {
    this->HandleHello(value);
    return;
  }
  if (type == "signal") {
    this->HandleSignal(value);
    return;
  }

  std::string const reply_to = value["inReplyTo"].asString();
  if (reply_to != this->ExpectedReply) {
  }

  if (type == "reply") {
    this->HandleReply(reply_to, value);
    return;
  }
  if (type == "error") {
    this->HandleError(value);
    return;
  }
  if (type == "message") {
    this->HandleMessage(value);
    return;
  }
  if (type == "progress") {
    this->HandleProgress(value);
    return;
  }
  // reportError("Got a message of an unknown type.");
}

void cmake::HandleHello(Json::Value const&  /*data*/)
{
  uv_stop(uv_default_loop());
}

void cmake::HandleReply(std::string const& type, Json::Value const& data)
{
  uv_stop(uv_default_loop());
  if (type == "cache") {
    this->ReadCache(data["cache"]);
  }
}

void cmake::HandleError(Json::Value const& data)
{
  std::string const error_message = data["errorMessage"].asString();
  cmSystemTools::Error("Server Error: ", error_message.c_str());
}

void cmake::HandleMessage(Json::Value const& data)
{
  std::string const title = data["title"].asString();
  std::string const message = data["message"].asString();
  this->ProgressMessage = message;
  if (this->ProgressCallback) {
    this->ProgressCallback(
      this->ProgressMessage.c_str(), this->Progress, this->ProgressUserData);
  }
}

void cmake::HandleProgress(Json::Value const& data)
{
  int const current = data["progressCurrent"].asInt();
  int const maximum = data["progressMaximum"].asInt();
  int const minimum = data["progressMinimum"].asInt();
  this->Progress = float(current) / float(maximum);
  if (this->ProgressCallback) {
    this->ProgressCallback(
      this->ProgressMessage.c_str(), this->Progress, this->ProgressUserData);
  }
}

void cmake::HandleSignal(Json::Value const&  /*data*/)
{
}

void cmake::SendRequest(std::string const& type, Json::Value extra)
{
  this->ExpectedReply = type;
  extra["type"] = type;

  Json::FastWriter writer;
  std::string const raw =
    "\n" START_MAGIC "\n" + writer.write(extra) + END_MAGIC "\n";

  uv_write_t req;
  uv_buf_t const buf = uv_buf_init(const_cast<char*>(raw.data()), raw.size());
  uv_write(
    &req, reinterpret_cast<uv_stream_t*>(&this->ServerInput), &buf, 1,
    nullptr);

  uv_read_start(
    reinterpret_cast<uv_stream_t*>(&this->ServerOutput), on_alloc, on_read);

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

static cmStateEnums::CacheEntryType parse_type(std::string const& str)
{
  if (str == "BOOL") {
    return cmStateEnums::BOOL;
  }
  if (str == "PATH") {
    return cmStateEnums::PATH;
  }
  if (str == "FILEPATH") {
    return cmStateEnums::FILEPATH;
  }
  if (str == "STRING") {
    return cmStateEnums::STRING;
  }
  if (str == "INTERNAL") {
    return cmStateEnums::INTERNAL;
  }
  if (str == "STATIC") {
    return cmStateEnums::STATIC;
  }
  return cmStateEnums::UNINITIALIZED;
}

void cmake::ReadCache(Json::Value const& json)
{
  auto& cache = this->State->GetCache();
  cache.clear();

  for (Json::Value elem : json) {
    std::string const key = elem["key"].asString();
    auto& cache_entry = cache[key];
    cache_entry.Type = parse_type(elem["type"].asString());
    cache_entry.Value = elem["value"].asString();
    cache_entry.Strings = elem["properties"]["STRINGS"].asString();
    cache_entry.HelpString = elem["properties"]["HELPSTRING"].asString();
    cache_entry.IsAdvanced =
      cmSystemTools::IsOn(elem["properties"]["ADVANCED"].asString());
  }
}
