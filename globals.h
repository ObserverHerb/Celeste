#pragma once

#include <QString>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QFont>
#include <QFontMetrics>
#include <chrono>
#include <optional>
#include <random>
#include <stdexcept>

using namespace Qt::Literals::StringLiterals;

namespace Resources
{
	inline const char *CELESTE=":/celeste.png";
}

namespace Concept
{
	template<typename T> concept Container=requires(T m)
	{
		requires std::forward_iterator<typename T::iterator>;
		{ m.empty() }->std::same_as<bool>;
		{ m.size() }->std::same_as<typename T::size_type>;
	};

	template<typename T> concept AssociativeContainer=requires(T m,typename T::key_type k)
	{
		typename T::key_type;
		typename T::mapped_type;
		typename T::const_iterator;
		{ m.find(k) }->std::convertible_to<typename T::const_iterator>;
	};

	template <typename T> concept Widget=std::is_base_of<QWidget,T>::value;
}

namespace NumberConvert
{
	template <std::integral T>
	inline unsigned int Positive(T value)
	{
		if (static_cast<std::size_t>(std::abs(value)) > static_cast<std::size_t>(std::numeric_limits<unsigned int>::max())) throw std::range_error("Overflow converting to positive integer");
		return value >= 0 ? static_cast<unsigned int>(value) : 0;
	}
}

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

	template <std::unsigned_integral T>
	inline QString NumberAgreement(const QString &singular,const QString &plural,T count)
	{
		return count == 1 ? singular : plural;
	}

	inline const QString SafeDump(const QString &data)
	{
#if defined QT_DEBUG && defined DEVELOPER_MODE
		QStringList lines=data.split("\n",Qt::SkipEmptyParts);
		for (QString &line : lines) line.prepend("> ");
		return lines.join("\n");
#else
		Q_UNUSED(data)
		return QString("> (data)");
#endif
	}

	inline int RestrictFontWidth(QFont font,const QString &text,int maxPixels)
	{
		int originalPointSize=font.pointSize();
		QFontMetrics metrics{font};
		QRect bounds=metrics.boundingRect(text);
		while (bounds.width() > maxPixels)
		{
			int reducedPointSize=font.pointSize()-1;
			if (reducedPointSize < 1) return originalPointSize;
			font.setPointSize(reducedPointSize);
			metrics=QFontMetrics{font};
			bounds=metrics.boundingRect(text);
		}
		return font.pointSize();
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

namespace StringView
{
	inline std::optional<QStringView> Take(QStringView &window,QChar delimiter)
	{
		QStringView candidate=window.left(window.indexOf(delimiter));
		window=window.mid(candidate.size()+1);
		if (candidate.isEmpty()) return std::nullopt;
		return candidate.trimmed();
	}

	inline std::optional<QStringView> Take(QStringView &window,QChar lead,QChar delimiter)
	{
		if (window.front() != lead) return std::nullopt;
		window=window.mid(1);
		return Take(window,delimiter);
	}

	inline std::optional<QStringView> First(const QStringView &window,QChar delimiter)
	{
		QStringView candidate=window.left(window.indexOf(delimiter));
		if (candidate.isEmpty()) return std::nullopt;
		return candidate.trimmed();
	}

	inline std::optional<QStringView> Last(const QStringView &window,QChar delimiter)
	{
		QStringView candidate=window.mid(window.lastIndexOf(delimiter)+1);
		if (candidate.isEmpty()) return std::nullopt;
		return candidate.trimmed();
	}
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

	inline const QDir HomePath()
	{
		return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
	}

	const std::optional<QString> CreateHiddenFile(const QString &filePath);

	inline bool Touch(QFile &file)
	{
		const QDir path(QFileInfo(file).absolutePath());
		if (!file.exists())
		{
			if (!path.mkpath(path.absolutePath())) return false;
			if (!file.open(QIODevice::WriteOnly)) return false;
			file.close();
		}
		return true;
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

	template<Concept::Container T> inline int Bounded(const T &container)
	{
		if (container.empty()) throw std::range_error("Tried to pull random item from empty container");
		if (container.size() > std::numeric_limits<int>::max()) throw std::range_error("Container contains too many elements");
		return Bounded(0,static_cast<int>(container.size())-1);
	}

	template<Concept::Container T> inline void Shuffle(T &container)
	{
		std::shuffle(container.begin(),container.end(),generator);
	}
}

namespace JSON
{
	namespace Keys
	{
		inline const char *DATA="data";
	}

	struct ParseResult
	{
		bool success;
		QJsonDocument json;
		QString error;
		operator bool() const { return success; }
		QJsonDocument operator()() const { return json; }
	};

	inline ParseResult Parse(const QByteArray &data)
	{
		QJsonParseError jsonError={
			.offset=0,
			.error=QJsonParseError::NoError
		};
		const QJsonDocument json=QJsonDocument::fromJson(data,&jsonError);
		QString error;
		bool success=true;
		if (jsonError.error != QJsonParseError::NoError)
		{
			success=false;
			error=jsonError.errorString();
		}
		return {
			.success=success,
			.json=json,
			.error=error
		};
	}
}

namespace Container
{
	template <Concept::AssociativeContainer T>
	typename T::mapped_type Resolve(T &container,const typename T::key_type &key,const typename T::mapped_type &value)
	{
		auto candidate=container.find(key);
		return candidate == container.end() ? value : *candidate;
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
