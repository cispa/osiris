#include "utils.h"

#include <openssl/sha.h>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

#include "logger.h"

namespace osiris {

osiris::byte_array CreateBytearray(const char* byte_arr, int arr_len) {
  osiris::byte_array res;
  for (int i = 0; i < arr_len; i++) {
    res.push_back(std::byte{static_cast<unsigned char>(byte_arr[i])});
  }
  return res;
}

std::string BytearrayToString(const osiris::byte_array& bytes) {
  std::stringstream res;
  for (const std::byte& b : bytes) {
    res << static_cast<char>(std::to_integer<int>(b));
  }
  return res.str();
}

byte_array NumberToBytesLE(uint64_t number, size_t result_length) {
  byte_array bytes;
  for (size_t i = 0; i < result_length; i++) {
    uint8_t pos = number & 0xff;
    bytes.push_back(std::byte{pos});
    number >>= 8;
  }
  return bytes;
}

std::vector<std::string> SplitString(const std::string& input_str, char delimiter) {
  std::string delims;
  delims += delimiter;
  std::vector<std::string> results;

  // pre-reserve for performance reasons
  results.reserve(16);

  std::string::const_iterator start = input_str.begin();
  std::string::const_iterator end = input_str.end();
  std::string::const_iterator next = std::find(start, end, delimiter);
  while (next != end) {
    results.emplace_back(start, next);
    start = next + 1;
    next = std::find(start, end, delimiter);
  }
  results.emplace_back(start, next);
  return results;
}

/*
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch
   ------------------------------------------------------------------------------------------------
   src: https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp/
   NOTE: Osiris dev changed the return type to byte_array
*/
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

byte_array base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  byte_array ret;

  while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret.push_back(std::byte{char_array_3[i]});
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++)
      char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) {
      ret.push_back(std::byte{char_array_3[j]});
    }
  }

  return ret;
}

std::string base64_encode(const byte_array& bytes_to_encode) {
  size_t len_encoded = (bytes_to_encode.size() + 2) / 3 * 4;
  unsigned char trailing_char = '=';
  std::string ret;
  ret.reserve(len_encoded);

  unsigned int pos = 0;
  while (pos < bytes_to_encode.size()) {
    ret.push_back(base64_chars[(std::to_integer<int>(bytes_to_encode[pos + 0]) & 0xfc) >> 2]);

    if (pos + 1 < bytes_to_encode.size()) {
      ret.push_back(
          base64_chars[((std::to_integer<int>(bytes_to_encode[pos + 0]) & 0x03) << 4)
              + ((std::to_integer<int>(bytes_to_encode[pos + 1]) & 0xf0) >> 4)]);

      if (pos + 2 < bytes_to_encode.size()) {
        ret.push_back(
            base64_chars[((std::to_integer<int>(bytes_to_encode[pos + 1]) & 0x0f) << 2)
                + ((std::to_integer<int>(bytes_to_encode[pos + 2]) & 0xc0) >> 6)]);
        ret.push_back(base64_chars[std::to_integer<int>(bytes_to_encode[pos + 2]) & 0x3f]);
      } else {
        ret.push_back(
            base64_chars[(std::to_integer<int>(bytes_to_encode[pos + 1]) & 0x0f) << 2]);
        ret.push_back(trailing_char);
      }
    } else {
      ret.push_back(
          base64_chars[(std::to_integer<int>(bytes_to_encode[pos + 0]) & 0x03) << 4]);
      ret.push_back(trailing_char);
      ret.push_back(trailing_char);
    }
    pos += 3;
  }
  return ret;
}

std::string CalculateFileHashSHA256(const std::string& filename) {
  std::ifstream file_stream(filename);
  if (!file_stream.is_open()) {
    LOG_ERROR("Could not access file for hashing.");
    // return empty string on error
    return std::string();
  }

  // initialize hash
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);

  // hash file content on a block per block basis
  constexpr size_t read_buffer_size = 4096 * 10;
  char read_buffer[read_buffer_size];
  while (!file_stream.eof()) {
    file_stream.read(read_buffer, read_buffer_size);
    size_t bytes_read = file_stream.gcount();
    SHA256_Update(&sha256, read_buffer, bytes_read);
  }

  // retrieve resulting hash
  SHA256_Final(hash, &sha256);

  // format hash to hexdigest
  std::stringstream ss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int) hash[i];
  }
  return ss.str();

}

}  // namespace osiris
