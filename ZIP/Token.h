#pragma once
#include <deque>

using FileToken = unsigned long long int;

inline std::deque<FileToken> freeTokens;
bool CheckToken(FileToken _token);