cmake_minimum_required(VERSION 3.16)
project(chatgpt-krunner)

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.0.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Network
    Gui
    Widgets
)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    Runner
    I18n
    CoreAddons
    Config
)

add_library(krunner_chatgpt MODULE
    chatgptrunner.cpp
)

target_link_libraries(krunner_chatgpt
    Qt6::Core
    Qt6::Network
    Qt6::Gui
    Qt6::Widgets
    KF6::Runner
    KF6::I18n
    KF6::CoreAddons
)

install(TARGETS krunner_chatgpt DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/krunner)
install(FILES chatgptrunner.json DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/krunner)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)