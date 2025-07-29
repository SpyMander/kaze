#include "logging.hpp"
#include <filesystem>
#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/compile.h>
#include <fmt/core.h>
#include <fstream>

namespace kaze {
  void errorExit(const char* m);
}

std::ofstream gLogStream;
LogFlag gLogFlags;

const char gInfoText[] = "[KZINFO] ";
const fmt::text_style gInfoColor = fmt::fg(fmt::color::cadet_blue);

const char gWarnText[] = "[KZWARN] ";
const fmt::text_style gWarnColor = fmt::fg(fmt::color::yellow);

const char gFatalText[] = "[KZFATAL] ";
const fmt::text_style gFatalColor = fmt::fg(fmt::color::red);

void kaze::logInfo(const std::string& m) {
  if (gLogFlags & kaze::LogFlagBits::Info) {
    fmt::print(fmt::emphasis::bold | gInfoColor, "{}", gInfoText);
    fmt::print("{}\n", m);
  }

  if (gLogFlags & kaze::LogFlagBits::FileInfo && gLogStream.is_open())
    gLogStream << fmt::format("{} {}\n", gInfoText, m);
}

void kaze::logWarn(const std::string& m) {
  if (gLogFlags & kaze::LogFlagBits::Warn) {
    fmt::print(fmt::emphasis::bold | gWarnColor, "{}", gWarnText);
    fmt::print("{}\n", m);
  }
  if (gLogFlags & kaze::LogFlagBits::FileWarn && gLogStream.is_open()) 
    gLogStream << fmt::format("{} {}\n", gWarnText, m);
}

void kaze::logFatal(const std::string& m) {
  if (gLogFlags & kaze::LogFlagBits::Fatal) {
    fmt::print(fmt::emphasis::bold | gFatalColor,"{}", gFatalText);
    fmt::print("{}\n", m);
  }

  if (gLogFlags & kaze::LogFlagBits::Warn && gLogStream.is_open())
    gLogStream << fmt::format("{} {}\n", gFatalText, m);
}

void kaze::setLogFile(std::filesystem::path path) {
  if (gLogStream.is_open())
    gLogStream.close();

  if (std::filesystem::exists(path) &&
      std::filesystem::is_character_file(path))
    gLogStream = std::ofstream(path);
  else {
    gLogStream = std::ofstream(path);
    if (gLogStream) {
      kaze::logInfo("log file wasn't here, so i created one.");
    } else {
      kaze::errorExit("unable to create log file");
    }
  }
}

void kaze::setLoggingMode(LogFlag flag) {
  gLogFlags = flag;
}
