#include "filter.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <cassert>

#include "utils.h"
#include "logger.h"

namespace osiris {

ResultLineData::ResultLineData(const std::string& line) {
  std::vector<std::string> line_splitted = SplitString(line, ';');
  if (line_splitted.size() != 16) {
    LOG_ERROR("Error Parsing csv result file due to invalid line format. Aborting!");
    std::exit(1);
  }

  timing = std::stoi(line_splitted[0]);
  measurement_sequence = std::move(line_splitted[2]);
  measurement_category = std::move(line_splitted[3]);
  measurement_extension = std::move(line_splitted[4]);
  measurement_isa_set = std::move(line_splitted[5]);

  trigger_sequence = std::move(line_splitted[7]);
  trigger_category = std::move(line_splitted[8]);
  trigger_extension = std::move(line_splitted[9]);
  trigger_isa_set = std::move(line_splitted[10]);

  reset_sequence = std::move(line_splitted[12]);
  reset_category = std::move(line_splitted[13]);
  reset_extension = std::move(line_splitted[14]);
  reset_isa_set = std::move(line_splitted[15]);
}

// ====================================================================================
// pre-/filter definitions
// Things to keep in mind when building new filters:
//  - use the prefix "PrefilterFunction" or "FilterFunction" in the function name
//  - PrefilterFunctions and Filterfunctions get two arguments
//    of type int64_t (line number) and const std::string& (current csv line)
//  - you can use ResultLineData for parsing the input format
//  - PrefilterFunctions return void
//  - FilterFunctions return bool (true iff the value should be filtered out)
// ====================================================================================

bool ResultFilter::FilterFunctionIncreaseThreshold300([[maybe_unused]] int64_t line_no,
                                                      const ResultLineData& result_line_data) {
  bool filter_out = -300 < result_line_data.timing && result_line_data.timing < 300;
  return filter_out;
}

void ResultFilter::PrefilterFunctionUniquePropertyTuples(int64_t line_no,
                                                         const ResultLineData& result_line_data) {
  std::string property_tuple = result_line_data.measurement_category
      + result_line_data.measurement_extension
      + result_line_data.measurement_isa_set
      + result_line_data.trigger_category
      + result_line_data.trigger_extension
      + result_line_data.trigger_isa_set
      + result_line_data.reset_category
      + result_line_data.reset_extension
      + result_line_data.reset_isa_set;

  if (best_property_tuples_seen_.find(property_tuple) != best_property_tuples_seen_.end()) {
    int current_best_timing = best_property_tuples_seen_[property_tuple].second;

    // check whether element is the new best and update map if this is the case
    if (std::abs(result_line_data.timing) > std::abs(current_best_timing)) {
      best_property_tuples_seen_[property_tuple] =
          std::pair<int64_t, int>(line_no, result_line_data.timing);
    }
  } else {
    // element is not in the map yet hence add it
    best_property_tuples_seen_[property_tuple] =
        std::pair<int64_t, int>(line_no, result_line_data.timing);
  }
}

bool ResultFilter::FilterFunctionUniquePropertyTuples([[maybe_unused]] int64_t line_no,
                                                      const ResultLineData& result_line_data) {
  std::string property_tuple = result_line_data.measurement_category
      + result_line_data.measurement_extension
      + result_line_data.measurement_isa_set
      + result_line_data.trigger_category
      + result_line_data.trigger_extension
      + result_line_data.trigger_isa_set
      + result_line_data.reset_category
      + result_line_data.reset_extension
      + result_line_data.reset_isa_set;
  auto[best_line_no, best_timing] = best_property_tuples_seen_[property_tuple];
  if (line_no == best_line_no) {
    assert(result_line_data.timing == best_timing);
    return false;
  }
  // filter out everything that is not the best for its tuple
  return true;
}

void ResultFilter::PrefilterFunctionMeasurementTriggerExtensionPairs(int64_t line_no,
                                                                     const ResultLineData&
                                                                     result_line_data) {
  std::string extension_pair = result_line_data.measurement_extension +
      result_line_data.trigger_extension;

  if (best_measure_trigger_extensionpair_seen_.find(extension_pair)
      != best_measure_trigger_extensionpair_seen_.end()) {
    int current_best_timing = best_measure_trigger_extensionpair_seen_[extension_pair].second;

    // check whether element is the new best and update map if this is the case
    if (std::abs(result_line_data.timing) > std::abs(current_best_timing)) {
      best_measure_trigger_extensionpair_seen_[extension_pair] =
          std::pair<int64_t, int>(line_no, result_line_data.timing);
    }
  } else {
    // element is not in the map yet hence add it
    best_measure_trigger_extensionpair_seen_[extension_pair] =
        std::pair<int64_t, int>(line_no, result_line_data.timing);
  }
}
bool ResultFilter::FilterFunctionMeasurementTriggerExtensionPairs(int64_t line_no,
                                                                  const ResultLineData&
                                                                  result_line_data) {
  std::string extension_pair = result_line_data.measurement_extension +
      result_line_data.trigger_extension;
  auto[best_line_no, best_timing] = best_measure_trigger_extensionpair_seen_[extension_pair];
  if (line_no == best_line_no) {
    assert(result_line_data.timing == best_timing);
    return false;
  }
  return true;
}

bool ResultFilter::FilterFunctionRemoveCacheResetSequence([[maybe_unused]] int64_t line_no,
                                                          const ResultLineData& result_line_data) {
  if (result_line_data.reset_sequence.find("CLFLUSH") != std::string::npos) {
    return true;
  }
  if (result_line_data.reset_sequence.find("MOV") != std::string::npos &&
      result_line_data.reset_sequence.find("NT") != std::string::npos) {
    return true;
  }
  if (result_line_data.reset_sequence.find("MASKMOV") != std::string::npos) {
    return true;
  }
  return false;
}

bool ResultFilter::FilterFunctionRemoveAllCacheSequences([[maybe_unused]] int64_t line_no,
                                                         const ResultLineData& result_line_data) {

  if (result_line_data.measurement_sequence.find("CLFLUSH") != std::string::npos) {
    return true;
  }
  if (result_line_data.measurement_sequence.find("MOV") != std::string::npos &&
      result_line_data.measurement_sequence.find("NT") != std::string::npos) {
    return true;
  }
  if (result_line_data.measurement_sequence.find("MASKMOV") != std::string::npos) {
    return true;
  }

  if (result_line_data.trigger_sequence.find("CLFLUSH") != std::string::npos) {
    return true;
  }
  if (result_line_data.trigger_sequence.find("MOV") != std::string::npos &&
      result_line_data.trigger_sequence.find("NT") != std::string::npos) {
    return true;
  }
  if (result_line_data.trigger_sequence.find("MASKMOV") != std::string::npos) {
    return true;
  }

  if (result_line_data.reset_sequence.find("CLFLUSH") != std::string::npos) {
    return true;
  }
  if (result_line_data.reset_sequence.find("MOV") != std::string::npos &&
      result_line_data.reset_sequence.find("NT") != std::string::npos) {
    return true;
  }
  if (result_line_data.reset_sequence.find("MASKMOV") != std::string::npos) {
    return true;
  }
  return false;
}


// ====================================================================================
// End pre-/filter definitions
// ====================================================================================


void ResultFilter::ExecutePrefilterFunction([[maybe_unused]] int64_t line_no,
                                            [[maybe_unused]] const ResultLineData& result_line_data,
                                            ResultFilterFunctions filter) {
  // call prefilter functions (not every filter implementation has one)
  switch (filter) {
    case ResultFilterFunctions::UNIQUE_PROPERTY_TUPLES:
      return PrefilterFunctionUniquePropertyTuples(line_no,
                                                   result_line_data);
    case ResultFilterFunctions::MEASUREMENT_TRIGGER_EXTENSION_PAIRS:
      return PrefilterFunctionMeasurementTriggerExtensionPairs(line_no,
                                                               result_line_data);
    case ResultFilterFunctions::REMOVE_ALL_CACHE_SEQUENCES:
    case ResultFilterFunctions::REMOVE_CACHE_RESET_SEQUENCE:
    case ResultFilterFunctions::INCREASE_THRESHOLD_TO_300:
      // no prefilter needed
      return;
  }
  LOG_ERROR("Tried to use unimplemented prefilter function. Aborting!");
  std::exit(1);
}

bool ResultFilter::ExecuteFilterFunction([[maybe_unused]] int64_t line_no,
                                         [[maybe_unused]] const ResultLineData& result_line_data,
                                         ResultFilterFunctions filter) {
  // call filter functions (new filter functions must be added here; and optional
  // in ExecutePrefilterFunction)
  switch (filter) {
    case ResultFilterFunctions::UNIQUE_PROPERTY_TUPLES:
      return FilterFunctionUniquePropertyTuples(line_no,
                                                result_line_data);

    case ResultFilterFunctions::MEASUREMENT_TRIGGER_EXTENSION_PAIRS:
      return FilterFunctionMeasurementTriggerExtensionPairs(line_no,
                                                            result_line_data);

    case ResultFilterFunctions::REMOVE_CACHE_RESET_SEQUENCE:
      return FilterFunctionRemoveCacheResetSequence(line_no,
                                                    result_line_data);

    case ResultFilterFunctions::REMOVE_ALL_CACHE_SEQUENCES:
      return FilterFunctionRemoveAllCacheSequences(line_no,
                                                   result_line_data);

    case ResultFilterFunctions::INCREASE_THRESHOLD_TO_300:
      return FilterFunctionIncreaseThreshold300(line_no,
                                                result_line_data);
  }
  LOG_ERROR("Tried to use unimplemented filter function. Aborting!");
  std::exit(1);
}

void ResultFilter::EnableFilter(ResultFilterFunctions filter) {
  // check if filter is already enabled
  if (std::find(active_filters_.begin(),
                active_filters_.end(),
                filter) == active_filters_.end()) {
    active_filters_.push_back(filter);
  }
}

void ResultFilter::DisableFilter(ResultFilterFunctions filter) {
  // check if filter is already disabled
  auto filter_pos = std::find(active_filters_.begin(), active_filters_.end(), filter);
  if (filter_pos != active_filters_.end()) {
    active_filters_.erase(filter_pos);
  }
}

void ResultFilter::ClearAllFilters() {
  active_filters_.clear();
}

void ResultFilter::EnableFilters(const std::vector<ResultFilterFunctions>& filters) {
  for (const auto& filter : filters) {
    EnableFilter(filter);
  }
}

void ResultFilter::DisableFilters(const std::vector<ResultFilterFunctions>& filters) {
  for (const auto& filter : filters) {
    DisableFilter(filter);
  }
}

void ResultFilter::ApplyFiltersOnFile(const std::string& input_filename,
                                      const std::string& output_filename) {
  std::ifstream unfiltered_input_stream(input_filename);
  if (!unfiltered_input_stream.is_open()) {
    LOG_ERROR("Could not open " + input_filename + " for filtering");
    exit(1);
  }

  std::ofstream filtered_result_stream(output_filename);
  if (!filtered_result_stream.is_open()) {
    LOG_ERROR("Could not open " + output_filename + " for filtering");
    exit(1);
  }

  // read and validate csv header
  std::string line;
  std::getline(unfiltered_input_stream, line);
  const std::string headerline = "timing;"
                                 "measurement-uid;measurement-sequence;measurement-category;"
                                 "measurement-extension;measurement-isa-set;"
                                 "trigger-uid;trigger-sequence;trigger-category;"
                                 "trigger-extension;trigger-isa-set;"
                                 "reset-uid;reset-sequence;reset-category;"
                                 "reset-extension;reset-isa-set";
  if (line != headerline) {
    LOG_ERROR("Mismatch in csv header line. Aborting!");
    std::exit(1);
  }

  // read input file the first time to let prefilters build up their data structures
  int line_no = 0;
  while (std::getline(unfiltered_input_stream, line)) {
    const ResultLineData result_line_data(line);
    for (const auto& filter : active_filters_) {
      ExecutePrefilterFunction(line_no, result_line_data, filter);
    }
    line_no++;
  }

  // start reading from the beginning again
  unfiltered_input_stream.clear();
  unfiltered_input_stream.seekg(0, std::ios::beg);
  line_no = 0;

  // write headerline of filtered file
  filtered_result_stream << headerline << std::endl;

  // read input file the second time and filter out data
  // read away headerline
  std::getline(unfiltered_input_stream, line);
  while (std::getline(unfiltered_input_stream, line)) {
    bool filter_out = false;
    const ResultLineData result_line_data(line);
    for (const auto& filter : active_filters_) {
      filter_out = filter_out || ExecuteFilterFunction(line_no, result_line_data, filter);
    }

    if (!filter_out) {
      // line should not be filtered, hence write to new file
      filtered_result_stream << line << std::endl;
    }
    line_no++;
  }
}

}  // namespace osiris
