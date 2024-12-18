cmake_minimum_required(VERSION 3.14)
project(fp)
set(CMAKE_CXX_STANDARD 17)
set(SOURCE_FILES
        ArcballCamera.cpp
        ArcballCamera.h
        Vehicle.cpp
        Vehicle.h
        FPEngine.cpp
        FPEngine.h
        main.cpp
        FPCamera.cpp
        FPCamera.h
        Marble.cpp
        Marble.h
        Coin.cpp
        Coin.h
        TPCamera.cpp
        TPCamera.h
)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Windows with MinGW Installations
if( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" AND MINGW )
    # if working on Windows but not in the lab
    # update the include directory location
    include_directories("Z:/CSCI441/include")
    # update the lib directory location
    target_link_directories(${PROJECT_NAME} PUBLIC "Z:/CSCI441/lib")
    target_link_libraries(${PROJECT_NAME} opengl32 glfw3 glad gdi32)
    # OS X Installations
elseif( APPLE AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
    # update the include directory location
    include_directories("/usr/local/include")
    # update the lib directory location
    target_link_directories(${PROJECT_NAME} PUBLIC "/usr/local/lib")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo" glfw3 glad)
    # Blanket *nix Installations
elseif( UNIX AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
    # update the include directory location
    # update the include directory location
    include_directories("/home/gray/mines/computer_graphics/dependencies")
    # update the lib directory location
    target_link_directories(${PROJECT_NAME} PUBLIC "/home/gray/mines/computer_graphics/dependencies")
    target_link_libraries(${PROJECT_NAME} GL glfw "/home/gray/mines/computer_graphics/dependencies/glad/libglad.a")
    target_link_libraries(${PROJECT_NAME} GL glfw)
endif()