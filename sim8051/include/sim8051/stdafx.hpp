#pragma once

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <functional>
#include <atomic>
#include <future>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fstream>
#include <filesystem>
#include <random>

#include "SFML/System.hpp"
#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"
#include "imgui-SFML.h"
#include "imgui.h"

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using i8 = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;

using f32 = float;
using f64 = double;

using String = std::string;

using std::stod;
using std::stof;
using std::stoi;
using std::stol;
using std::stold;
using std::stoll;
using std::stoul;
using std::stoull;
using std::to_string;

// Log a message
void log( const String &str );
