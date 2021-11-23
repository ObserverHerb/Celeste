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
	connect(this,&UserRecognizer::Error,this,&UserRecognizer::deleteLater);
	connect(this,&UserRecognizer::Recognized,this,&UserRecognizer::deleteLater);

	QNetworkAccessManager* manager=new QNetworkAccessManager(this);
	connect(manager,&QNetworkAccessManager::finished,[this,manager,username](QNetworkReply* reply) {
		manager->deleteLater();
		QJsonParseError jsonError;
		QJsonDocument json=QJsonDocument::fromJson(reply->readAll(),&jsonError);
		if (json.isNull() || !json.object().contains("data"))
		{
			emit Error(QString("Something went wrong asking Twitch for user %1's profile: %2").arg(username,jsonError.errorString()));
			return;
		}
		QJsonArray data=json.object().value("data").toArray();
		if (data.size() < 1)
		{
			emit Error(QString("%1 is not a valid Twitch user").arg(username));
			return;
		}
		QJsonObject details=data.at(0).toObject();
		emit Recognized(Viewer(username,details.value("id").toString(),details.value("display_name").toString(),std::make_shared<ProfileImage>(data.at(0).toObject().value("profile_image_url").toString()),details.value("description").toString()));
	});
	QUrl query(TWITCH_API_ENDPOINT_USERS);
	query.setQuery({{"login",username}});
	QNetworkRequest request(query);
	request.setRawHeader("Authorization",StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(settingOAuthToken))));
	request.setRawHeader("Client-ID",settingClientID);
	manager->get(request);
};

const QString& Viewer::Name() const
{
	return name;
}

const QString& Viewer::ID() const
{
	return id;
}

const QString& Viewer::DisplayName() const
{
	return displayName;
}

std::shared_ptr<ProfileImage> Viewer::ProfileImage() const
{
	return profileImage;
}

const QString& Viewer::Description() const
{
	return description;
}

ProfileImage::ProfileImage(const QUrl &profileImageURL)
{
	QNetworkAccessManager* manager=new QNetworkAccessManager(this);
	manager->connect(manager,&QNetworkAccessManager::finished,[this,manager](QNetworkReply *reply) {
		manager->deleteLater();
		if (reply->error())
			emit Error(QString("Something went wrong downloading profile image: %2").arg(reply->errorString()));
		else
			image=QImage::fromData(reply->readAll());
			emit ProfileImageDownloaded(image);
		});
	manager->get(QNetworkRequest(profileImageURL));
}

ProfileImage::operator QImage() const
{
	return image;
}