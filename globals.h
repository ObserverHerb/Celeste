#pragma once

#include <QString>
#include <QStandardPaths>
#include <QDir>
#include <chrono>
#include <optional>
#include <random>
#include <functional>
#include <stdexcept>
#include "platform.h"

inline const char *ORGANIZATION_NAME="Sky-Meyg";
inline const char *APPLICATION_NAME="Celeste";
inline const char *TWITCH_HOST="irc.chat.twitch.tv";
inline const unsigned int TWITCH_PORT=6667;
inline const char *TWITCH_PING="PING :tmi.twitch.tv\n";
inline const char *TWITCH_PONG="PONG :tmi.twitch.tv\n";
inline const char *TWITCH_API_ENDPOINT_EMOTE_LIST="https://api.twitch.tv/kraken/chat/emoticons";
inline const char *TWITCH_API_ENDPOINT_EMOTE_URL="https://static-cdn.jtvnw.net/emoticons/v1/%1/1.0";
inline const char *TWITCH_API_ENDPOINT_USERS="https://api.twitch.tv/helix/users";
inline const char *TWITCH_API_ENDPOINT_EVENTSUB="https://api.twitch.tv/helix/eventsub/subscriptions";
inline const char *TWITCH_API_VERSION_5="application/vnd.twitchtv.v5+json";
inline const char *IRC_COMMAND_USER="NICK %1\n";
inline const char *IRC_COMMAND_PASSWORD="PASS oauth:%1\n";
inline const char *IRC_COMMAND_JOIN="JOIN #%1\n";
inline const char *IRC_VALIDATION_AUTHENTICATION="You are in a maze of twisty passages, all alike.";
inline const char *IRC_VALIDATION_JOIN="End of /NAMES list";
inline const char *COMMANDS_LIST_FILENAME="commands.json";
inline const char *EMOTE_FILENAME="emoticons.json";
inline const char *LOG_DIRECTORY="logs";
inline const short KEY=0;
inline const short VALUE=1;

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
	inline QString PositiveInteger(const unsigned int &value)
	{
		QString result=QString::number(value);
		if (result.isEmpty()) throw std::range_error("Unable to convert number to text");
		return result;
	}
	inline unsigned int PositiveInteger(const QString &value)
	{
		bool succeeded=false;
		unsigned int result=value.toUInt(&succeeded);
		if (!succeeded) throw std::range_error("Unable to convert text to positive number");
		return result;
	}

	class Valid : public QString
	{
	public:
		Valid(const QString &string) : QString(string)
		{
			if (string.isEmpty() || string.isNull()) throw std::runtime_error("Invalid or empty string was passed");
		}
	};
	
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

namespace Console
{
	inline const QString GenerateMessage(const QString &subsystem,const QString &message)
	{
		return QString("== %1\n%2").arg(subsystem.toUpper(),message);
	}

	inline const QString GenerateMessage(const QString &subsystem,const QString &operation,const QString &message)
	{
		return GenerateMessage(QString("%1 (%2)").arg(subsystem,operation),message);
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

namespace Filesystem
{
	inline const QDir DataPath()
	{
		return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	}

	inline const QDir LogPath()
	{
		return DataPath().absoluteFilePath(LOG_DIRECTORY);
	}
}

namespace Random
{
	inline std::random_device generator;
	inline int Bounded(int lower,int upper)
	{
		std::uniform_int_distribution<int> distribution(lower,upper);
		return distribution(generator);
	}
	inline int Bounded(std::size_t lower,std::size_t upper)
	{
		if (upper > std::numeric_limits<int>::max()) throw std::range_error("Range is larger than integer type");
		return Bounded(static_cast<int>(lower),static_cast<int>(upper));
	}
	template<typename T>
	inline int Bounded(const std::vector<T> &container) // TODO: good candidate for concepts/constraints here when they land, so we can use any type of container with a size
	{
		if (container.empty()) throw std::range_error("Tried to pull random item from empty container");
		return Bounded(static_cast<std::size_t>(0),container.size()-1);
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