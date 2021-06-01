#ifndef OSIRIS_SRC_FILTER_H_
#define OSIRIS_SRC_FILTER_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace osiris {

enum class ResultFilterFunctions {
  INCREASE_THRESHOLD_TO_300,
  REMOVE_CACHE_RESET_SEQUENCE,
  REMOVE_ALL_CACHE_SEQUENCES,
  UNIQUE_PROPERTY_TUPLES,
  MEASUREMENT_TRIGGER_EXTENSION_PAIRS
};

/// struct to hold data that are encoded in one single line of the csv-file
struct ResultLineData {
  int timing;
  std::string measurement_sequence;
  std::string measurement_category;
  std::string measurement_extension;
  std::string measurement_isa_set;

  std::string trigger_sequence;
  std::string trigger_category;
  std::string trigger_extension;
  std::string trigger_isa_set;

  std::string reset_sequence;
  std::string reset_category;
  std::string reset_extension;
  std::string reset_isa_set;

  explicit ResultLineData(const std::string& line);
};

class ResultFilter {
 public:
  ///
  /// Enable single filter
  /// \param filter Enum of the filter to enable
  void EnableFilter(ResultFilterFunctions filter);

  ///
  /// Disable single filter
  /// \param filter Enum of the filter to disable
  void DisableFilter(ResultFilterFunctions filter);

  ///
  /// Enable multiple filters
  /// \param filter vector over filter enums
  void EnableFilters(const std::vector<ResultFilterFunctions>& filters);

  ///
  /// Disable multiple filters
  /// \param filter vector over filter enums
  void DisableFilters(const std::vector<ResultFilterFunctions>& filters);

  ///
  /// Disable all filters
  ///
  void ClearAllFilters();

  ///  Filters csv-outputfile of the fuzzing logic for given filters
  /// \param input_filename output csv-file of the fuzzing logic
  /// \param output_filename
  void ApplyFiltersOnFile(const std::string& input_filename, const std::string& output_filename);

 private:
  // pre-/filter definitions
  // Please use the prefix "PrefilterFunction" or "FilterFunction" in the function name
  //
  bool FilterFunctionIncreaseThreshold300(int64_t line_no,
                                          const ResultLineData& result_line_data);

  void PrefilterFunctionUniquePropertyTuples(int64_t line_no,
                                             const ResultLineData& result_line_data);
  bool FilterFunctionUniquePropertyTuples(int64_t line_no,
                                          const ResultLineData& result_line_data);

  void PrefilterFunctionMeasurementTriggerExtensionPairs(int64_t line_no,
                                             const ResultLineData& result_line_data);
  bool FilterFunctionMeasurementTriggerExtensionPairs(int64_t line_no,
                                          const ResultLineData& result_line_data);

  bool FilterFunctionRemoveCacheResetSequence(int64_t line_no,
                                              const ResultLineData& result_line_data);

  bool FilterFunctionRemoveAllCacheSequences(int64_t line_no,
                                              const ResultLineData& result_line_data);

  void ExecutePrefilterFunction(int64_t line_no,
                                const ResultLineData& result_line_data,
                                ResultFilterFunctions filter);
  bool ExecuteFilterFunction(int64_t line_no,
                             const ResultLineData& result_line_data,
                             ResultFilterFunctions filter);

  //
  // End pre-/filter definitions
  //

  //
  // prefilter-filter shared data structures
  //
  std::unordered_map<std::string, std::pair<int64_t, int>> best_property_tuples_seen_;
  std::unordered_map<std::string, std::pair<int64_t, int>> best_measure_trigger_extensionpair_seen_;
  //
  // end prefilter-filter shared data structures
  //


  std::vector<ResultFilterFunctions> active_prefilters_;
  std::vector<ResultFilterFunctions> active_filters_;
};

}  // namespace osiris

#endif  // OSIRIS_SRC_FILTER_H_
