#include <QFileInfo>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "entities.h"
#include "globals.h"

namespace Volume
{
	Fader::Fader(QMediaPlayer *player,const QString &arguments,QObject *parent) : Fader(player,0,static_cast<std::chrono::seconds>(0),parent)
	{
		Parse(arguments);
	}

	Fader::Fader(QMediaPlayer *player,unsigned int volume,std::chrono::seconds duration,QObject *parent) : QObject(parent),
		settingDefaultDuration(SETTINGS_CATEGORY_VOLUME,"DefaultFadeSeconds",5),
		player(player),
		initialVolume(static_cast<unsigned int>(player->volume())),
		targetVolume(std::clamp<int>(volume,0,100)),
		startTime(TimeConvert::Now()),
		duration(TimeConvert::Milliseconds(duration))
	{
		connect(this,&Fader::AdjustmentNeeded,this,&Fader::Adjust,Qt::QueuedConnection);
	}

	void Fader::Parse(const QString &text)
	{
		QStringList values=text.split(" ",StringConvert::Split::Behavior(StringConvert::Split::Behaviors::SKIP_EMPTY_PARTS));
		if (values.size() < 1) throw std::range_error("No volume specified");
		targetVolume=StringConvert::PositiveInteger(values[0]);
		duration=values.size() > 1 ? static_cast<std::chrono::seconds>(StringConvert::PositiveInteger(values[1])) : settingDefaultDuration;
	}

	void Fader::Start()
	{
		emit Print(QString("Adjusting volume from %1% to %2% over %3 seconds").arg(
			StringConvert::Integer(initialVolume),
			StringConvert::Integer(targetVolume),
			StringConvert::Integer(TimeConvert::Seconds(duration).count())
		));
		emit AdjustmentNeeded();
	}

	void Fader::Stop()
	{
		disconnect(this,&Fader::AdjustmentNeeded,this,&Fader::Adjust);
		deleteLater();
	}

	void Fader::Abort()
	{
		Stop();
		player->setVolume(initialVolume); // FIXME: volume doesn't return to initial value
	}

	double Fader::Step(const double &secondsPassed)
	{
		// uses x^4 method described here: https://www.dr-lex.be/info-stuff/volumecontrols.html#ideal3
		// no need to find an ideal curve, this is a bot, not a DAW
		double totalSeconds=static_cast<double>(duration.count())/TimeConvert::Milliseconds(TimeConvert::OneSecond()).count();
		int volumeDifference=targetVolume-initialVolume;
		return std::pow(secondsPassed/totalSeconds,4)*(volumeDifference)+initialVolume;
	}

	void Fader::Adjust()
	{
		double secondsPassed=static_cast<double>((TimeConvert::Now()-startTime).count())/TimeConvert::Milliseconds(TimeConvert::OneSecond()).count();
		if (secondsPassed > TimeConvert::Seconds(duration).count())
		{
			deleteLater();
			return;
		}
		double adjustment=Step(secondsPassed);
		player->setVolume(adjustment);
		emit AdjustmentNeeded();
	}
}

namespace File
{
	List::List(const QString &path)
	{
		const QFileInfo pathInfo(path);
		if (pathInfo.isDir())
		{
			for (const QFileInfo &fileInfo : QDir(path).entryInfoList())
			{
				if (fileInfo.isFile()) files.push_back(fileInfo.absoluteFilePath());
			}
		}
		else
		{
			files.push_back(pathInfo.absoluteFilePath());
		}
	}

	List::List(const QStringList &list)
	{
		files=list;
	}

	const QString List::File(const int index)
	{
		return files.at(index);
	}

	const QString List::First()
	{
		return files.front();
	}


	const QString List::Random()
	{
		return File(RandomIndex());
	}

	int List::RandomIndex()
	{
		return Random::Bounded(files);
	}
}


namespace Viewer
{
	const char *TWITCH_API_ENDPOINT_USERS="https://api.twitch.tv/helix/users";

	namespace ProfileImage
	{
		Remote::Remote(const QUrl &profileImageURL)
		{
			QNetworkAccessManager* manager=new QNetworkAccessManager(this);
			manager->connect(manager,&QNetworkAccessManager::finished,[this,manager](QNetworkReply *reply) {
				manager->deleteLater();
				this->deleteLater();
				if (reply->error())
					emit Print(QString("Failed: %1").arg(reply->errorString()),"profile image retrieval");
				else
					emit Retrieved(QImage::fromData(reply->readAll()));
			});
			manager->get(QNetworkRequest(profileImageURL));
		}
	}

	const QString& Local::Name() const
	{
		return name;
	}

	const QString& Local::ID() const
	{
		return id;
	}

	const QString& Local::DisplayName() const
	{
		return displayName;
	}

	ProfileImage::Remote* Local::ProfileImage() const
	{
		return new ProfileImage::Remote(profileImageURL);
	}

	const QString& Local::Description() const
	{
		return description;
	}

	Remote::Remote(const QString &username,const PrivateSetting &settingOAuthToken,const PrivateSetting &settingClientID) : name(username)
	{
		QNetworkAccessManager* manager=new QNetworkAccessManager(this);
		connect(manager,&QNetworkAccessManager::finished,[this,manager](QNetworkReply* reply) {
			const char *OPERATION="request viewer information";
			manager->deleteLater();
			this->deleteLater();
			QJsonParseError jsonError;
			QJsonDocument json=QJsonDocument::fromJson(reply->readAll(),&jsonError);
			if (json.isNull())
			{
				emit Print(QString("Failed: %1").arg(jsonError.errorString()),OPERATION);
				return;
			}
			QJsonArray data=json.object().value("data").toArray();
			if (data.size() < 1)
			{
				emit Print("Invalid user",OPERATION);
				return;
			}
			QJsonObject details=data.at(0).toObject();
			emit Recognized(Local(details.value("name").toString(),details.value("id").toString(),details.value("display_name").toString(),data.at(0).toObject().value("profile_image_url").toString(),details.value("description").toString()));
		});
		QUrl query(TWITCH_API_ENDPOINT_USERS);
		query.setQuery({{"login",username}});
		QNetworkRequest request(query);
		request.setRawHeader("Authorization",StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(settingOAuthToken))));
		request.setRawHeader("Client-ID",settingClientID);
		manager->get(request);
	};
}
