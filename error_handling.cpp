#include "error_handling.hpp"
#include "logging.hpp"
#include <iostream>
#include <SDL3/SDL_error.h>

// TODO: better printing of errors,
// print to file, color?
// formating??
// vulkan validation errors, create borders around them

// generally this is a logging issue.
// use cpp 23 print; 

void kaze::errorExit(const char* m) {
  kaze::logFatal(m);
  exit(1);
}
