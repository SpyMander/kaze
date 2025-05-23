#include"error_handling.hpp"
#include<iostream>
#include<SDL3/SDL_error.h>

void kaze::errorExit(const char* m) {
  std::cerr << "[FATAL]: " << m << std::endl;
  std::cerr << SDL_GetError() << std::endl;
  exit(1);
}
