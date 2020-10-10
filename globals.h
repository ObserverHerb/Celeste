#pragma once

#include <QString>
#include <chrono>
#include <stdexcept>

const QString ORGANIZATION_NAME("Sky-Meyg");
const QString APPLICATION_NAME("Celeste");
const QString TWITCH_HOST("irc.chat.twitch.tv");
const unsigned int TWITCH_PORT=6667;
const QString TWITCH_PING("PING :tmi.twitch.tv");
const QString TWITCH_PONG("PONG :tmi.twitch.tv");
const QString IRC_COMMAND_USER("NICK %1\n");
const QString IRC_COMMAND_PASSWORD("PASS oauth:%1\n");
const QString IRC_COMMAND_JOIN("JOIN #%1\n");
const QString IRC_VALIDATION_AUTHENTICATION("You are in a maze of twisty passages, all alike.");
const QString IRC_VALIDATION_JOIN("End of /NAMES list");
const QString COMMANDS_LIST_FILENAME("commands.json");

namespace StringConvert
{
	inline QByteArray ByteArray(const QString &value) { return value.toLocal8Bit(); } // TODO: how do I report that this failed?
	inline QString Integer(const int &value)
	{
		QString result=QString::number(value);
		if (result.isEmpty()) throw std::range_error("Unable to convert number to string");
		return result;
	}
	inline int Integer(const QString &value)
	{
		bool succeeded=false;
		int result=value.toInt(&succeeded);
		if (!succeeded) throw std::range_error("Unable to convert string to number");
		return result;
	}
}

namespace TimeConvert
{
	constexpr std::chrono::seconds Seconds(const std::chrono::milliseconds &value) { return std::chrono::duration_cast<std::chrono::seconds>(value); }
	constexpr int Interval(const std::chrono::seconds &value) { return value.count(); }
	constexpr int Interval(const std::chrono::milliseconds &value) { return value.count(); }
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
