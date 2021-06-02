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


#include <getopt.h>

#include <cassert>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <random>

#include "code_generator.h"
#include "core.h"
#include "executor.h"
#include "utils.h"
#include "filter.h"
#include "logger.h"

//
// Constants
//
const std::string kInstructionFile("../x86-instructions/instructions.b64");
const std::string kInstructionFileCleaned("../x86-instructions/instructions_cleaned.b64");

const std::string kOutputCSVNoAssumptions("./measure_trigger_pairs.csv");

const std::string kOutputCSVTriggerEqualsMeasurement("./triggerpairs.csv");
const std::string kOutputFolderTriggerEqualsMeasurement("./triggerpairs");
const std::string kOutputFolderFormattedTriggerEqualsMeasurement("./triggerpairs-formatted");

void ConfirmResultsOfFuzzer(const std::string& input_file, const std::string& output_file) {
  // parse input csv with following format:
  //  measurement-sequence;measurement-category;measurement-extension;
  //  measurement-isa-set;measurement-bytes;
  //  trigger-sequence;trigger-category;trigger-extension;
  //  trigger-isa-set;trigger-bytes;
  //  reset-sequence;reset-category;reset-extension;reset-isa-set;reset-bytes
  std::string input_headerline("timing;"
                               "measurement-uid;measurement-sequence;measurement-category;"
                               "measurement-extension;measurement-isa-set;"
                               "trigger-uid;trigger-sequence;trigger-category;"
                               "trigger-extension;trigger-isa-set;"
                               "reset-uid;reset-sequence;reset-category;reset-extension;"
                               "reset-isa-set");
  std::ifstream input_stream(input_file);
  std::ofstream output_stream(output_file);
  std::ofstream output_cleaned_stream(
      output_file.substr(0, output_file.find_last_of('.')) + "_cleaned.csv");
  std::string line;
  std::getline(input_stream, line);
  if (line != input_headerline) {
    LOG_ERROR("Mismatch in file header. Aborting!");
    LOG_DEBUG("got: " + line);
    LOG_DEBUG("expected: " + input_headerline);
    std::exit(0);
  }

  // write output headerline
  output_stream << input_headerline << std::endl;
  output_cleaned_stream << input_headerline << std::endl;

  osiris::Executor executor;
  osiris::CodeGenerator code_generator(kInstructionFileCleaned);
  int succeeded = 0;
  int failed = 0;
  std::vector<std::tuple<uint64_t, uint64_t, uint64_t, std::string>> inputs;
  while (std::getline(input_stream, line)) {
    std::vector<std::string> line_splitted = osiris::SplitString(line, ';');
    assert(line_splitted.size() == 16);

    uint64_t measurement_uid = stoll(line_splitted[1], nullptr, 16);
    uint64_t trigger_uid = stoll(line_splitted[6], nullptr, 16);
    uint64_t reset_uid = stoll(line_splitted[11], nullptr, 16);
    inputs.emplace_back(measurement_uid, trigger_uid, reset_uid, line);
  }

  // randomize order
  std::shuffle(inputs.begin(), inputs.end(), std::mt19937(std::random_device()()));

  for (const auto& elem : inputs) {
    uint64_t measurement_uid = std::get<0>(elem);
    uint64_t trigger_uid = std::get<1>(elem);
    uint64_t reset_uid = std::get<2>(elem);
    line = std::get<3>(elem);
    osiris::x86Instruction measurement = code_generator.CreateInstructionFromUID(measurement_uid);
    osiris::x86Instruction trigger = code_generator.CreateInstructionFromUID(trigger_uid);
    osiris::x86Instruction reset = code_generator.CreateInstructionFromUID(reset_uid);

    if (trigger.assembly_code == "busy-sleep" || measurement.assembly_code == "busy-sleep") {
      // the sleep is only a valid reset sequence
      continue;
    }
    if (trigger.assembly_code == "sleep-syscall" || measurement.assembly_code == "sleep-syscall") {
      // the sleep is only a valid reset sequence
      continue;
    }
    if (trigger.assembly_code == "short-busy-sleep" ||
        measurement.assembly_code == "short-busy-sleep") {
      // the sleep is only a valid reset sequence
      continue;
    }
    int64_t result;
    int reset_amount = reset.assembly_code == "busy-sleep" ||
        reset.assembly_code == "sleep-syscall" ||
        reset.assembly_code == "short-busy-sleep" ? 1 : 100;
    executor.TestTriggerSequence(trigger.byte_representation,
                                 measurement.byte_representation,
                                 reset.byte_representation,
                                 true, 200,
                                 reset_amount, &result);
    LOG_DEBUG(measurement.assembly_code + ": " + std::to_string(result));

    // write result to file
    std::string line_without_timing = line.substr(line.find(';') + 1);
    std::string output_line = std::to_string(result) + ";" + line_without_timing;
    output_stream << output_line << std::endl;

    if (std::abs(result) > 50) {
      succeeded++;
      output_cleaned_stream << output_line << std::endl;
    } else {
      failed++;
    }
  }

  LOG_INFO("succeeded: " + std::to_string(succeeded) + " failed: " + std::to_string(failed));
}

void PrintHelp(char** argv) {
  std::cout << "USAGE: " << argv[0]
            << " [OPTION] [confirmation input file] [confirmation output file]" << std::endl
            << "Without any option the tool searches with trigger sequence == measurement sequence"
            << std::endl
            << "The following options can influence or change the behavior:" << std::endl
            << "--cleanup \t Create new instruction file "
            << "consisting of only non-faulting instructions" << std::endl
            << "--all \t\t Search with trigger sequence != measurement sequence (takes a few days)"
            << std::endl
            << "--speculation \t Executes trigger sequence only transiently" << std::endl
            << "--filter \t Apply filters to the output of the search" << std::endl
            << "--confirm-results \t Randomize order of the sequence triples and test again. "
            << std::endl
            << " \t\t Requires 2 positional arguments for the input and output file" << std::endl
            << "--help/-h \t Print usage" << std::endl;
}

struct CommandLineArguments {
  bool cleanup = false;
  bool all = false;
  bool speculation_trigger = false;

  bool filter = false;
  std::string filename_filter;

  bool confirm = false;
  std::string filename_confirm_input;
  std::string filename_confirm_output;
};

CommandLineArguments ParseArguments(int argc, char** argv) {
  CommandLineArguments command_line_arguments;
  const struct option long_options[] = {
      {"cleanup", no_argument, nullptr, 'c'},
      {"all", no_argument, nullptr, 'a'},
      {"speculation", no_argument, nullptr, 's'},
      {"filter", required_argument, nullptr, 'f'},
      {"confirm", no_argument, nullptr, '1'},
      {nullptr, 0, nullptr, 0}
  };

  int option_index;
  int c;
  while ((c = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
    switch (c) {
      case '0':
        break;
      case 'c':
        command_line_arguments.cleanup = true;
        break;
      case 'a':
        command_line_arguments.all = true;
        break;
      case 's':
        command_line_arguments.speculation_trigger = true;
        break;
      case '1':
        command_line_arguments.confirm = true;
        break;
      case 'f':
        command_line_arguments.filter = true;
        command_line_arguments.filename_filter = std::string(optarg);
        break;
      case 'h':
      case '?':
      case ':':
        PrintHelp(argv);
        exit(0);
      default:
        std::cerr << "[-] Argument parsing failed. Aborting!" << std::endl;
        exit(1);
    }
  }
  if (command_line_arguments.confirm) {
    if (argv[optind] == nullptr || argv[optind + 1] == nullptr) {
      std::cerr << "[-] Missing positional parameter for --confirm" << std::endl
                << "[-] Argument parsing failed. Aborting!" << std::endl;
      exit(1);
    }
    command_line_arguments.filename_confirm_input = std::string(argv[optind]);
    command_line_arguments.filename_confirm_output = std::string(argv[optind + 1]);
    printf("got confirm with %s and %s\n", command_line_arguments.filename_confirm_input.c_str(),
           command_line_arguments.filename_confirm_output.c_str());
  }
  return command_line_arguments;
}

int main(int argc, char* argv[]) {
  if (DEBUGMODE) {
    LOG_WARNING("Started in DEBUGMODE");
  }
  osiris::SetLogLevel(osiris::DEBUG);
  CommandLineArguments command_line_arguments = ParseArguments(argc, argv);

  //
  // CONFIRM RESULTS
  //
  if (command_line_arguments.confirm) {
    LOG_INFO(" === Starting Confirmation Stage ===");
    assert(!command_line_arguments.filename_confirm_input.empty());
    assert(!command_line_arguments.filename_confirm_output.empty());
    std::string input_file = command_line_arguments.filename_confirm_input;
    std::string output_file = command_line_arguments.filename_confirm_output;
    ConfirmResultsOfFuzzer(input_file, output_file);
    std::exit(0);
  }


  //
  // FILTER
  //
  if (command_line_arguments.filter) {
    LOG_INFO(" === Starting Filter Stage ===");
    assert(!command_line_arguments.filename_filter.empty());
    std::string input_file = command_line_arguments.filename_filter;

    // cut off file ending
    std::string base_name;
    size_t fileending_position = input_file.find_last_of('.');
    if (fileending_position != std::string::npos) {
      base_name = input_file.substr(0, fileending_position);
    } else {
      base_name = input_file;
    }

    osiris::ResultFilter result_filter;

    // filter stage 1 (unique properties)
    std::string output_file1(base_name + "_nocache.csv");
    LOG_INFO("Filtering content of " + input_file + " to " + output_file1);
    result_filter.EnableFilter(osiris::ResultFilterFunctions::REMOVE_ALL_CACHE_SEQUENCES);
    result_filter.ApplyFiltersOnFile(input_file, output_file1);

    // filter stage 2 (remove cache)
    result_filter.ClearAllFilters();
    std::string output_file2(base_name + "_nocache_filtered_by_all.csv");
    LOG_INFO("Filtering content of " + output_file1 + " to " + output_file2);
    result_filter.EnableFilter(osiris::ResultFilterFunctions::UNIQUE_PROPERTY_TUPLES);
    result_filter.ApplyFiltersOnFile(output_file1, output_file2);

    // filter stage 3 (unique measurement trigger extension pairs)
    result_filter.ClearAllFilters();
    std::string output_file3(base_name + "_nocache_filtered_by_all_mt_extensionpair.csv");
    LOG_INFO("Filtering content of " + output_file2 + " to " + output_file3);
    result_filter.EnableFilter(osiris::ResultFilterFunctions::MEASUREMENT_TRIGGER_EXTENSION_PAIRS);
    result_filter.ApplyFiltersOnFile(output_file2, output_file3);
    std::exit(0);
  }

  //
  // CLEANUP THE INSTRUCTION SET
  //
  if (command_line_arguments.cleanup) {
    LOG_INFO(" === Starting Cleanup Stage ===");
    osiris::Core osiris_core(kInstructionFile);
    osiris_core.OutputNonFaultingInstructions(kInstructionFileCleaned);
    osiris_core.PrintFaultStatistics();
    exit(0);
  }

  //
  // FUZZING RUNS
  //
  osiris::Core osiris_core(kInstructionFileCleaned);
  LOG_INFO(" === Starting Main Fuzzing Stage ===");
  if (command_line_arguments.speculation_trigger) {
    LOG_INFO("Searching with transiently executed trigger sequence");
  } else {
    LOG_INFO("Searching with architecturally executed trigger sequence");
  }

  if (command_line_arguments.all) {
    LOG_INFO("Searching with trigger sequence != measurement sequence");
    LOG_INFO("This search is expected to take a few days!");
    osiris_core.FindAndOutputTriggerpairsWithoutAssumptions(
        kOutputCSVNoAssumptions,
        command_line_arguments.speculation_trigger,
        50);
  } else {
    LOG_INFO("Searching with trigger sequence == measurement sequence");
    osiris_core.FindAndOutputTriggerpairsWithTriggerEqualsMeasurement(
        kOutputFolderTriggerEqualsMeasurement,
        kOutputCSVTriggerEqualsMeasurement,
        command_line_arguments.speculation_trigger,
        -50,
        50);
    osiris_core.FormatTriggerPairOutput(kOutputFolderTriggerEqualsMeasurement,
                                        kOutputFolderFormattedTriggerEqualsMeasurement);
  }

  osiris_core.PrintFaultStatistics();
}
