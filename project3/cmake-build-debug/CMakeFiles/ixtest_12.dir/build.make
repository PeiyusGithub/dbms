# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.8

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/mlz000/mlz000/clion-2017.2.3/bin/cmake/bin/cmake

# The command to remove a file.
RM = /home/mlz000/mlz000/clion-2017.2.3/bin/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/mlz000/mlz000/DataBase-Management-System/project3

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/mlz000/mlz000/DataBase-Management-System/project3/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/ixtest_12.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ixtest_12.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ixtest_12.dir/flags.make

CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o: CMakeFiles/ixtest_12.dir/flags.make
CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o: ../src/ix/ixtest_12.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mlz000/mlz000/DataBase-Management-System/project3/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o -c /home/mlz000/mlz000/DataBase-Management-System/project3/src/ix/ixtest_12.cc

CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/mlz000/mlz000/DataBase-Management-System/project3/src/ix/ixtest_12.cc > CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.i

CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/mlz000/mlz000/DataBase-Management-System/project3/src/ix/ixtest_12.cc -o CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.s

CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o.requires:

.PHONY : CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o.requires

CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o.provides: CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o.requires
	$(MAKE) -f CMakeFiles/ixtest_12.dir/build.make CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o.provides.build
.PHONY : CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o.provides

CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o.provides.build: CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o


# Object files for target ixtest_12
ixtest_12_OBJECTS = \
"CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o"

# External object files for target ixtest_12
ixtest_12_EXTERNAL_OBJECTS =

ixtest_12: CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o
ixtest_12: CMakeFiles/ixtest_12.dir/build.make
ixtest_12: libLIBRBF.a
ixtest_12: CMakeFiles/ixtest_12.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/mlz000/mlz000/DataBase-Management-System/project3/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ixtest_12"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ixtest_12.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ixtest_12.dir/build: ixtest_12

.PHONY : CMakeFiles/ixtest_12.dir/build

CMakeFiles/ixtest_12.dir/requires: CMakeFiles/ixtest_12.dir/src/ix/ixtest_12.cc.o.requires

.PHONY : CMakeFiles/ixtest_12.dir/requires

CMakeFiles/ixtest_12.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ixtest_12.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ixtest_12.dir/clean

CMakeFiles/ixtest_12.dir/depend:
	cd /home/mlz000/mlz000/DataBase-Management-System/project3/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/mlz000/mlz000/DataBase-Management-System/project3 /home/mlz000/mlz000/DataBase-Management-System/project3 /home/mlz000/mlz000/DataBase-Management-System/project3/cmake-build-debug /home/mlz000/mlz000/DataBase-Management-System/project3/cmake-build-debug /home/mlz000/mlz000/DataBase-Management-System/project3/cmake-build-debug/CMakeFiles/ixtest_12.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ixtest_12.dir/depend

