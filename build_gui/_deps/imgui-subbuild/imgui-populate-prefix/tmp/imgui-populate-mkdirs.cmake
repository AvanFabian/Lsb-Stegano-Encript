# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "F:/AvanD/KULIAH/Semester Antara 6-7_TI 22/Project Steganography/LSB-Steganography/build_gui/_deps/imgui-src")
  file(MAKE_DIRECTORY "F:/AvanD/KULIAH/Semester Antara 6-7_TI 22/Project Steganography/LSB-Steganography/build_gui/_deps/imgui-src")
endif()
file(MAKE_DIRECTORY
  "F:/AvanD/KULIAH/Semester Antara 6-7_TI 22/Project Steganography/LSB-Steganography/build_gui/_deps/imgui-build"
  "F:/AvanD/KULIAH/Semester Antara 6-7_TI 22/Project Steganography/LSB-Steganography/build_gui/_deps/imgui-subbuild/imgui-populate-prefix"
  "F:/AvanD/KULIAH/Semester Antara 6-7_TI 22/Project Steganography/LSB-Steganography/build_gui/_deps/imgui-subbuild/imgui-populate-prefix/tmp"
  "F:/AvanD/KULIAH/Semester Antara 6-7_TI 22/Project Steganography/LSB-Steganography/build_gui/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp"
  "F:/AvanD/KULIAH/Semester Antara 6-7_TI 22/Project Steganography/LSB-Steganography/build_gui/_deps/imgui-subbuild/imgui-populate-prefix/src"
  "F:/AvanD/KULIAH/Semester Antara 6-7_TI 22/Project Steganography/LSB-Steganography/build_gui/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "F:/AvanD/KULIAH/Semester Antara 6-7_TI 22/Project Steganography/LSB-Steganography/build_gui/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "F:/AvanD/KULIAH/Semester Antara 6-7_TI 22/Project Steganography/LSB-Steganography/build_gui/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
