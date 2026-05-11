add_executable(test_jsonprotocol
    test_jsonprotocol.cpp
    ../src/communication/JsonProtocol.cpp
)
target_link_libraries(test_jsonprotocol PRIVATE Qt5::Core Qt5::Test)
target_include_directories(test_jsonprotocol PRIVATE ../src/communication)
add_test(NAME jsonprotocol COMMAND test_jsonprotocol)

add_executable(test_devicestate
    test_devicestate.cpp
    ../src/model/DeviceState.cpp
)
target_link_libraries(test_devicestate PRIVATE Qt5::Core Qt5::Test)
target_include_directories(test_devicestate PRIVATE ../src/model)
add_test(NAME devicestate COMMAND test_devicestate)
