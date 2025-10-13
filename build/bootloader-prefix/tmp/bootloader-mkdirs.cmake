# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/jhona/esp/v5.5.1/esp-idf/components/bootloader/subproject")
  file(MAKE_DIRECTORY "C:/Users/jhona/esp/v5.5.1/esp-idf/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "C:/Users/jhona/OneDrive/Escritorio/STR_20252/LEDRGB_PWM/build/bootloader"
  "C:/Users/jhona/OneDrive/Escritorio/STR_20252/LEDRGB_PWM/build/bootloader-prefix"
  "C:/Users/jhona/OneDrive/Escritorio/STR_20252/LEDRGB_PWM/build/bootloader-prefix/tmp"
  "C:/Users/jhona/OneDrive/Escritorio/STR_20252/LEDRGB_PWM/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/jhona/OneDrive/Escritorio/STR_20252/LEDRGB_PWM/build/bootloader-prefix/src"
  "C:/Users/jhona/OneDrive/Escritorio/STR_20252/LEDRGB_PWM/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/jhona/OneDrive/Escritorio/STR_20252/LEDRGB_PWM/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/jhona/OneDrive/Escritorio/STR_20252/LEDRGB_PWM/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
