#!/bin/sh

export LIBMYSOFA_PATH=`pwd`"/submodules/libmysofa"
export LIBMYSOFA_BUILD_PATH="$LIBMYSOFA_PATH/build"
export LIBMYSOFA_INSTALL_PATH="$LIBMYSOFA_PATH/out"
export LIBSPATIALAUDIO_PATH=`pwd`"/submodules/libspatialaudio"
export LIBSPATIALAUDIO_BUILD_PATH="$LIBSPATIALAUDIO_PATH/build"
export LIBSPATIALAUDIO_INSTALL_PATH="$LIBSPATIALAUDIO_PATH/install"

# libmysofa

cd $LIBMYSOFA_PATH
mkdir $LIBMYSOFA_BUILD_PATH
mkdir $LIBMYSOFA_INSTALL_PATH

# configure mysofa
cmake -S . -B build -G "Xcode" -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.5 \
    -DCMAKE_INSTALL_PREFIX="$LIBMYSOFA_INSTALL_PATH" \
    -DBUILD_SHARED_LIBS=0 \
    -DBUILD_TESTS=0

# build and install mysofa
cmake --build build --config Release --target install

# libspatialaudio

cd $LIBSPATIALAUDIO_PATH
mkdir $LIBSPATIALAUDIO_BUILD_PATH
mkdir $LIBSPATIALAUDIO_INSTALL_PATH

# add zlib to CMakeLists.txt
perl -0777 -i.bak -pe 's{if\(MYSOFA_FOUND\).*?endif\(MYSOFA_FOUND\)}{if(MYSOFA_FOUND)
    message("Found mysofa!")
    find_package(ZLIB REQUIRED)
    set(MYSOFA_LIB "-L\${MYSOFA_LIBRARY_DIRS} -lmysofa")
    set(MYSOFA_INCLUDE "-I\${MYSOFA_INCLUDE_DIRS}")
    target_include_directories(spatialaudio
        PUBLIC \${MYSOFA_INCLUDE_DIRS} \${ZLIB_INCLUDE_DIR})
    target_link_libraries(spatialaudio \${MYSOFA_LIBRARIES} ZLIB::ZLIB)
endif(MYSOFA_FOUND)}s' CMakeLists.txt

# configure spatialaudio
cmake -S . -B build -G "Xcode" -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.5 \
    -DCMAKE_INSTALL_PREFIX="$LIBSPATIALAUDIO_INSTALL_PATH" \
    -DBUILD_SHARED_LIBS=0 -DMYSOFA_LIBRARY_DIRS="$LIBMYSOFA_INSTALL_PATH" \
    -DMYSOFA_LIBRARIES="$LIBMYSOFA_INSTALL_PATH/lib/libmysofa.a" \
    -DMYSOFA_INCLUDE_DIRS="$LIBMYSOFA_INSTALL_PATH/include"

# build and install spatialaudio
cmake --build build --config Release --target install

# reset libspatialaudio CMakeLists.txt file
mv CMakeLists.txt.bak CMakeLists.txt
