#pragma once
/**
 * @file Blurhash.hpp
 * @brief Compact blurhash encoder/decoder for progressive image loading
 *
 * Blurhash is a compact representation of a placeholder for an image.
 * Algorithm specification: https://github.com/woltapp/blurhash
 *
 * This is an independent C++20 implementation of the public-domain algorithm.
 */

#include <cstdint>
#include <string>
#include <vector>

namespace bwp::utils {

namespace blurhash {

/**
 * @brief Decode a blurhash string into raw RGBA pixel data
 * @param hash   The blurhash string (e.g. "LEHV6nWB2yk8pyoJadR*.7kCMdnj")
 * @param width  Output image width in pixels
 * @param height Output image height in pixels
 * @param punch  Contrast factor (1.0 = normal, >1 = more vivid)
 * @return RGBA pixel data (width * height * 4 bytes), empty on error
 */
std::vector<uint8_t> decode(const std::string &hash, int width, int height,
                            float punch = 1.0f);

/**
 * @brief Encode raw RGB pixel data into a blurhash string
 * @param pixels     RGB pixel data (width * height * 3 bytes)
 * @param width      Image width in pixels
 * @param height     Image height in pixels
 * @param componentsX Horizontal component count (1-9, typically 4)
 * @param componentsY Vertical component count (1-9, typically 3)
 * @return Blurhash string, empty on error
 */
std::string encode(const uint8_t *pixels, int width, int height,
                   int componentsX, int componentsY);

/**
 * @brief Validate a blurhash string
 * @param hash The blurhash string to validate
 * @return true if the string is a valid blurhash
 */
bool isValid(const std::string &hash);

} // namespace blurhash

} // namespace bwp::utils
