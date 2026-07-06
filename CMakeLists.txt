cmake_minimum_required(VERSION 3.24)
# Single source of truth for the plugin version; the release workflow
# verifies the git tag matches this before publishing.
project(Nimbostratus VERSION 1.3.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(dpf)

set(SP ${CMAKE_CURRENT_SOURCE_DIR}/superparasites)
set(SPEEX ${CMAKE_CURRENT_SOURCE_DIR}/speexdsp)

dpf_add_plugin(nimbostratus
  TARGETS vst3 clap
  UI_TYPE opengl
  FILES_UI
    plugin/CloudsUI.cpp
    dpf-widgets/opengl/DearImGui.cpp
  FILES_DSP
    plugin/CloudsPlugin.cpp
    ${SP}/supercell/dsp/correlator.cc
    ${SP}/supercell/dsp/granular_processor.cc
    ${SP}/supercell/dsp/mu_law.cc
    ${SP}/supercell/dsp/kammerl_player.cc
    ${SP}/supercell/dsp/pvoc/frame_transformation.cc
    ${SP}/supercell/dsp/pvoc/phase_vocoder.cc
    ${SP}/supercell/dsp/pvoc/spectral_clouds_transformation.cc
    ${SP}/supercell/dsp/pvoc/stft.cc
    ${SP}/supercell/resources.cc
    ${SP}/stmlib/utils/random.cc
    ${SP}/stmlib/dsp/atan.cc
    ${SP}/stmlib/dsp/units.cc
    ${SPEEX}/libspeexdsp/resample.c
)

target_include_directories(nimbostratus PUBLIC
  plugin
  ${SP}
  ${SPEEX}/include
  ${SPEEX}/include/speex
  ${SPEEX}/libspeexdsp
  dpf-widgets/opengl
)

# TEST keeps stmlib/clouds off the STM32-specific code paths (same define the
# firmware's own desktop test harness uses). -Wno-narrowing: the firmware
# brace-initializes bools from 0.0f, an error on modern GCC.
target_compile_definitions(nimbostratus PUBLIC TEST
  NIMBO_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  NIMBO_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  NIMBO_VERSION_PATCH=${PROJECT_VERSION_PATCH})
target_compile_options(nimbostratus PUBLIC -Wno-narrowing)

set_source_files_properties(${SPEEX}/libspeexdsp/resample.c PROPERTIES
  COMPILE_DEFINITIONS "OUTSIDE_SPEEX;FLOATING_POINT;EXPORT=;RANDOM_PREFIX=cloudsvst")

if(MINGW)
  # Self-contained binaries: no libstdc++/libgcc/winpthread DLL dependencies.
  foreach(tgt nimbostratus-vst3 nimbostratus-clap)
    if(TARGET ${tgt})
      target_link_options(${tgt} PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
  endforeach()
endif()
