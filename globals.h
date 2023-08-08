#pragma once

#include <QString>
#include <QStandardPaths>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFont>
#include <QFontMetrics>
#include <chrono>
#include <optional>
#include <random>
#include <functional>
#include <queue>
#include <concepts>
#include <stdexcept>

using namespace Qt::Literals::StringLiterals;

namespace Resources
{
	inline const char *CELESTE=":/celeste.png";
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
	inline const char* Raw(const QString &value) { return ByteArray(value).data(); }
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
#ifdef QT_DEBUG
#ifdef DEVELOPER_MODE
		QStringList lines=data.split("\n",Split::Behavior(Split::Behaviors::SKIP_EMPTY_PARTS));
		for (QString &line : lines) line.prepend("> ");
		return lines.join("\n");
#else
		return QString("> (data)");
#endif
#endif
	}

	inline int RestrictFontWidth(QFont font,const QString &text,int maxPixels)
	{
		int originalPointSize=font.pointSize();
		QFontMetrics metrics{font};
		QRect bounds=metrics.boundingRect(text);
		while (bounds.width() > maxPixels)
		{
			font.setPointSize(font.pointSize()-1);
			if (font.pointSize() < 1) return originalPointSize;
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
	template<typename T> concept Container=requires(T m)
	{
		requires std::forward_iterator<typename T::iterator>;
		{ m.empty() }->std::same_as<bool>;
		{ m.size() }->std::same_as<typename T::size_type>;
	};

	inline std::random_device generator;

	inline int Bounded(int lower,int upper)
	{
		std::uniform_int_distribution<int> distribution(lower,upper);
		return distribution(generator);
	}

	template<Container T> inline int Bounded(const T &container)
	{
		if (container.empty()) throw std::range_error("Tried to pull random item from empty container");
		if (container.size() > std::numeric_limits<int>::max()) throw std::range_error("Container contains too many elements");
		return Bounded(0,static_cast<int>(container.size())-1);
	}

	template<Container T> inline void Shuffle(T &container)
	{
		std::shuffle(container.begin(),container.end(),generator);
	}
}

namespace Multimedia
{
	inline QMediaPlayer* Player(QObject *parent,qreal initialVolume)
	{
		QMediaPlayer *player=new QMediaPlayer(parent);
		player->setAudioOutput(new QAudioOutput(parent));
		player->audioOutput()->setVolume(initialVolume);
		return player;
	}
}

namespace Network
{
	inline const char *CONTENT_TYPE="Content-Type";
	inline const char *CONTENT_TYPE_PLAIN="text/plain";
	inline const char* CONTENT_TYPE_HTML="text/html";
	inline const char *CONTENT_TYPE_JSON="application/json";
	inline const char *CONTENT_TYPE_FORM="application/x-www-form-urlencoded";

	static QNetworkAccessManager networkManager;

	enum class Method
	{
		GET,
		POST,
		PATCH
	};

	using Reply=std::function<void(QNetworkReply*)>;
	static std::queue<std::pair<std::function<void()>,Reply>> queue;

	inline void Request(QUrl url,Method method,Reply callback,const QUrlQuery &queryParameters=QUrlQuery(),const std::vector<std::pair<QByteArray,QByteArray>> &headers=std::vector<std::pair<QByteArray,QByteArray>>(),const QByteArray &payload=QByteArray())
	{
		networkManager.connect(&networkManager,&QNetworkAccessManager::finished,&networkManager,[](QNetworkReply *reply) {
			reply->deleteLater();
		},Qt::QueuedConnection);

		QNetworkRequest request;
		for (const std::pair<QByteArray,QByteArray> &header : headers) request.setRawHeader(header.first,header.second);
		switch (method)
		{
		case Method::GET:
		{
			url.setQuery(queryParameters);
			request.setUrl(url);
			auto sendRequest=[request,callback]() {
				QNetworkReply *reply=networkManager.get(request);
				networkManager.connect(&networkManager,&QNetworkAccessManager::finished,reply,callback,Qt::QueuedConnection);
				reply->connect(reply,&QNetworkReply::finished,reply,[]() {
					int size=queue.size();
					queue.pop();
					if (queue.size() > 0) queue.front().first();
				},Qt::QueuedConnection);
			};
			if (queue.size() == 0) sendRequest();
			queue.push({sendRequest,callback});
			break;
		}
		case Method::POST:
			request.setUrl(url);
			networkManager.connect(&networkManager,&QNetworkAccessManager::finished,networkManager.post(request,payload.isEmpty() ? StringConvert::ByteArray(queryParameters.query()) : payload),callback,Qt::QueuedConnection);
			break;
		case Method::PATCH:
			url.setQuery(queryParameters);
			request.setUrl(url);
			networkManager.connect(&networkManager,&QNetworkAccessManager::finished,networkManager.sendCustomRequest(request,"PATCH",payload),callback,Qt::QueuedConnection);
			break;
		}
	}
}

namespace JSON
{
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
	template<typename T> concept AssociativeContainer=requires(T m,typename T::key_type k)
	{
		typename T::key_type;
		typename T::mapped_type;
		typename T::const_iterator;
		{ m.find(k) }->std::convertible_to<typename T::const_iterator>;
	};

	template <AssociativeContainer T>
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
