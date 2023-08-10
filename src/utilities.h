#pragma once

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#include <functional>
#include <iostream>
#include <string>

template <typename T, typename... Rest> void HashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
	seed ^= std::hash<T> {}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	(HashCombine(seed, rest), ...);
};

#ifndef NDEBUG
	#if defined(_WIN32)
#	define ASSERT(condition)                                                                                                                   \
		if(!(condition)) {                                                                                                                      \
			std::cout << (std::string(#condition) + " ON LINE " + std::to_string(__LINE__) + " IN FILE " + std::string(__FILE__)) << std::endl; \
			__debugbreak();                                                                                                                   \
		}    // I believe this will be __debugbreak() on windows
	#else
#	define ASSERT(condition)                                                                                                                   \
		if(!(condition)) {                                                                                                                      \
			std::cout << (std::string(#condition) + " ON LINE " + std::to_string(__LINE__) + " IN FILE " + std::string(__FILE__)) << std::endl; \
			__builtin_trap();                                                                                                                   \
		}    // I believe this will be __debugbreak() on windows
	#endif 

#else
#	define ASSERT(condition)
#endif

struct WindowInfo {
	GLFWwindow* windowPtr;
	glm::vec2 windowSize;
};