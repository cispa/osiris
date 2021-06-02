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


#ifndef OSIRIS_SRC_CORE_H_
#define OSIRIS_SRC_CORE_H_

#include <string>
#include <vector>

#include "code_generator.h"
#include "executor.h"

namespace osiris {

using InstructionIndexSequence = std::vector<size_t>;

/// The key component of Osiris.
/// It lets the CodeGenerator generates new code samples and
/// sends them to the executor
class Core {
 public:
  explicit Core(const std::string& instructions_filename);

  /// Searches for trigger-reset pairs without any assumption
  /// \param output_csvfilename human-readable csv output
  ///     output file format:
  ///     timing;measurement-uid;measurement-sequence;measurement-category;measurement-extension;
  ///     measurement-isa-set;
  ///     trigger-uid;trigger-sequence;trigger-category;trigger-extension;trigger-isa-set;
  ///     reset-uid;reset-sequence;reset-category;reset-extension;reset-isa-set
  /// \param execute_trigger_only_in_speculation toggle to execute trigger sequence only transiently
  /// \param threshold_in_cycles absolute cycle difference for logging a success
  void FindAndOutputTriggerpairsWithoutAssumptions(const std::string& output_csvfilename,
                                                   bool execute_trigger_only_in_speculation,
                                                   int64_t threshold_in_cycles);

  /// Searches for trigger-reset pairs with the assumption that the trigger-sequence is the same
  /// as the measurement-sequence
  /// \param output_folder folder where results are written to (can be formatted by
  ///     FormatTriggerPairOutput
  ///     output file format:
  ///         filename is the base64 encoded version of the trigger/measurement sequence
  ///         each line consists of base64 encoded reset sequence and timing:
  ///         line-format:    <reset-sequence(b64)>;<timing-diff>
  /// \param output_csvfilename human-readable csv output
  ///     output file format:
  ///     timing;measurement-uid;measurement-sequence;measurement-category;measurement-extension;
  ///     measurement-isa-set;
  ///     trigger-uid;trigger-sequence;trigger-category;trigger-extension;trigger-isa-set;
  ///     reset-uid;reset-sequence;reset-category;reset-extension;reset-isa-set
  /// \param execute_trigger_only_in_speculation toggle to execute trigger sequence only transiently
  /// \param negative_threshold cycle difference for logging a success
  /// \param positive_threshold cycle difference for logging a success
  void FindAndOutputTriggerpairsWithTriggerEqualsMeasurement(const std::string& output_folder,
                                                             const std::string& output_csvfilename,
                                                             bool
                                                             execute_trigger_only_in_speculation,
                                                             int64_t negative_threshold,
                                                             int64_t positive_threshold);

  /// Formats output of FindAndOutputTriggerpairsWithTriggerEqualsMeasurement by disassembling all output encodings
  /// \param output_folder output folder of FindAndOutputTriggerpairsWithTriggerEqualsMeasurement
  /// \param output_folder_formatted new folder with formatted output
  void FormatTriggerPairOutput(const std::string& output_folder,
                               const std::string& output_folder_formatted);

  ///
  /// Print fault statistics of the underlying executor
  ///
  void PrintFaultStatistics();

  ///
  /// outputs csv file in the format of the instruction input file consisting only of instructions
  /// which did not result in a fault
  void OutputNonFaultingInstructions(const std::string& output_filename);

 private:
  ///
  /// Tests all instructions and checks which one results in a fault
  /// \return vector of non-faulting instructions indexes
  std::vector<size_t> FindNonFaultingInstructions();

  CodeGenerator code_generator_;
  Executor executor_;
  int iterations_no_;
  int reset_executions_amount_without_assumptions_;
  int reset_executions_amount_trigger_equals_measurement_;
};

}  // namespace osiris

#endif  //OSIRIS_SRC_CORE_H_


