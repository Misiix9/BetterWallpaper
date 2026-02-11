#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace bwp::utils {
namespace blurhash {
std::vector<uint8_t> decode(const std::string &hash, int width, int height,
                            float punch = 1.0f);
std::string encode(const uint8_t *pixels, int width, int height,
                   int componentsX, int componentsY);
bool isValid(const std::string &hash);
}  
}  
