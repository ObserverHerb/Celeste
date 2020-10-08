cmake_minimum_required(VERSION 3.5)

project(Celeste LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets Network Multimedia MultimediaWidgets TextToSpeech REQUIRED)

add_executable(Celeste
	globals.h
	settings.h
	command.h
	receivers.h
	receivers.cpp
	panes.h
	panes.cpp
	window.cpp
	window.h
	main.cpp
)

target_link_libraries(Celeste PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::TextToSpeech)
