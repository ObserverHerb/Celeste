cmake_minimum_required(VERSION 3.11)

project(Celeste LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
	find_package(Qt6 COMPONENTS Widgets Network Mqtt Multimedia MultimediaWidgets WebSockets REQUIRED)
else()
	find_package(Qt6 COMPONENTS Widgets Network Mqtt Multimedia MultimediaWidgets WebSockets REQUIRED)
endif()

add_executable(Celeste
	globals.h
	settings.h
	security.h
	security.cpp
	channel.h
	channel.cpp
	widgets.h
	widgets.cpp
	entities.h
	entities.cpp
	eventsub.h
	eventsub.cpp
	panes.h
	panes.cpp
	pulsar.h
	pulsar.cpp
	log.h
	log.cpp
	network.h
	network.cpp
	window.h
	window.cpp
	bot.h
	bot.cpp
	main.cpp
	resources/resources.qrc
)

if (CMAKE_BUILD_TYPE MATCHES Debug)
	target_compile_options(Celeste PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/W4>
		$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
	)
endif()

if (WIN32)
	set(CMAKE_CXX_FLAGS_RELEASE "-O2")
	set_property(TARGET Celeste PROPERTY WIN32_EXECUTABLE true)
	target_sources(Celeste PRIVATE win32.cpp resources/resources.rc)
	target_link_libraries(Celeste PRIVATE Qt::Widgets Qt::Network Qt::Mqtt Qt::Multimedia Qt::MultimediaWidgets Qt::WebSockets)
	target_compile_definitions(Celeste PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
else()
	include(CheckIPOSupported)
	check_ipo_supported(RESULT result)
	if(result)
		set_property(TARGET Celeste PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	endif()
	set(CMAKE_CXX_FLAGS_RELEASE "-O2 -pipe")
	target_sources(Celeste PRIVATE unix.cpp)
	target_link_libraries(Celeste PRIVATE Qt::Widgets Qt::Network Qt::Mqtt Qt::Multimedia Qt::MultimediaWidgets Qt::WebSockets)
	install(TARGETS Celeste)
endif()

if (WITH_PULSAR)
	add_library(Pulsar MODULE pulsar/pulsar.cpp)
	if (WIN32)
		set(OBS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/obs-studio)
		set(OBS_LIB_DIR ${OBS_SOURCE_DIR}/build_x64)
		if(NOT EXISTS ${OBS_SOURCE_DIR})
			include(FetchContent)
			FetchContent_Declare(
				obs-studio
				GIT_REPOSITORY	https://github.com/obsproject/obs-studio.git
				GIT_TAG			origin/master
				SOURCE_DIR		${OBS_SOURCE_DIR}
			)
			FetchContent_Populate(obs-studio)
		endif()
		if(NOT EXISTS ${OBS_LIB_DIR})
			execute_process(COMMAND cmake -DENABLE_BROWSER=OFF --preset windows-x64 WORKING_DIRECTORY ${OBS_SOURCE_DIR})
			execute_process(COMMAND cmake --build --preset windows-x64 WORKING_DIRECTORY ${OBS_SOURCE_DIR})
		endif()

		include_directories(${OBS_SOURCE_DIR}/libobs)
		include_directories(${OBS_SOURCE_DIR}/UI/obs-frontend-api/)
		target_include_directories(Pulsar SYSTEM PRIVATE ${OBS_INCLUDE_DIR}/obs)

		find_library(LIBOBS_LIBRARY obs
			${OBS_LIB_DIR}
			${OBS_LIB_DIR}/libobs/
			${OBS_LIB_DIR}/libobs/RelWithDebInfo/
			${OBS_LIB_DIR}/libobs/Relase/
			${OBS_LIB_DIR}/libobs/*/
		)

		find_library(OBS_FRONTEND_LIBRARY obs-frontend-api
			${OBS_LIB_DIR}
			${OBS_LIB_DIR}/UI/obs-frontend-api/
			${OBS_LIB_DIR}/UI/obs-frontend-api/RelWithDebInfo/
			${OBS_LIB_DIR}/UI/obs-frontend-api/Release/
			${OBS_LIB_DIR}/UI/obs-frontend-api/*/
		)
	else()
		find_package(Qt6 COMPONENTS Widgets Network REQUIRED)
		include(FindPkgConfig)
		pkg_search_module(OBS REQUIRED libobs)
		find_path(OBS_INCLUDE_DIR obs-module.h HINTS ${OBS_INCLUDEDIR} ${OBS_INCLUDE_DIRS} ${OBS_INCLUDE_DIRS}/obs)
		target_include_directories(Pulsar SYSTEM PRIVATE ${OBS_INCLUDE_DIR})
	endif()
	target_link_libraries(Pulsar ${LIBOBS_LIBRARY} ${OBS_FRONTEND_LIBRARY} Qt::Widgets Qt::Network)
endif()
