#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include "globals.h"
#include "recognizers.h"
#include "settings.h"

/*!
 * \class FileRecognizer
 * \brief Performs operations a file that has been "recognized" from a
 * path to a file or directory
 */

/*!
 * \fn FileRecognizer::FileRecognizer
 * \brief Default constructor
 *
 * \param path A path to a file or directory
 */
FileRecognizer::FileRecognizer(const QString &path)
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

const QString FileRecognizer::File(const int index)
{
	return files.at(index);
}

/*!
 * \fn FileRecognizer::First
 * \brief Return a path to the first file that was recognized
 */
const QString FileRecognizer::First()
{
	return files.front();
}

/*!
 * \fn FileRecognizer::First
 * \brief Return a path to a random file in the list of files
 * that was recognized
 */
const QString FileRecognizer::Random()
{
	return File(RandomIndex());
}

int FileRecognizer::RandomIndex()
{
	return Random::Bounded(files);
}

UserRecognizer::UserRecognizer(const QString &username) : name(username)
{
	QNetworkAccessManager* manager=new QNetworkAccessManager(this);
	connect(manager,&QNetworkAccessManager::finished,[this,manager,username](QNetworkReply* reply) {
		manager->deleteLater();
		QJsonParseError jsonError;
		QJsonDocument json=QJsonDocument::fromJson(reply->readAll(),&jsonError);
		if (json.isNull() || !json.object().contains("data"))
		{
			emit Error(QString("Something went wrong asking Twitch for user %1's profile: %2").arg(username,jsonError.errorString()));
			deleteLater();
			return;
		}
		QJsonArray data=json.object().value("data").toArray();
		if (data.size() < 1)
		{
			emit Error(QString("%1 is not a valid Twitch user").arg(username));
			deleteLater();
			return;
		}
		QJsonObject details=data.at(0).toObject();
		id=details.value("id").toString();
		displayName=details.value("display_name").toString();
		description=details.value("description").toString();
		DownloadProfileImage(data.at(0).toObject().value("profile_image_url").toString());
		connect(this,&UserRecognizer::ProfileImageDownloaded,[this]() {
			emit Recognized(this);
		});
	});
	QUrl query(TWITCH_API_ENDPOINT_USERS);
	query.setQuery({{"login",username}});
	QNetworkRequest request(query);
	request.setRawHeader("Authorization",StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(settingOAuthToken))));
	request.setRawHeader("Client-ID",settingClientID);
	manager->get(request);
};

const QString& UserRecognizer::Name() const
{
	return name;
}

const QString& UserRecognizer::ID() const
{
	return id;
}

const QString& UserRecognizer::DisplayName() const
{
	return displayName;
}

const QImage& UserRecognizer::ProfileImage() const
{
	return profileImage;
}

const QString& UserRecognizer::Description() const
{
	return description;
}

void UserRecognizer::DownloadProfileImage(const QString &url)
{
	QNetworkAccessManager* manager=new QNetworkAccessManager(this);
	connect(manager,&QNetworkAccessManager::finished,[this,manager](QNetworkReply *reply) {
		manager->deleteLater();
		if (reply->error())
		{
			emit Error(QString("Something went wrong user %1's profile image: %2").arg(name,reply->errorString()));
		}
		else
		{
			profileImage=QImage::fromData(reply->readAll());
			emit ProfileImageDownloaded();
		}
	});
	manager->get(QNetworkRequest(url));
}