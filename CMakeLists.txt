cmake_minimum_required(VERSION 3.9)

project(Celeste LANGUAGES CXX)
include(CTest)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
	find_package(Qt5 COMPONENTS Widgets Network Multimedia MultimediaWidgets Concurrent WinExtras REQUIRED)
	find_package(Qt5 COMPONENTS Test REQUIRED)
else()
	find_package(Qt5 COMPONENTS Widgets Network Multimedia MultimediaWidgets Concurrent TextToSpeech REQUIRED)
	find_package(Qt5 COMPONENTS Test REQUIRED)
endif()

if(BUILD_TESTING)
	add_executable(Gauntlet
		globals.h
		settings.h
		channel.h
		channel.cpp
		command.h
		volume.h
		relay.h
		widgets.h
		widgets.cpp
		recognizers.h
		recognizers.cpp
		receivers.h
		receivers.cpp
		subscribers.h
		subscribers.cpp
		panes.h
		panes.cpp
		platform.h
		platform.cpp
		window.h
		window.cpp
		testing.cpp
	)
	add_test(NAME Gauntlet COMMAND Gauntlet ${CMAKE_SOURCE_DIR}/exercises)

	if (WIN32)
		target_link_libraries(Gauntlet PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Concurrent Qt5::Test Qt5::WinExtras)
	else()
		target_link_libraries(Gauntlet PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Concurrent Qt5::Test Qt5::TextToSpeech)
	endif()
endif()

add_executable(Celeste
	globals.h
	settings.h
	channel.h
	channel.cpp
	command.h
	emote.h
	volume.h
	relay.h
	widgets.h
	widgets.cpp
	receivers.h
	receivers.cpp
	recognizers.h
	recognizers.cpp
	subscribers.h
	subscribers.cpp
	panes.h
	panes.cpp
	platform.h
	platform.cpp
	window.h
	window.cpp
	main.cpp
	resources/resources.qrc
)

if (WIN32)
	set(CMAKE_CXX_FLAGS_RELEASE "-O2")
	target_link_libraries(Celeste PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Concurrent Qt5::WinExtras)
	target_link_libraries(Gauntlet PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Concurrent Qt5::Test Qt5::WinExtras)
else()
	include(CheckIPOSupported)
	check_ipo_supported(RESULT result)
	if(result)
	  set_property(TARGET Celeste PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	endif()
	set(CMAKE_CXX_FLAGS_RELEASE "-O2 -pipe")
	target_link_libraries(Celeste PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Concurrent Qt5::TextToSpeech)
	target_link_libraries(Gauntlet PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::MultimediaWidgets Qt5::Concurrent Qt5::Test Qt5::TextToSpeech)
endif()
