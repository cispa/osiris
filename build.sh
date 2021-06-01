#! /bin/sh

# this must match the entry in run.sh
BUILD_FOLDER="./build"

# save src directory
pushd . 
# switch to src directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR


mkdir $BUILD_FOLDER | true
cd $BUILD_FOLDER
cmake ..
make -j 5
cd ..


# restore directory of caller
popd
