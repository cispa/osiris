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


#ifndef OSIRIS_SRC_CODE_GENERATOR_H_
#define OSIRIS_SRC_CODE_GENERATOR_H_

#include <unordered_map>
#include <vector>
#include <string>
#include <random>

#include "utils.h"

namespace osiris {

///
/// mark the start of the memory range accessed by instructions
///
constexpr uint64_t kMemoryBegin = 0x13370000;

///
/// mark the start of the memory range accessed by instructions
///
constexpr uint64_t kMemoryEnd = 0x13371fff;

////
/// Represents one x86 instruction
///
struct x86Instruction {
  uint64_t instruction_uid;
  byte_array byte_representation;
  std::string assembly_code;
  std::string category;
  std::string extension;
  std::string isa_set;

  std::string GetCSVRepresentation() const;
};

///
/// Generates assembly code in binary format
///
class CodeGenerator {
 public:
  /// Initializes Codegenerator with given instruction list
  /// \param instructions_filename file containing base64 encoded instructions one per line
  explicit CodeGenerator(const std::string &instructions_filename);

  /// Create instruction from instruction list
  /// \param instruction_idx instruction index
  /// \return corresponding instruction
  x86Instruction CreateInstructionFromIndex(size_t instruction_idx);

  /// Create instruction from instruction UID
  /// \param instruction_idx instruction UID
  /// \return corresponding instruction
  x86Instruction CreateInstructionFromUID(uint64_t instruction_uid);

  /// Create random instruction
  /// \return random instruction
  x86Instruction CreateRandomInstruction();

  /// Get number of Instructions that were loaded to the codegen
  /// \return no of instructions
  size_t GetNumberOfInstructions();

 private:
  int GenerateRandomNumber(int min, int max);

  /// We add a checksum over the instruction input file to our instruction UIDs to detect when the
  /// user tries to use the wrong combination of instruction file and UIDs
  /// \param instruction_idx
  /// \return
  uint64_t GenerateInstructionUID(size_t instruction_idx);
  size_t InstructionUIDToInstructionIndex(uint64_t instruction_uid);

  std::vector<x86Instruction> instruction_list_;
  std::default_random_engine rand_generator_;
  std::string instruction_file_sha256hash_;
};

}  // namespace osiris

#endif  // OSIRIS_SRC_CODE_GENERATOR_H_
