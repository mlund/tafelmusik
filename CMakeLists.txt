cmake_minimum_required(VERSION 2.6)
project("TAFELMUSIK")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

  add_library(tafelmusik tafelmusik.cpp soundcard.c sidengine.c)
  find_library(framework_cocoa "Cocoa")
  find_library(framework_audiounit "AudioUnit")
  TARGET_LINK_LIBRARIES(tafelmusik ${framework_cocoa})
  TARGET_LINK_LIBRARIES(tafelmusik ${framework_audiounit})

  add_executable(sample sample.cpp)
  target_link_libraries(sample tafelmusik)

endif()
