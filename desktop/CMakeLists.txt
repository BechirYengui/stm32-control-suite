cmake_minimum_required(VERSION 3.16)
project(STM32Interface VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ============================================================================
# OPTIONS DE CONFIGURATION
# ============================================================================
option(BUILD_WITH_QML "Build with QML interface support" OFF)  # ← CHANGÉ: OFF par défaut
option(BUILD_WITH_CHARTS "Build with Qt Charts support" OFF)  # ← CHANGÉ: OFF par défaut
option(BUILD_TESTS "Build test suite" OFF)
option(BUILD_DOCS "Build documentation" OFF)

# ============================================================================
# RECHERCHE DES PACKAGES Qt
# ============================================================================
set(QT_COMPONENTS Core Widgets SerialPort)

if(BUILD_WITH_QML)
    list(APPEND QT_COMPONENTS Qml Quick)
endif()

if(BUILD_WITH_CHARTS)
    list(APPEND QT_COMPONENTS Charts)
endif()

find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})

# ============================================================================
# SOURCES DU PROJET
# ============================================================================
set(PROJECT_SOURCES
    # Main
    src/main.cpp
    
    # Model (MVC)
    src/model/DeviceState.h
    src/model/DeviceState.cpp
    src/model/DataModel.h
    src/model/DataModel.cpp
    
    # View (MVC) - Qt Widgets
    src/view/MainWindow.h
    src/view/MainWindow.cpp
    src/view/MainWindow.ui
    
    # Controller (MVC)
    src/controller/DeviceController.h
    src/controller/DeviceController.cpp
    
    # Communication Layer
    src/communication/SerialManager.h
    src/communication/SerialManager.cpp
    src/communication/SerialWorker.h
    src/communication/SerialWorker.cpp
    src/communication/JsonProtocol.h
    src/communication/JsonProtocol.cpp
)

# ============================================================================
# RESSOURCES (OPTIONNEL - QML)
# ============================================================================
if(BUILD_WITH_QML)
    # Vérifie que le fichier existe avant de l'ajouter
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/qml/resources.qrc")
        qt5_add_resources(QML_RESOURCES qml/resources.qrc)
        list(APPEND PROJECT_SOURCES ${QML_RESOURCES})
    else()
        message(WARNING "qml/resources.qrc not found, skipping QML resources")
    endif()
endif()

# ============================================================================
# CRÉATION DE L'EXÉCUTABLE
# ============================================================================
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# ============================================================================
# LINK DES BIBLIOTHÈQUES Qt
# ============================================================================
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt5::Core 
    Qt5::Widgets 
    Qt5::SerialPort
)

if(BUILD_WITH_QML)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        Qt5::Qml
        Qt5::Quick
    )
endif()

if(BUILD_WITH_CHARTS)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Charts)
endif()

# ============================================================================
# INCLUSION DES RÉPERTOIRES
# ============================================================================
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controller
    ${CMAKE_CURRENT_SOURCE_DIR}/src/communication
)

# ============================================================================
# DÉFINITIONS DE COMPILATION
# ============================================================================
target_compile_definitions(${PROJECT_NAME} PRIVATE
    QT_DEPRECATED_WARNINGS
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

if(BUILD_WITH_QML)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_WITH_QML)
endif()

if(BUILD_WITH_CHARTS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_WITH_CHARTS)
endif()

# ============================================================================
# OPTIONS DE COMPILATION
# ============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror=return-type
    )
endif()

# ============================================================================
# INSTALLATION
# ============================================================================
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Installation des ressources
if(BUILD_WITH_QML)
    install(DIRECTORY qml/
        DESTINATION share/${PROJECT_NAME}/qml
        FILES_MATCHING PATTERN "*.qml"
    )
endif()

# ============================================================================
# TESTS (OPTIONNEL)
# ============================================================================
if(BUILD_TESTS)
    enable_testing()
    find_package(Qt5 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

# ============================================================================
# DOCUMENTATION (OPTIONNEL)
# ============================================================================
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# ============================================================================
# INFORMATIONS DE CONFIGURATION
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  ${PROJECT_NAME} Configuration")
message(STATUS "========================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Qt Components:")
message(STATUS "  Qt5 Core: ${Qt5Core_VERSION}")
message(STATUS "  Qt5 Widgets: ${Qt5Widgets_VERSION}")
message(STATUS "  Qt5 SerialPort: ${Qt5SerialPort_VERSION}")
if(BUILD_WITH_QML)
    message(STATUS "  Qt5 Qml: ${Qt5Qml_VERSION}")
    message(STATUS "  Qt5 Quick: ${Qt5Quick_VERSION}")
endif()
if(BUILD_WITH_CHARTS)
    message(STATUS "  Qt5 Charts: ${Qt5Charts_VERSION}")
endif()
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  BUILD_WITH_QML: ${BUILD_WITH_QML}")
message(STATUS "  BUILD_WITH_CHARTS: ${BUILD_WITH_CHARTS}")
message(STATUS "  BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "  BUILD_DOCS: ${BUILD_DOCS}")
message(STATUS "========================================")
message(STATUS "")

# ============================================================================
# CPACK - PACKAGING
# ============================================================================
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "IMT Atlantique")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY 
    "Interface de pilotage et supervision STM32")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

include(CPack)
