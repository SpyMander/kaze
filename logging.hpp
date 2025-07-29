#pragma once
#include <string>
#include <filesystem>
#include <fmt/base.h>

typedef uint8_t LogFlag;

namespace kaze {
  typedef enum LogFlagBits : uint8_t {
    None      = 0,
    FileInfo  = 1 << 0,
    FileWarn  = 1 << 1,
    FileFatal = 1 << 2,
    Info      = 1 << 3,
    Warn      = 1 << 4,
    Fatal     = 1 << 5,
    All       = FileInfo | FileWarn | FileFatal | Info | Warn | Fatal,
  } LogFlagBits;

  void logInfo(const std::string& m);
  void logWarn(const std::string& m);
  void logFatal(const std::string& m);


  void logFlush();

  void setLogFile(std::filesystem::path filepath);
  void setLogfd(int fd);
  void setLoggingMode(LogFlag flags);
};
