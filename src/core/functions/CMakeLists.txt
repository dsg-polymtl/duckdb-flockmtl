add_subdirectory(scalar)

set(EXTENSION_SOURCES
    ${EXTENSION_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/prompt_builder.cpp
    PARENT_SCOPE)
