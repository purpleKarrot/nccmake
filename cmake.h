/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmake_h
#define cmake_h

#include <memory>
#include <string>
#include <vector>

#include <json/value.h>
#include <uv.h>

class cmState;
struct cmDocumentationEntry;

class cmake
{
public:
  enum Role
  {
    RoleInternal,
    RoleProject
  };

  cmake(Role role);
  ~cmake();

  cmake(cmake const&) = delete;
  cmake& operator=(cmake const&) = delete;

  void SetArgs(std::vector<std::string> const& args);
  void SetDirectoriesFromFile(std::string const& arg);

  int Configure();
  int Generate();

  void ReadData(const char* data, ssize_t len);

public:
  typedef void (*ProgressCallbackType)(const char* msg, float progress, void*);
  void SetProgressCallback(ProgressCallbackType callback, void* clientData)
  {
    this->ProgressCallback = callback;
    this->ProgressUserData = clientData;
  }

  void SetHomeDirectory(std::string const& dir)
  {
    this->SourceDirectory = dir;
  }

  void SetHomeOutputDirectory(std::string const& dir)
  {
    this->BinaryDirectory = dir;
  }

  const char* GetHomeOutputDirectory() const
  {
    return this->BinaryDirectory.c_str();
  }

  cmState* GetState() { return this->State.get(); }

  int DoPreConfigureChecks() { return 0; }
  int LoadCache() { return 0; }
  void AddCMakePaths() {}
  void GetGeneratorDocumentation(std::vector<cmDocumentationEntry> const&) {}
  void PreLoadCMakeFiles() {}
  void SaveCache(std::string const&) {}
  void SetCMakeEditCommand(std::string const&) {}
  void SetCacheArgs(std::vector<std::string> const&) {}
  void UnwatchUnusedCli(const std::string& var) {}

private:
  void HandleResponse(std::string const& str);

  void HandleHello(Json::Value const& data);
  void HandleReply(std::string const& type, Json::Value const& data);
  void HandleError(Json::Value const& data);
  void HandleMessage(Json::Value const& data);
  void HandleProgress(Json::Value const& data);
  void HandleSignal(Json::Value const& data);

  void SendRequest(
    std::string const& type, Json::Value extra = Json::objectValue);

  void ReadCache(Json::Value const& json);

private:
  std::string SourceDirectory;
  std::string BinaryDirectory;

  uv_pipe_t ServerInput;
  uv_pipe_t ServerOutput;

  float Progress = 0;
  std::string ProgressMessage;
  ProgressCallbackType ProgressCallback;
  void* ProgressUserData = nullptr;

  Json::Value CacheArguments = Json::arrayValue;

  std::unique_ptr<cmState> State;

  std::string ExpectedReply;
  std::string RawReadBuffer;
  std::string RequestBuffer;
};

#endif
