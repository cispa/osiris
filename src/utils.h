// Copyright 2021 Daniel Weber, Ahmad Ibrahim, Hamed Nemati, Michael Schwarz, Christian Rossow
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
//     limitations under the License.


#ifndef OSIRIS_SRC_UTILS_H_
#define OSIRIS_SRC_UTILS_H_

#include <algorithm>
#include <string>
#include <vector>

namespace osiris {

///
/// represents ByteArrays
///
using byte_array = std::vector<std::byte>;

/// Create ByteArray
/// \param byte_arr bytes as char array
/// \param arr_len char array length
/// \return created ByteArray
osiris::byte_array CreateBytearray(const char* byte_arr, int arr_len);

/// Converts ByteArray to String
/// \param bytes ByteArray to be converted
/// \return created String
std::string BytearrayToString(const osiris::byte_array& bytes);

/// Splits a string on a given delimiter
/// \param input_str string to split
/// \param delimiter delimiter character
/// \return vector of splitted strings
std::vector<std::string> SplitString(const std::string& input_str, char delimiter);

/// Encodes a number in Little Endian format
/// \param number number
/// \param result_length byte length of the result
/// \return Little Endian bytes
byte_array NumberToBytesLE(uint64_t number, size_t result_length);

/// Decodes base64 to ByteArray
/// \param encoded_string base64 string
/// \return ByteArray
byte_array base64_decode(std::string const& encoded_string);

/// Encodes ByteArray as base64 string
/// \param bytes_to_encode ByteArray to encode
/// \return base64 string
std::string base64_encode(const byte_array& bytes_to_encode);

/// Calculate the SHA256 hash of a given file
/// \param filename filename to calculate hash from
/// \return upon failure returns empty string
std::string CalculateFileHashSHA256(const std::string& filename);

/// Calculates the median of a given vector
/// \tparam T type of the vector elements
/// \param values list of values
/// \return median
template<class T>
double median(std::vector<T> values) {
  if (values.empty()) {
    return 0;
  }
  std::sort(values.begin(), values.end());
  if (values.size() % 2 == 0) {
    return static_cast<double>((values[(values.size() - 1) / 2] + values[values.size() / 2])) / 2;
  } else {
    return values[values.size() / 2];
  }
}

}  // namespace osiris

#endif //OSIRIS_SRC_UTILS_H_
