cmake_minimum_required(VERSION 3.24)
project(NexusClient VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

qt_add_executable(NexusClient WIN32
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/theme.h
    src/authsession.h
    src/firebaseauthclient.h
    src/firebaseauthclient.cpp
    src/operatorcatalog.h
    src/operatorcatalog.cpp
    src/nexuswidgets.h
    src/nexuswidgets.cpp
    src/authpages.h
    src/authpages.cpp
    src/pathselectionpage.h
    src/pathselectionpage.cpp
    src/pages.h
    src/pages.cpp
    src/sensitivityfovconverterpage.h
    src/sensitivityfovconverterpage.cpp
    src/authenticatedroot.h
    src/authenticatedroot.cpp
    src/nexus_license/include/DeviceId.h
    src/nexus_license/include/LicenseClient.h
    src/nexus_license/include/LicenseManager.h
    src/nexus_license/include/LicenseTypes.h
    src/nexus_license/include/SecureLicenseStore.h
    src/nexus_license/src/DeviceId.cpp
    src/nexus_license/src/LicenseClient.cpp
    src/nexus_license/src/LicenseManager.cpp
    src/nexus_license/src/SecureLicenseStore.cpp
    resources/resources.qrc
)

target_include_directories(NexusClient PRIVATE src/nexus_license/include)
target_link_libraries(NexusClient PRIVATE Qt6::Widgets Qt6::Network)
target_link_libraries(NexusClient PRIVATE winhttp bcrypt crypt32 advapi32 shell32)

target_compile_definitions(NexusClient PRIVATE
    QT_NO_KEYWORDS
    NEXUS_APP_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
    target_compile_options(NexusClient PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(NexusClient PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(NexusClient PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS NexusClient
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
