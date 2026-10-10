# Sources and include directories of the Hoops LVGL GUI process. Generated
# font C files live under assets/ and are committed.
file(GLOB_RECURSE GUI_APP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/gui/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/assets/fonts/*.c
)

set(GUI_APP_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/gui/include
    ${CMAKE_CURRENT_LIST_DIR}/assets
)
