cmake_minimum_required(VERSION 3.9)

project(Celeste LANGUAGES CXX)
include(CTest)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
	find_package(Qt5 COMPONENTS Widgets Network Multimedia MultimediaWidgets Concurrent WinExtras REQUIRED)
else()
	find_package(Qt5 COMPONENTS Widgets Network Multimedia MultimediaWidgets Concurrent TextToSpeech REQUIRED)
endif()

add_executable(Celeste
	globals.h
	settings.h
	security.h
	security.cpp
	channel.h
	channel.cpp
	server.h
	server.cpp
	widgets.h
	widgets.cpp
	entities.h
	entities.cpp
	eventsub.h
	eventsub.cpp
	panes.h
	panes.cpp
	platform.cpp
	log.h
	log.cpp
	window.h
	window.cpp
	bot.h
	bot.cpp
	main.cpp
	resources/resources.qrc
)

if (WIN32)
	set(CMAKE_CXX_FLAGS_RELEASE "-O2")
	target_link_libraries(Celeste PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Concurrent Qt5::WinExtras)
else()
	include(CheckIPOSupported)
	check_ipo_supported(RESULT result)
	if(result)
	  set_property(TARGET Celeste PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	endif()
	set(CMAKE_CXX_FLAGS_RELEASE "-O2 -pipe")
	target_link_libraries(Celeste PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Concurrent Qt5::TextToSpeech)
endif()
