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


#include "core.h"

#include <capstone/capstone.h>  // disassembling the output for proper formatting

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <fstream>

#include "code_generator.h"
#include "logger.h"

namespace osiris {

Core::Core(const std::string& instructions_filename) :
    code_generator_(CodeGenerator(instructions_filename)),
    executor_(Executor()) {
  iterations_no_ = 10;
  reset_executions_amount_without_assumptions_ = 1;
  reset_executions_amount_trigger_equals_measurement_ = 50;
}

void Core::FindAndOutputTriggerpairsWithoutAssumptions(const std::string& output_csvfilename,
                                                       bool execute_trigger_only_in_speculation,
                                                       int64_t threshold_in_cycles) {
  std::ofstream output_csvfile(output_csvfilename);
  if (output_csvfile.fail()) {
    LOG_ERROR("Couldn't not open " + output_csvfilename + " for writing. Aborting!");
  }
  std::string headerline("timing;"
                         "measurement-uid;measurement-sequence;measurement-category;"
                         "measurement-extension;measurement-isa-set;"
                         "trigger-uid;trigger-sequence;trigger-category;trigger-extension;"
                         "trigger-isa-set;"
                         "reset-uid;reset-sequence;reset-category;reset-extension;"
                         "reset-isa-set");
  output_csvfile << headerline << std::endl;

  size_t max_instruction_no = code_generator_.GetNumberOfInstructions();
  for (size_t measurement_idx = 0; measurement_idx < max_instruction_no; measurement_idx++) {
    x86Instruction measurement_sequence =
        code_generator_.CreateInstructionFromIndex(measurement_idx);
    LOG_INFO("processing measurement " + std::to_string(measurement_idx) + "/"
                 + std::to_string(max_instruction_no - 1));

    for (size_t trigger_idx = 0; trigger_idx < max_instruction_no; trigger_idx++) {
      x86Instruction trigger_sequence = code_generator_.CreateInstructionFromIndex(trigger_idx);
      if (trigger_sequence.assembly_code == "busy-sleep" ||
          trigger_sequence.assembly_code == "short-busy-sleep" ||
          trigger_sequence.assembly_code == "sleep-syscall") {
        // the sleeps are only valid reset sequences
        continue;
      }
      for (size_t reset_idx = 0; reset_idx < max_instruction_no; reset_idx++) {
        x86Instruction reset_sequence = code_generator_.CreateInstructionFromIndex(reset_idx);
        // execute sleeps only 1 time
        int reset_executions_amount =
            reset_sequence.assembly_code == "busy-sleep" ||
                reset_sequence.assembly_code == "sleep-syscall" ||
                reset_sequence.assembly_code == "short-busy-sleep"
            ? 1 : reset_executions_amount_without_assumptions_;
        int64_t result;
        int error = executor_.TestTriggerSequence(trigger_sequence.byte_representation,
                                                  measurement_sequence.byte_representation,
                                                  reset_sequence.byte_representation,
                                                  execute_trigger_only_in_speculation,
                                                  iterations_no_,
                                                  reset_executions_amount,
                                                  &result);
        if (error == 0 && (result < -threshold_in_cycles || result > threshold_in_cycles)) {
          // this removes the "reset-sequence is not really working"-problem
          // by checking that the reset we observe is indeed triggered by this reset sequence
          int64_t reset_test_result;
          error = executor_.TestResetSequence(trigger_sequence.byte_representation,
                                              measurement_sequence.byte_representation,
                                              reset_sequence.byte_representation,
                                              iterations_no_,
                                              reset_executions_amount,
                                              &reset_test_result);
          if (error == 0 && -20 < reset_test_result && reset_test_result < 20) {
            // write csv line
            std::string csv_line = std::to_string(result);
            csv_line += ";";
            csv_line += measurement_sequence.GetCSVRepresentation();
            csv_line += ";";
            csv_line += trigger_sequence.GetCSVRepresentation();
            csv_line += ";";
            csv_line += reset_sequence.GetCSVRepresentation();
            output_csvfile << csv_line << std::endl;
          }
        }
      }
    }
  }
}

void Core::FindAndOutputTriggerpairsWithTriggerEqualsMeasurement(
    const std::string& output_folder,
    const std::string&
    output_csvfilename,
    bool execute_trigger_only_in_speculation,
    int64_t negative_threshold,
    int64_t positive_threshold) {
  // remove and recreate output directory to delete all old content
  std::filesystem::remove_all(output_folder);
  std::filesystem::create_directory(output_folder);

  std::ofstream output_csvfile(output_csvfilename);
  std::string headerline("timing;"
                         "measurement-uid;measurement-sequence;measurement-category;"
                         "measurement-extension;measurement-isa-set;"
                         "trigger-uid;trigger-sequence;trigger-category;trigger-extension;"
                         "trigger-isa-set;"
                         "reset-uid;reset-sequence;reset-category;reset-extension;"
                         "reset-isa-set");
  output_csvfile << headerline << std::endl;
  size_t max_instruction_no = code_generator_.GetNumberOfInstructions();
  for (size_t trigger_idx = 0; trigger_idx < max_instruction_no; trigger_idx++) {
    x86Instruction trigger_sequence = code_generator_.CreateInstructionFromIndex(trigger_idx);
    std::stringstream output_stream;
    LOG_INFO("processing trigger " + std::to_string(trigger_idx) +
        " (" + trigger_sequence.assembly_code + ")");
    if (trigger_sequence.assembly_code == "busy-sleep" ||
        trigger_sequence.assembly_code == "short-busy-sleep" ||
        trigger_sequence.assembly_code == "sleep-syscall") {
      // the sleeps are only valid reset sequences
      continue;
    }
    for (size_t reset_idx = 0; reset_idx < max_instruction_no; reset_idx++) {
      x86Instruction reset_sequence = code_generator_.CreateInstructionFromIndex(reset_idx);
      // execute sleeps only 1 time
      int reset_executions_amount =
          reset_sequence.assembly_code == "busy-sleep" ||
              reset_sequence.assembly_code == "sleep-syscall" ||
              reset_sequence.assembly_code == "short-busy-sleep"
          ? 1 : reset_executions_amount_trigger_equals_measurement_;
      int64_t result;
      // we assume that trigger sequence equals measurement sequence
      int error = executor_.TestTriggerSequence(trigger_sequence.byte_representation,
                                                trigger_sequence.byte_representation,
                                                reset_sequence.byte_representation,
                                                execute_trigger_only_in_speculation,
                                                iterations_no_,
                                                reset_executions_amount,
                                                &result);
      if (error == 0 && (result < negative_threshold || result > positive_threshold)) {
        // this removes the "reset-sequence is not really working"-problem
        // by checking that the reset we observe is indeed triggered by this reset sequence
        int64_t reset_test_result;
        error = executor_.TestResetSequence(trigger_sequence.byte_representation,
                                            trigger_sequence.byte_representation,
                                            reset_sequence.byte_representation,
                                            iterations_no_, reset_executions_amount,
                                            &reset_test_result);
        if (error == 0 && -20 < reset_test_result && reset_test_result < 20) {
          output_stream << base64_encode(reset_sequence.byte_representation)
                        << ";" << result << std::endl;

          // write csv line
          std::string csv_line = std::to_string(result);
          csv_line += ";";
          csv_line += trigger_sequence.GetCSVRepresentation();  // measurement sequence
          csv_line += ";";
          csv_line += trigger_sequence.GetCSVRepresentation();
          csv_line += ";";
          csv_line += reset_sequence.GetCSVRepresentation();
          output_csvfile << csv_line << std::endl;
        }
      }
    }

    if (output_stream.rdbuf()->in_avail() > 0) {
      // write all results for the trigger sequence to a file
      std::string output_path_instructionfile(output_folder + "/" +
          base64_encode(trigger_sequence.byte_representation));
      std::ofstream output_instructionfile(output_path_instructionfile);
      output_instructionfile << output_stream.rdbuf();
    }
  }
}

void Core::FormatTriggerPairOutput(const std::string& output_folder,
                                   const std::string& output_folder_formatted) {
  // delete and create the folder to remove all old content in there
  std::filesystem::remove_all(output_folder_formatted);
  std::filesystem::create_directory(output_folder_formatted);

  // initialize capstone
  csh capstone_handle;
  if (cs_open(CS_ARCH_X86, CS_MODE_64, &capstone_handle) != CS_ERR_OK) {
    LOG_ERROR("Couldn't initialize Capstone! Aborting!");
    std::exit(1);
  }
  // iterate over all files and format them and store inside the new folder
  size_t unique_idx = 0;
  std::string delimiter(
      "=======================================================================");
  std::string delimiter2(
      "-----------------------------------------------------------------------");
  for (const auto& dir_entry : std::filesystem::recursive_directory_iterator(output_folder)) {
    std::ifstream unformatted_file(dir_entry.path());
    byte_array unformatted_filename_bytes = base64_decode(dir_entry.path().filename());
    std::string unformatted_filename = BytearrayToString(unformatted_filename_bytes);
    cs_insn* disassembled_instructions;
    size_t instruction_count = cs_disasm(capstone_handle,
                                         reinterpret_cast<const uint8_t*>(
                                             unformatted_filename.c_str()),
                                         unformatted_filename.length(),
                                         0x1000, 0, &disassembled_instructions);
    // format of output filename: mnemonic_operands___UniqueID
    std::string formatted_filename;
    if (instruction_count > 0) {
      formatted_filename += disassembled_instructions[0].mnemonic;
      formatted_filename += "_";
      formatted_filename += disassembled_instructions[0].op_str;
    } else {
      // in case we can't disassemble the filename we just leave the original one
      // and add a prefix
      LOG_WARNING("Couldn't disassemble filename " + dir_entry.path().filename().string()
                      + ".");
      formatted_filename += "disasm_err_";
      formatted_filename += dir_entry.path().filename();
    }
    std::replace(formatted_filename.begin(), formatted_filename.end(), ' ', '_');
    formatted_filename += "---";
    formatted_filename += std::to_string(unique_idx++);

    std::string formatted_filepath(output_folder_formatted);
    formatted_filepath += "/";
    formatted_filepath += formatted_filename;

    std::ofstream formatted_file(formatted_filepath);
    // first write out the complete trigger instruction
    formatted_file << delimiter << std::endl
                   << "=================== trigger/measurement instruction ==================="
                   << std::endl
                   << delimiter << std::endl;
    if (instruction_count > 0) {
      for (size_t i = 0; i < instruction_count; i++) {
        formatted_file << disassembled_instructions[i].mnemonic
                       << " "
                       << disassembled_instructions[i].op_str
                       << std::endl;
      }
    } else {
      // in case we can't disassemble the filename we just leave the original one and an error
      // message
      formatted_file << "DISASM ERR(inst:"
                     << dir_entry.path().filename()
                     << ")"
                     << std::endl;
    }
    formatted_file << delimiter << std::endl
                   << "========================== reset instructions ========================="
                   << std::endl
                   << delimiter << std::endl;

    // free capstone disassembly again
    cs_free(disassembled_instructions, instruction_count);

    std::string line;
    while (std::getline(unformatted_file, line)) {
      // line-format: <reset-sequence(b64);<timing-diff>
      std::vector<std::string> line_splitted = SplitString(line, ';');
      std::string instruction_bytestring = BytearrayToString(
          base64_decode(line_splitted[0]));
      instruction_count = cs_disasm(capstone_handle,
                                    reinterpret_cast<const uint8_t*>(
                                        instruction_bytestring.c_str()),
                                    instruction_bytestring.length(),
                                    0x1000, 0, &disassembled_instructions);
      if (instruction_count > 0) {
        for (size_t i = 0; i < instruction_count; i++) {
          formatted_file << disassembled_instructions[i].mnemonic
                         << " "
                         << disassembled_instructions[i].op_str
                         << std::endl;
        }
      } else {
        LOG_WARNING("Couldn't disassemble " + line_splitted[0] + ".");
        // failed to disassemble instruction (could be due to a bug in capstone
        // - see https://github.com/aquynh/capstone/issues/1648)
        formatted_file << "DISASM ERR (inst: "
                       << line_splitted[0]
                       << ")"
                       << std::endl;
      }
      formatted_file << "TIMING: " << line_splitted[1] << std::endl;
      formatted_file << delimiter2 << std::endl;
      // free capstone disassembly again
      cs_free(disassembled_instructions, instruction_count);
    }
  }

  // cleanup capstone
  cs_close(&capstone_handle);
}

void Core::OutputNonFaultingInstructions(const std::string& output_filename) {
  std::vector<size_t> non_faulting_instructions = FindNonFaultingInstructions();
  LOG_INFO("found " + std::to_string(non_faulting_instructions.size())
               + " non faulting instructions");
  std::ofstream output_file(output_filename);

  // write headerline
  std::string headerline("byte_representation;assembly_code;category;extension;isa_set");
  output_file << headerline << std::endl;

  // write non-faulting instructions in original format
  for (size_t instruction_idx : non_faulting_instructions) {
    x86Instruction instruction = code_generator_.CreateInstructionFromIndex(instruction_idx);

    std::string line = base64_encode(instruction.byte_representation);
    line += ";";
    line += instruction.assembly_code;
    line += ";";
    line += instruction.category;
    line += ";";
    line += instruction.extension;
    line += ";";
    line += instruction.isa_set;
    output_file << line << std::endl;
  }
  output_file.close();
  LOG_INFO("Wrote non faulting instructions to the file " + output_filename);
}

std::vector<size_t> Core::FindNonFaultingInstructions() {
  std::vector<size_t> non_faulting_instruction_indexes;
  byte_array empty_sequence;
  for (size_t inst_idx = 0; inst_idx < code_generator_.GetNumberOfInstructions(); inst_idx++) {
    x86Instruction measurement_sequence = code_generator_.CreateInstructionFromIndex(inst_idx);
    int64_t result;
    LOG_INFO("testing instruction " + measurement_sequence.assembly_code);
    int error = executor_.TestTriggerSequence(measurement_sequence.byte_representation,
                                              measurement_sequence.byte_representation,
                                              measurement_sequence.byte_representation,
                                              false,
                                              1, 1, &result);
    if (error == 0) {
      non_faulting_instruction_indexes.push_back(inst_idx);
    }
  }
  return non_faulting_instruction_indexes;
}

void Core::PrintFaultStatistics() {
  Executor::PrintFaultCount();
}

}  // namespace osiris
