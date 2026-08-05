cmake_minimum_required(VERSION 3.24)
project(NexusClient VERSION 1.6.18 LANGUAGES CXX)

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
    src/operatorbackendinterface.h
    src/firebaseauthclient.h
    src/firebaseauthclient.cpp
    src/autoupdater.h
    src/autoupdater.cpp
    src/operatorcatalog.h
    src/operatorcatalog.cpp
    src/operatorloadoutcatalog.h
    src/operatorloadoutcatalog.cpp
    src/nexuswidgets.h
    src/nexuswidgets.cpp
    src/nexusprogressview.h
    src/nexusprogressview.cpp
    src/gamelaunchoverlaywindow.h
    src/gamelaunchoverlaywindow.cpp
    src/siegeprocesswatcher.h
    src/siegeprocesswatcher.cpp
    src/launchreadinesscontroller.h
    src/launchreadinesscontroller.cpp
    src/authpages.h
    src/authpages.cpp
    src/pathselectionpage.h
    src/pathselectionpage.cpp
    src/pages.h
    src/pages.cpp
    src/moreoptionspage.h
    src/moreoptionspage.cpp
    src/sensitivityfovconverterpage.h
    src/sensitivityfovconverterpage.cpp
    src/updateprogresspage.h
    src/updateprogresspage.cpp
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

add_executable(license_probe
    src/license_probe.cpp
    src/nexus_license/src/DeviceId.cpp
    src/nexus_license/src/LicenseClient.cpp
)
target_include_directories(license_probe PRIVATE src/nexus_license/include)
target_compile_features(license_probe PRIVATE cxx_std_20)
target_compile_definitions(license_probe PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)
target_link_libraries(license_probe PRIVATE winhttp bcrypt advapi32)

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
