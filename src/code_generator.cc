#include "code_generator.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "logger.h"
#include "utils.h"

namespace osiris {

std::string x86Instruction::GetCSVRepresentation() const {
  std::stringstream line;
  line << std::hex << instruction_uid;
  line << ";";

  line << assembly_code;
  line << ";";

  line << category;
  line << ";";

  line << extension;
  line << ";";

  line << isa_set;
  return line.str();
}

CodeGenerator::CodeGenerator(const std::string& instructions_filename) {
  // seed rng
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  rand_generator_ = std::default_random_engine(seed);

  // calculate hash of the instruction file (required for instruction UID)
  instruction_file_sha256hash_ = CalculateFileHashSHA256(instructions_filename);

  // load instructions from file
  std::ifstream istream(instructions_filename);
  if (!istream.is_open()) {
    LOG_ERROR("Could not open '" + instructions_filename + "'. Aborting!");
    std::exit(1);
  }

  std::string line;
  // read header line of csv file
  std::getline(istream, line);
  if (line != "byte_representation;assembly_code;category;extension;isa_set") {
    LOG_ERROR("Mismatch in header of instruction file. Aborting!");
    std::exit(1);
  }

  uint64_t instruction_idx = 0;
  while (std::getline(istream, line)) {
    std::vector<std::string> line_splitted = SplitString(line, ';');
    if (line_splitted.size() != 5) {
      LOG_ERROR("Mismatch of line format in instruction file. Aborting!");
      std::abort();
    }
    x86Instruction instruction{
        GenerateInstructionUID(instruction_idx),
        base64_decode(line_splitted[0]),
        line_splitted[1],
        line_splitted[2],
        line_splitted[3],
        line_splitted[4]
    };
    instruction_list_.push_back(instruction);
    instruction_idx++;
  }
}

uint64_t CodeGenerator::GenerateInstructionUID(size_t instruction_idx) {
  // the filehash defaults to empty, hence we need to generate the corresponding instruction file
  // hash before calling this function
  assert(!instruction_file_sha256hash_.empty());
  assert(instruction_idx <= 65535);  // idx < 2**16

  // uid = the last 4 hex chars (last 2 bytes of the instruction file hash) + instruction_idx
  std::string end_of_hash = instruction_file_sha256hash_.substr(
      instruction_file_sha256hash_.size() - 4,
      instruction_file_sha256hash_.size());
  uint64_t end_of_hash_integer = std::stoi(end_of_hash, nullptr, 16);
  uint64_t instruction_uid = (end_of_hash_integer << 16) + instruction_idx;
  return instruction_uid;
}

size_t CodeGenerator::InstructionUIDToInstructionIndex(uint64_t instruction_uid) {
  // check whether the correct instruction file is used (last 2 byte of hash are encoded in UID)
  std::stringstream instructionfile_end_of_hash_uid;
  instructionfile_end_of_hash_uid << std::setw(4) << std::setfill('0') << std::hex
                                  << ((instruction_uid >> 16) & 0xffff);
  std::string instructionfile_end_of_hash_loaded = instruction_file_sha256hash_.substr(
      instruction_file_sha256hash_.size() - 4,
      instruction_file_sha256hash_.size());
  if (instructionfile_end_of_hash_uid.str() != instructionfile_end_of_hash_loaded) {
    LOG_ERROR("UID was not generated using this instruction file. "
              "Maybe the instruction file has changed in the meantime. Aborting!");
    LOG_ERROR("expected UID beginning: " + instructionfile_end_of_hash_loaded);
    LOG_ERROR("got UID beginning: " + instructionfile_end_of_hash_uid.str());
    std::exit(1);
  }

  // lowest 2 byte encode the instruction index (see CodeGenerator::GenerateInstructionUID)
  return instruction_uid & 0xffff;
}

int CodeGenerator::GenerateRandomNumber(int min, int max) {
  std::uniform_int_distribution<int> distribution(min, max);
  return distribution(rand_generator_);
}

x86Instruction CodeGenerator::CreateInstructionFromIndex(uint64_t instruction_idx) {
  if (instruction_idx > instruction_list_.size()) {
    LOG_ERROR("Invalid instruction index");
    std::abort();
  }
  return instruction_list_[instruction_idx];
}

x86Instruction CodeGenerator::CreateInstructionFromUID(uint64_t instruction_uid) {
  size_t instruction_idx = InstructionUIDToInstructionIndex(instruction_uid);
  if (instruction_idx > instruction_list_.size()) {
    LOG_ERROR("Invalid instruction index");
    std::abort();
  }
  return instruction_list_[instruction_idx];
}

x86Instruction CodeGenerator::CreateRandomInstruction() {
  size_t idx = GenerateRandomNumber(0, instruction_list_.size() - 1);
  LOG_DEBUG("Got random instruction on index " + std::to_string(idx));
  return CreateInstructionFromIndex(idx);
}

size_t CodeGenerator::GetNumberOfInstructions() {
  return instruction_list_.size();
}

}  // namespace osiris
