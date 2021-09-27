cmake_minimum_required(VERSION 3.10)
project(stbtt)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)


add_executable(stbtt
        macros.h
        cff.h
        cff.cpp
        buf.h
        buf.cpp
        letter.h
        letter.cpp
        font.h
        font.cpp
        # TrueTypeTest.cpp
        RasterizerTest.cpp
        )


target_link_libraries(stbtt -lm)