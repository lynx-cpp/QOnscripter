project(onscripter)

set(SRCS
    onscripter.cpp
    DirectReader.cpp
    SarReader.cpp
    NsaReader.cpp
    DirPaths.cpp
    ScriptHandler.cpp
    ScriptParser.cpp
    ScriptParser_command.cpp
    ONScripterLabel.cpp
    ONScripterLabel_command.cpp
    ONScripterLabel_text.cpp
    ONScripterLabel_effect.cpp
    ONScripterLabel_effect_breakup.cpp
    ONScripterLabel_effect_cascade.cpp
    ONScripterLabel_effect_trig.cpp
    ONScripterLabel_event.cpp
    ONScripterLabel_rmenu.cpp
    ONScripterLabel_animation.cpp
    ONScripterLabel_sound.cpp
    ONScripterLabel_file.cpp
    ONScripterLabel_file2.cpp
    ONScripterLabel_image.cpp
    AnimationInfo.cpp
    FontInfo.cpp
    DirtyRect.cpp
    graphics_routines.cpp
    resize_image.cpp
    sjis2utf16.cpp
    graphics_mmx.cpp
    graphics_sse2.cpp
    resources.cpp
    Layer.cpp
)

set(HDRS
    BaseReader.h
    DirectReader.h
    SarReader.h
    NsaReader.h
    DirPaths.h
    ScriptHandler.h
    ScriptParser.h
    ONScripterLabel.h
    AnimationInfo.h
    FontInfo.h
    DirtyRect.h
    resize_image.h
    graphics_altivec.h
    graphics_blend.h
    graphics_common.h
    graphics_cpu.h
    graphics_resize.h
    graphics_sum.h
    graphics_mmx.h
    graphics_sse2.h
    resources.h
    Layer.h
    version.h
)

#find_package(Qt4 REQUIRED)
#set(CMAKE_AUTOMOC TRUE)
#include(${QT_USE_FILE})
#add_definitions(${QT_DEFINITIONS})

find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(SDL_ttf REQUIRED)
find_package(BZip2 REQUIRED)

find_library(SMPEG_LIBRARY NAMES smpeg SMPEG)

include_directories(
    ${INCLUDE_DIRECTORIES}
    ${SDL_INCLUDE_DIR}
    ${BZIP_INCLUDE_DIR}
    /usr/include/smpeg/
)

link_libraries(
    SDLmain
    ${SDL_LIBRARY}
    ${SDLIMAGE_LIBRARY}
    ${SDLMIXER_LIBRARY}
    ${SDLTTF_LIBRARY}
    ${SMPEG_LIBRARY}
    ${BZIP2_LIBRARIES}
#    ${QT_LIBRARIES}
)

add_executable(onscripter-fork ${SRCS}) # ${HDRS_MOC})

set(CMAKE_BUILD_TYPE Debug)
