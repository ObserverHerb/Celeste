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
			const char *operation="profile image retrieval";
			QNetworkAccessManager* manager=new QNetworkAccessManager(this);
			manager->connect(manager,&QNetworkAccessManager::finished,[this,manager](QNetworkReply *reply) {
				manager->deleteLater();
				this->deleteLater();
				if (reply->error())
					emit Print(QString("Failed: %1").arg(reply->errorString()));
				else
					emit Retrieved(QImage::fromData(reply->readAll()));
			});
			emit Print(QString("Sending request: %1").arg(profileImageURL.toString()));
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
		const char *operation="request viewer information";
		QNetworkAccessManager* manager=new QNetworkAccessManager(this);
		connect(manager,&QNetworkAccessManager::finished,[this,operation,manager](QNetworkReply* reply) {
			manager->deleteLater();
			this->deleteLater();
			QJsonParseError jsonError;
			QJsonDocument json=QJsonDocument::fromJson(reply->readAll(),&jsonError);
			if (json.isNull())
			{
				emit Print(QString("Failed: %1").arg(jsonError.errorString()));
				return;
			}
			QJsonArray data=json.object().value("data").toArray();
			if (data.size() < 1)
			{
				emit Print("Invalid user");
				return;
			}
			QJsonObject details=data.at(0).toObject();
			emit Recognized(Local(details.value("name").toString(),details.value("id").toString(),details.value("display_name").toString(),data.at(0).toObject().value("profile_image_url").toString(),details.value("description").toString()));
		});
		QUrl query(TWITCH_API_ENDPOINT_USERS);
		query.setQuery({{"login",username}});
		emit Print(QString("Sending request: %1").arg(query.toString(),operation));
		QNetworkRequest request(query);
		request.setRawHeader("Authorization",StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(settingOAuthToken))));
		request.setRawHeader("Client-ID",settingClientID);
		manager->get(request);
	};
}
