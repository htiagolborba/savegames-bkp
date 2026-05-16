cmake_minimum_required(VERSION 3.15)
project(GameSaveBackupTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
set(SOURCES
    src/main.cpp
    src/App.cpp
    src/BackupManager.cpp
    src/GameScanner.cpp
    src/ConfigManager.cpp
    src/HistoryWriter.cpp
    src/Logger.cpp
    src/Utils.cpp
)

# Create WIN32 executable (no console window)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# Include vendor directory for nlohmann/json
target_include_directories(${PROJECT_NAME} PRIVATE vendor)

# Link required Windows libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Comctl32
    Shlwapi
    Shell32
    Ole32
)

# Enable Unicode
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)

# Copy known_games.json to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/known_games.json"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/known_games.json"
    COMMENT "Copying known_games.json to output directory"
)
