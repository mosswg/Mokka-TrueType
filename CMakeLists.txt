cmake_minimum_required(VERSION 3.10)
project(mka_truetype)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(mka_truetype
        src/macros.h
        src/cff.h
        src/cff.cpp
        src/buf.h
        src/buf.cpp
        src/letter.h
        src/letter.cpp
        src/font.h
        src/font.cpp
        # src/TrueTypeTest.cpp
        src/RasterizerTest.cpp
        )


target_link_libraries(mka_truetype -lm)