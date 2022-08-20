#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "entities.h"
#include "globals.h"

Q_DECLARE_METATYPE(std::chrono::milliseconds)

namespace Music
{
	Player::Player(QObject *parent) : player(new QMediaPlayer(this)),
		volumeAdjustment(player,"volume"),
		settingSuppressedVolume(SETTINGS_CATEGORY_VOLUME,"SuppressedLevel",10)
	{
		player->setVolume(0);

		connect(player,QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),this,&Player::ConvertError);
		connect(player,&QMediaPlayer::stateChanged,this,&Player::StateChanged);

		connect(&sources,&QMediaPlaylist::loadFailed,this,[this]() {
			emit Print(QString("Failed to load vibe playlist: %1").arg(sources.errorString()));
		}); // TODO: this shouldn't be a lambda anymore
		connect(&sources,&QMediaPlaylist::loaded,this,&Player::PlaylistLoaded);
	}

	void Player::Load(const QString &location)
	{
		Load(QUrl::fromLocalFile(location));
	}

	void Player::Load(const QUrl &location)
	{
		sources.load(location);
	}

	void Player::Start()
	{
		player->play();
	}

	void Player::Stop()
	{
		player->pause();
	}

	bool Player::Playing() const
	{
		return player->state() == QMediaPlayer::PlayingState;
	}

	void Player::DuckVolume(bool duck)
	{
		if (player->volume() < static_cast<int>(settingSuppressedVolume)) return;

		if (duck)
		{
			if (volumeAdjustment.state() == QAbstractAnimation::Running) volumeAdjustment.pause();
			volumeAdjustment.setStartValue(player->volume());
			player->setVolume(settingSuppressedVolume);
		}
		else
		{
			int originalVolume=volumeAdjustment.startValue().toInt();
			if (player->volume() < originalVolume) player->setVolume(originalVolume);
			if (volumeAdjustment.state() == QAbstractAnimation::Paused) volumeAdjustment.resume();
		}
	}

	void Player::Volume(unsigned int volume)
	{
		player->setVolume(volume);
	}

	void Player::Volume(unsigned int targetVolume,std::chrono::seconds duration)
	{
		if (volumeAdjustment.state() == QAbstractAnimation::Paused) return;

		emit Print(QString("Adjusting volume from %1% to %2% over %3 seconds").arg(
			StringConvert::Integer(player->volume()),
			StringConvert::Integer(targetVolume),
			StringConvert::Integer(duration.count())
		),"volume fade");

		if (volumeAdjustment.state() == QAbstractAnimation::Running) volumeAdjustment.stop();
		volumeAdjustment.setDuration(TimeConvert::Milliseconds(duration).count());
		volumeAdjustment.setStartValue(player->volume());
		volumeAdjustment.setEndValue(targetVolume);
		volumeAdjustment.start();
	}

	QString Player::SongTitle() const
	{
		return player->metaData("Title").toString();
	}

	QString Player::AlbumTitle() const
	{
		return player->metaData("AlbumTitle").toString();
	}

	QString Player::AlbumArtist() const
	{
		return player->metaData("AlbumArtist").toString();
	}

	QImage Player::AlbumCoverArt() const
	{
		return player->metaData("CoverArtImage").value<QImage>();
	}

	void Player::StateChanged(QMediaPlayer::State state)
	{
		if (state == QMediaPlayer::PlayingState) emit Print(QString("Now playing %1 by %2").arg(player->metaData("Title").toString(),player->metaData("AlbumArtist").toString()));
	}

	void Player::ConvertError(QMediaPlayer::Error error)
	{
		emit Print(QString("Failed to start: %1").arg(player->errorString()));
	}

	void Player::PlaylistLoaded()
	{
		sources.shuffle();
		sources.setCurrentIndex(Random::Bounded(0,sources.mediaCount()-1)); // TODO: create bounded template that can accept QMediaPlaylist?
		player->setPlaylist(&sources);
		emit Print("Playlist loaded!");
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
			Network::Request(profileImageURL,Network::Method::GET,[this](QNetworkReply *reply) {
				this->deleteLater();
				if (reply->error())
					emit Print(QString("Failed: %1").arg(reply->errorString()),"profile image retrieval");
				else
					emit Retrieved(QImage::fromData(reply->readAll()));
			});
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

	Remote::Remote(Security &security,const QString &username) : name(username)
	{
		Network::Request({TWITCH_API_ENDPOINT_USERS},Network::Method::GET,[this](QNetworkReply* reply) {
			const char *OPERATION="request viewer information";
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
			emit Recognized(Local(details.value("login").toString(),details.value("id").toString(),details.value("display_name").toString(),data.at(0).toObject().value("profile_image_url").toString(),details.value("description").toString()));
		},{
			{"login",username}
		},{
			{"Authorization",StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
			{"Client-ID",security.ClientID()}
		});
	};
}
