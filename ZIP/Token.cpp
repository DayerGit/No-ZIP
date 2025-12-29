#include "Token.h"
#include "files.h"

bool CheckToken(FileToken _token) {
    if (_token >= files.size()) return false;

    auto it = std::find(freeTokens.begin(), freeTokens.end(), _token);
    if (it != freeTokens.end()) return false;

    if (!files[_token].file) return false;

    return true;
}