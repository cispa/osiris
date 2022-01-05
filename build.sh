#! /bin/sh

# this must match the entry in run.sh
BUILD_FOLDER="./build"

# default architecture
ARCH=INTEL

# parse first arg
if [ -z "$1" ]
then
  echo "[-] Missing first argument representing target architecture [INTEL/AMD]. Aborting."
  exit 1
else
  ARCH=$1
fi

# save src directory
pushd . 
# switch to src directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR


mkdir $BUILD_FOLDER | true
cd $BUILD_FOLDER
cmake .. -DARCH=${ARCH}
make -j 5
cd ..


# restore directory of caller
popd
