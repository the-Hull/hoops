# The score model (plain C++, no SDK): the service links all of it, the GUI
# links it too for spot names and percentage formatting.
file(GLOB_RECURSE LIBS_SCORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/score/*.cpp
)

file(GLOB_RECURSE LIBS_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/Sources/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Sources/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Source/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Source/*.cpp
)
list(APPEND LIBS_SOURCES ${LIBS_SCORE_SOURCES})

set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Header
    ${CMAKE_CURRENT_LIST_DIR}
)
