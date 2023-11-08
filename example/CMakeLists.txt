cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wdouble-promotion
    -Wformat=2
    -Wformat-security
    -Wformat-signedness
    -Wnull-dereference
    -Wtrivial-auto-var-init
    -Wunused-parameter
    -Wunused-const-variable=2
    -Wuninitialized
    -Wmaybe-uninitialized
    -Wstringop-overflow=4
    -Wconversion
    -Wfloat-conversion
    -Wsign-conversion
    -Warith-conversion
    -Wbool-compare
    -Wduplicated-branches
    -Wduplicated-cond
    -Wfloat-equal
    -Wshadow
    -Wundef
    -Wunused-macros
    -Wcast-qual
    -Wcast-align=strict
    -Wlogical-op
    -Wmissing-declarations
    -Wredundant-decls
    # -Winline
    -Wlong-long
    -Woverloaded-virtual
    -Wimplicit-fallthrough=5
    -Wmissing-include-dirs
    -Wsuggest-override
    -Wnon-virtual-dtor
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined -g")

set(SRC_FILES message_queue.cpp)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

add_executable(example ${SRC_FILES})
target_link_libraries(example PUBLIC libmsg_queue)

if( supported )
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET example PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()

