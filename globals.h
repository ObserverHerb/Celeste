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

inline const char *TWITCH_API_VERSION_5="application/vnd.twitchtv.v5+json";
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

	inline const QString Dump(const QString &data)
	{
#ifdef DEVELOPER_MODE
		QStringList lines=data.split("\n",Split::Behavior(Split::Behaviors::SKIP_EMPTY_PARTS));
		for (QString &line : lines) line.prepend("> ");
		return lines.join("\n");
#else
		return QString("> (data)");
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

namespace Filesystem
{
	inline const QDir DataPath()
	{
		return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	}

	inline const QDir TemporaryPath()
	{
		return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
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
	template<typename K,typename V>
	inline int Bounded(const std::unordered_map<K,V> &container) // TODO: good candidate for concepts/constraints here when they land, so we can use any type of container with a size
	{
		if (container.empty()) throw std::range_error("Tried to pull random item from empty container");
		return Bounded(static_cast<std::size_t>(0),container.size()-1);
	}
	template<typename T>
	inline int Bounded(const QList<T> &container)
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
