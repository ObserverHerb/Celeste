#pragma once

#include <QString>
#include <chrono>
#include <random>
#include <functional>
#include <stdexcept>

inline const char *ORGANIZATION_NAME="Sky-Meyg";
inline const char *APPLICATION_NAME="Celeste";
inline const char *TWITCH_HOST="irc.chat.twitch.tv";
inline const unsigned int TWITCH_PORT=6667;
inline const char *TWITCH_PING="PING :tmi.twitch.tv\n";
inline const char *TWITCH_PONG="PONG :tmi.twitch.tv\n";
inline const char *IRC_COMMAND_USER="NICK %1\n";
inline const char *IRC_COMMAND_PASSWORD="PASS oauth:%1\n";
inline const char *IRC_COMMAND_JOIN="JOIN #%1\n";
inline const char *IRC_VALIDATION_AUTHENTICATION="You are in a maze of twisty passages, all alike.";
inline const char *IRC_VALIDATION_JOIN="End of /NAMES list";
inline const char *COMMANDS_LIST_FILENAME="commands.json";

namespace StringConvert
{
	inline QByteArray ByteArray(const QString &value) { return value.toLocal8Bit(); } // TODO: how do I report that this failed?
	inline QString Integer(const int &value)
	{
		QString result=QString::number(value);
		if (result.isEmpty()) throw std::range_error("Unable to convert number to text");
		return result;
	}
	inline int Integer(const QString &value)
	{
		bool succeeded=false;
		int result=value.toInt(&succeeded);
		if (!succeeded) throw std::range_error("Unable to convert text to number");
		return result;
	}
	inline unsigned int PositiveInteger(const QString &value)
	{
		bool succeeded=false;
		unsigned int result=value.toUInt(&succeeded);
		if (!succeeded) throw std::range_error("Unable to convert text to positive number");
		return result;
	}

	namespace Split
	{
#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
		enum class Behaviors
		{
			KEEP_EMPTY_PARTS=QString::SplitBehavior::KeepEmptyParts,
			SKIP_EMPTY_PARTS=QString::SplitBehavior::SkipEmptyParts
		};
		inline QString::SplitBehavior Behavior(Behaviors behaviors) { return static_cast<QString::SplitBehavior>(behaviors); }
#else
		enum class Behaviors
		{
			KEEP_EMPTY_PARTS=Qt::KeepEmptyParts,
			SKIP_EMPTY_PARTS=Qt::SkipEmptyParts
		};
		inline Qt::SplitBehaviorFlags Behavior(Behaviors behaviors) { return static_cast<Qt::SplitBehaviorFlags>(behaviors); }
#endif
	}
}

namespace TimeConvert
{
	constexpr std::chrono::seconds Seconds(const std::chrono::milliseconds &value) { return std::chrono::duration_cast<std::chrono::seconds>(value); }
	constexpr std::chrono::milliseconds Milliseconds(const std::chrono::seconds &value) { return std::chrono::duration_cast<std::chrono::milliseconds>(value); }
	constexpr int Interval(const std::chrono::seconds &value) { return value.count(); }
	constexpr int Interval(const std::chrono::milliseconds &value) { return value.count(); }
	constexpr std::chrono::seconds OneSecond() { return static_cast<std::chrono::seconds>(1); }
	inline const std::chrono::milliseconds Now() { return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch(); }
}

namespace Tuple
{
	template<typename Type,typename... Arguments>
	Type* New(std::tuple<Arguments...> tuple)
	{
		return std::apply([](Arguments... arguments)->Type* {
			return new Type(arguments...);
		},tuple);
	}
}

namespace Platform
{
	constexpr bool Windows()
	{
	#ifdef Q_OS_WIN
		return true;
	#else
		return false;
	#endif
	}
}

namespace Random
{
	inline std::default_random_engine generator;
	inline int Bounded(int lower,int upper)
	{
		std::uniform_int_distribution<int> distribution(lower,upper);
		return distribution(generator);
	}
	template<typename T>
	inline int Bounded(const std::vector<T> &container) // TODO: good candidate for concepts/constraints here when they land, so we can use any type of container with a size
	{
		if (container.empty()) throw std::range_error("Tried to pull random item from empty container");
		return Bounded(0,container.size()-1);
	}
}

#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
template<>
struct std::hash<QString>
{
	std::size_t operator()(const QString &string) const noexcept
	{
		return qHash(string);
	}
};
#endif
