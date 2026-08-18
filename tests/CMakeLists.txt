find_package(Catch2 CONFIG REQUIRED)

add_executable(DirectorDeskTests
    Core/ResultTests.cpp
    Core/LogTests.cpp
    Platform/PathTests.cpp
)

target_link_libraries(DirectorDeskTests
    PRIVATE
        dd_core
        dd_platform
        Catch2::Catch2WithMain
)

dd_set_strict_warnings(DirectorDeskTests)

add_test(NAME DirectorDeskTests COMMAND DirectorDeskTests)
