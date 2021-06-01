#! /bin/bash

##########################################################
BUILD_FOLDER="./build"
##########################################################

# default values for arguments
all=false
speculation=false
filter_cache=false

# parse and verify first argument (isolated core)
if [ -z "$1" ]
then
  echo "[-] Missing first argument representing CPU core. Aborting."
  exit 1
else
  CPU_NO=$1
fi
number_re='^[0-9]+$'
if ! [[ $CPU_NO =~ $number_re ]] ; then
   echo "[-] First argument must be a number representing a CPU core. Aborting." >&2; exit 1
fi

# parse second argument
if [ "$2" == "--all" ]
then
  all=true
elif [ "$2" == "--speculation" ]
then
  speculation=true
elif [ "$2" == "--filter-cache" ]
then
  filter_cache=true
fi

# parse third argument
if [ "$3" == "--all" ]
then
  all=true
elif [ "$3" == "--speculation" ]
then
  speculation=true
elif [ "$3" == "--filter-cache" ]
then
  filter_cache=true
fi

# parse fourth argument
if [ "$4" == "--all" ]
then
  all=true
elif [ "$4" == "--speculation" ]
then
  speculation=true
elif [ "$4" == "--filter-cache" ]
then
  filter_cache=true
fi


# save src directory
pushd . 
# switch to src directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR

if [ "$speculation" = true ]
then
  echo "[+] Trigger-sequence will only be executed transiently."
  speculation_argument="--speculation"
else
  echo "[+] Trigger-sequence will be executed architecturally."
  speculation_argument=""
fi

cd $BUILD_FOLDER
# cleanup instruction file
echo "[+] Removing faulting instructions from the instruction set..."
./osiris --cleanup

if [ "$all" = true ]
then
  echo "[+] Starting fuzzer without assumptions on CPU core ${CPU_NO}..."
  taskset -c $CPU_NO ./osiris --all $speculation_argument
  output_base_filename="measure_trigger_pairs_spectrigger"
else
  echo "[+] Starting fuzzer with trigger==measurement assumption on CPU core ${CPU_NO}..."
  taskset -c $CPU_NO ./osiris $speculation_argument
  output_base_filename="triggerpairs"
fi

if [ "$filter_cache" = true ]
then
  echo ""
  # filter out cache-related side channels
  ./osiris --filter ./${output_base_filename}.csv
  taskset -c $CPU_NO ./osiris --confirm ./${output_base_filename}_nocache.csv ./${output_base_filename}_confirmed_iter1.csv
else
  echo ""
  # do not filter out cache-related side channels
  taskset -c $CPU_NO ./osiris --confirm ./${output_base_filename}.csv ./${output_base_filename}_confirmed_iter1.csv
fi
taskset -c $CPU_NO ./osiris --confirm ./${output_base_filename}_confirmed_iter1_cleaned.csv ./${output_base_filename}_confirmed_iter2.csv

# apply filters on final result
./osiris --filter ./${output_base_filename}_confirmed_iter2_cleaned.csv

# print results before and after filtering
result_no_before_confirmation=`cat ${output_base_filename}.csv | wc -l`
result_no_after_confirmation=`cat ${output_base_filename}_confirmed_iter2_cleaned.csv | wc -l`
# substract headerlines
result_no_before_confirmation=$(($result_no_before_confirmation - 1))
result_no_after_confirmation=$(($result_no_after_confirmation - 1))

if [ "$filter_cache" = true ]
then
  echo "[+] Filtered out cache"
else
  echo "[+] Did not filter out cache"
fi
echo "[+] Number of results before filtering: $result_no_before_confirmation"
echo "[+] Number of results after filtering: $result_no_after_confirmation"

# restore directory of caller
popd
