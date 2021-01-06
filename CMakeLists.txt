cmake_minimum_required(VERSION 3.9)

project(Celeste LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -pipe")

if (WIN32)
	find_package(Qt5 COMPONENTS Widgets Network Multimedia MultimediaWidgets Concurrent WinExtras REQUIRED)
else()
	find_package(Qt5 COMPONENTS Widgets Network Multimedia MultimediaWidgets Concurrent TextToSpeech REQUIRED)
endif()

add_executable(Celeste
	globals.h
	settings.h
	command.h
	volume.h
	relay.h
	viewers.h
	widgets.h
	widgets.cpp
	receivers.h
	receivers.cpp
	panes.h
	panes.cpp
	window.h
	window.cpp
	main.cpp
	resources/resources.qrc
)

include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result)
  set_property(TARGET Celeste PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (WIN32)
	target_link_libraries(Celeste PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Concurrent Qt5::WinExtras)
else()
	target_link_libraries(Celeste PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Concurrent Qt5::TextToSpeech)
endif()
