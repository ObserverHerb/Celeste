cmake_minimum_required(VERSION 3.5)

project(Celeste LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

if (WIN32)
	find_package(Qt5 COMPONENTS Widgets Network Multimedia MultimediaWidgets WinExtras REQUIRED)
else()
	find_package(Qt5 COMPONENTS Widgets Network Multimedia MultimediaWidgets TextToSpeech Concurrent REQUIRED)
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
)

if (WIN32)
	target_link_libraries(Celeste PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::WinExtras)
else()
	target_link_libraries(Celeste PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::TextToSpeech Qt5::Concurrent)
	target_link_libraries(Celeste PRIVATE tbb)
endif()
