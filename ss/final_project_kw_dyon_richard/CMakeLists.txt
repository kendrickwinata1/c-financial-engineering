cmake_minimum_required(VERSION 3.28.1)
project(final_project_kw_dyon)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all .cpp and .h files in main directory
file(GLOB OOP_SOURCES
        "*.cpp"
        "*.h"
)

# Add the executable and link all source files
add_executable(main Main Main.cpp ${OOP_SOURCES})

# --- Add this for .txt files ---
## Assuming your .txt files are in the same directory as CMakeLists.txt
#file(GLOB TXT_FILES "*.txt")
#foreach(txt_file ${TXT_FILES})
#    # Copy each text file to the build directory (CMAKE_CURRENT_BINARY_DIR)
#    # The @ONLY is often used for files that don't need variable substitution
#    # but configure_file is also often used for simple copying.
#    configure_file(${txt_file} ${CMAKE_CURRENT_BINARY_DIR}/${txt_file} COPYONLY)
#endforeach()

# Alternatively, if you just want to copy files without any variable substitution:
# file(COPY ${TXT_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})