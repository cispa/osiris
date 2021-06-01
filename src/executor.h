#ifndef OSIRIS_SRC_EXECUTOR_H_
#define OSIRIS_SRC_EXECUTOR_H_

#include <vector>

#include "code_generator.h"

namespace osiris {

constexpr size_t kPagesize = 4096;


///
/// Generates code for testing the effects of sequence triples.
/// The current version only supports x86 architectures
///
class Executor {
 public:
  Executor();
  ~Executor();

  /// run with and without reset sequence and return the timing difference in cycles_difference
  /// \param trigger_sequence trigger sequence to test
  /// \param measurement_sequence  measurement sequence to test
  /// \param reset_sequence reset sequence to test
  /// \param no_testruns number of test iterations
  /// \param reset_executions_amount amount of executions of the reset sequence
  /// \param cycles_difference outputs resulting difference in CPU cycles
  /// \return 0 on success
  int TestResetSequence(const byte_array& trigger_sequence,
                        const byte_array& measurement_sequence,
                        const byte_array& reset_sequence,
                        int no_testruns,
                        int reset_executions_amount,
                        int64_t* cycles_difference);

  /// run with and without reset sequence and return the timing difference in cycles_difference
  /// \param trigger_sequence trigger sequence to test
  /// \param measurement_sequence  measurement sequence to test
  /// \param reset_sequence reset sequence to test
  /// \param execute_trigger_only_in_speculation execute the trigger sequence only transiently
  /// \param no_testruns number of test iterations
  /// \param reset_executions_amount amount of executions of the reset sequence
  /// \param cycles_difference outputs resulting difference in CPU cycles
  /// \return 0 on success
  int TestTriggerSequence(const byte_array& trigger_sequence,
                          const byte_array& measurement_sequence,
                          const byte_array& reset_sequence,
                          bool execute_trigger_only_in_speculation,
                          int no_testruns,
                          int reset_executions_amount,
                          int64_t* cycles_difference);

  /// returns the delta between trigger;reset;measure and reset;trigger;measure
  /// \param trigger_sequence trigger sequence to test
  /// \param measurement_sequence  measurement sequence to test
  /// \param reset_sequence reset sequence to test
  /// \param no_testruns number of test iterations
  /// \param cycles_difference outputs resulting difference in CPU cycles
  /// \return 0 on success
  int TestSequenceTriple(const byte_array& trigger_sequence,
                         const byte_array& measurement_sequence,
                         const byte_array& reset_sequence,
                         int no_testruns,
                         int64_t* cycles_difference);

  /// prints current number of faults per signal
  static void PrintFaultCount();

 private:
  /// Create code which executes the trigger followed by the reset followed
  ///  by a timed measurement sequence
  /// \param codepage_no index of the codepage to use
  /// \param trigger_sequence trigger sequence to test
  /// \param measurement_sequence  measurement sequence to test
  /// \param reset_sequence reset sequence to test
  /// \param reset_executions_amount amount of executions of the reset sequence
  void CreateResetTestrunCode(int codepage_no, const byte_array& trigger_sequence,
                              const byte_array& measurement_sequence,
                              const byte_array& reset_sequence,
                              int reset_executions_amount);

  /// Create code which executes the "first" sequence n-times followed by the "second" sequence
  ///  and followed by a timed measurement sequence
  /// \param codepage_no index of the codepage to use
  /// \param trigger_sequence trigger sequence to test
  /// \param measurement_sequence  measurement sequence to test
  /// \param reset_sequence reset sequence to test
  /// \param first_sequence_executions_amount amount of executions of the first sequence
  void CreateTestrunCode(int codepage_no, const byte_array& first_sequence,
                         const byte_array& second_sequence,
                         const byte_array& measurement_sequence,
                         int first_sequence_executions_amount);

  /// Create code which executes the reset sequence n-times followed by a transient execution
  ///  of the trigger sequence and followed by a timed measurement sequence
  /// \param codepage_no index of the codepage to use
  /// \param trigger_sequence trigger sequence to test
  /// \param measurement_sequence  measurement sequence to test
  /// \param reset_sequence reset sequence to test
  /// \param reset_executions_amount amount of executions of the reset sequence
  void CreateSpeculativeTriggerTestrunCode(int codepage_no,
                                           const byte_array& measurement_sequence,
                                           const byte_array& trigger_sequence,
                                           const byte_array& reset_sequence,
                                           int reset_executions_amount);

  /// Tests the timing difference. Assumes that one of the Create...Code functions
  /// was previously called on the codepage
  /// \param codepage_no codepage to use
  /// \param cycles_elapsed  median cycle difference over all testruns between
  ///                        trigger sequence and no trigger sequence
  /// \return 0 if no failure (e.g. SIGSEGV, SIGILL, SIGFPE) occurred
  int ExecuteTestrun(int codepage_no, uint64_t* cycles_elapsed);

  ///
  /// Clears the data page by overwriting its content with nullbytes
  ///
  void ClearDataPage();

  ///
  /// initializes code page with NOPs and RET at the end
  /// \param codepage_no code page to use
  void InitializeCodePage(int codepage_no);

  /// adds a serializing instruction to the page
  /// \param codepage_no code page to use
  void AddSerializeInstructionToCodePage(int codepage_no);

  /// thrashes registers RDX, RAX, R10
  /// \param codepage_no code page to use
  void AddTimerStartToCodePage(int codepage_no);

  /// thrashes registers RDX, RAX
  /// resulting timing difference will be stored in R11 afterwards
  /// \param codepage_no code page to use
  void AddTimerEndToCodePage(int codepage_no);

  /// thrashes register RAX
  /// \param codepage_no code page to use
  void MakeTimerResultReturnValue(int codepage_no);

  /// Adds prolog to the codepage (i.e. saves callee-saved registers, creates large stackspace
  /// and initializes registers
  /// \param codepage_no code page to use
  void AddProlog(int codepage_no);

  /// Adds Epilog to the codepage (i.e. restores registers and stack)
  /// \param codepage_no code page to use
  void AddEpilog(int codepage_no);

  /// adds instruction bytes to codepage
  /// \param codepage_no code page to use
  /// \param instruction_bytes bytes what should be written
  /// \param instruction_length number of bytes that should be written
  void AddInstructionToCodePage(int codepage_no, const char* instruction_bytes,
                                  size_t instruction_length);

  /// adds instruction bytes to codepage
  /// \param codepage_no code page to use
  /// \param instruction_bytes bytes what should be written
  void AddInstructionToCodePage(int codepage_no, const byte_array& instruction_bytes);

  /// Create a sequence of NOP instructions
  /// \param length length of the nop sled
  /// \return nop sled as byte array
  byte_array CreateSequenceOfNOPs(size_t length);

  template<size_t size>
  static void RegisterFaultHandler(std::array<int, size> signals_to_handle);

  template<size_t size>
  static void UnregisterFaultHandler(std::array<int, size> signals_to_handle);
  // NOTE: FaultHandler and ExecuteCodePage must both be static functions
  //       for the signal handling + jmp logic to work
  static void FaultHandler(int sig);
  static int ExecuteCodePage(void* codepage, uint64_t* cycles_elapsed);

  ///
  /// acts as read/write memory for instructions
  ///
  std::array<void*, 2> execution_data_pages_;

  ///
  /// rwx page where we generate and execute code
  ///
  std::array<char*, 2> execution_code_pages_;

  ///
  ///
  ///
  std::array<size_t, 2> code_pages_last_written_index_;

  ///
  /// results for the trigger testruns (preallocated for performance)
  /// used in TestTriggerSequence
  ///
  std::vector<int64_t> results_trigger;

  ///
  /// results for the testruns without a trigger (preallocated for performance)
  /// used in TestTriggerSequence
  ///
  std::vector<int64_t> results_notrigger;
};

}  // namespace osiris

#endif  //OSIRIS_SRC_EXECUTOR_H_
