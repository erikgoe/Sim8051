#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <array>
#include <sstream>
#include <functional>
#include <fstream>
#include <filesystem>
#include <algorithm>

using size_t = std::size_t;

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
