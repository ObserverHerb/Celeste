#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cstring>
#include "entities.h"
#include "globals.h"

Q_DECLARE_METATYPE(std::chrono::milliseconds)

Command::Command(const QString &name,Command* const parent) : name(name), description(parent->description), type(parent->type), random(parent->random), path(parent->path), message(parent->message), protect(parent->protect), parent(parent)
{
	parent->children.push_back(this);
}

namespace Music
{
	Player::Player(QObject *parent) : player(new QMediaPlayer(this)),
		settingSuppressedVolume(SETTINGS_CATEGORY_VOLUME,"SuppressedLevel",10),
		volumeAdjustment(player,"volume")
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
		const char *OPERATION_ALBUM_ART="album art";
		const QString errorMessage("Failed to capture album art (%1)");

		try
		{
			return ID3::Tag(Filename()).AlbumCoverFront();
		}

		catch (const std::out_of_range &exception)
		{
			emit Print(errorMessage.arg(exception.what()),OPERATION_ALBUM_ART);
		}

		catch (const std::runtime_error &exception)
		{
			emit Print(errorMessage.arg(exception.what()),OPERATION_ALBUM_ART);
		}

		return {};
	}

	QString Player::Filename() const
	{
		return sources.currentMedia().request().url().toLocalFile();
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

	ApplicationSetting& Player::SuppressedVolume()
	{
		return settingSuppressedVolume;
	}

	namespace ID3
	{
		quint32 SyncSafe(const char *value)
		{
			quint32 result;
			std::memcpy(&result,value,sizeof(quint32));
			quint8 out=0;
			quint32 mask=0x7F000000;
			while (mask)
			{
				out >>= 1;
				out |= result & mask;
				mask >>= 8;
			}
			return result;
		}

		Tag::Tag(const QString &filename) : APIC(nullptr)
		{
			static const std::unordered_map<QString,Frame::Frame> FRAMES={
				{"APIC",Frame::Frame::APIC}
			};

			try
			{
				file.setFileName(filename);
				if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Failed to open mp3 file");
				Header header(file);
				while (file.pos() < header.Size())
				{
					Frame::Header frameHeader(file);

					auto headerID=FRAMES.find(frameHeader.ID());
					if (headerID == FRAMES.end())
					{
						file.skip(frameHeader.Size());
						continue;
					}

					switch (headerID->second)
					{
					case Frame::Frame::APIC:
						APIC=std::make_unique<Frame::APIC>(file,frameHeader.Size());
						break;
					}
				}
			}

			catch (const std::runtime_error &exception)
			{
				Q_UNUSED(exception)
				Destroy();
				throw;
			}
		}

		Tag::~Tag()
		{
			Destroy();
		}

		void Tag::Destroy()
		{
			file.close();
		}

		const QImage& Tag::AlbumCoverFront() const
		{
			return APIC ? APIC->Picture() : throw std::runtime_error("No image data in mp3 file");
		}
		
		Header::Header(QFile &file) : file(file)
		{
			ParseIdentifier();
			ParseVersion();
			ParseFlags();
			ParseSize();
		}

		void Header::ParseIdentifier()
		{
			QByteArray header=file.read(3);
			if (header.size() < 3) throw std::runtime_error("Failed to read header from mp3 file");
			if (header != "ID3") throw std::runtime_error("File is not a valid mp3 file");
		}

		void Header::ParseVersion()
		{
			char major;
			char minor;
			if (!file.getChar(&major) || !file.getChar(&minor)) throw std::runtime_error("Failed to read version information from mp3 file");
			versionMajor=major;
			versionMinor=minor;
		}

		void Header::ParseFlags()
		{
			char flags;
			if (!file.getChar(&flags)) throw std::runtime_error("Failed to read flags from mp3 file");
			extendedHeader=flags & 0x1000000;
			unsynchronization=flags & 0x10000000;
		}

		void Header::ParseSize()
		{
			char data[4];
			for (int index=3; index >= 0; index--)
			{
				if (!file.getChar(&data[index])) throw std::runtime_error("Failed to read size of frame from mp3 file");
			}
			std::memcpy(&size,data,sizeof(quint32));
		}

		quint32 Header::Size() const
		{
			return size+10; // entire size of tag is size + 10 byte header
		}

		namespace Frame
		{
			Header::Header(QFile &file) : file(file)
			{
				ParseID();
				ParseSize();
				file.skip(2); // skip frame flags
			}

			void Header::ParseID()
			{
				id=file.read(4);
				if (id.size() < 4) throw std::runtime_error("Invalid frame ID in mp3 file");
			}

			QByteArray Header::ID() const
			{
				return id;
			}

			void Header::ParseSize()
			{
				char data[4];
				for (int index=3; index >= 0; index--)
				{
					if (!file.getChar(&data[index])) throw std::runtime_error("Failed to read size of frame from mp3 file");
				}
				size=SyncSafe(data);
			}
			
			quint32 Header::Size() const
			{
				return size;
			}

			APIC::APIC(QFile &file,quint32 size) : file(file), size(size)
			{
				ParseEncoding();
				ParseMIMEType();
				ParsePictureType();
				ParseDescription();
				ParsePictureData();
			}

			void APIC::ParseEncoding()
			{
				char data;
				if (!file.getChar(&data)) throw std::runtime_error("Invalid encoding in frame of mp3 file");
				unsigned short numeric=data;
				if (numeric > 3) throw std::out_of_range("Unrecognized encoding in frame of mp3 file");
				encoding=static_cast<Encoding>(numeric);
			}

			void APIC::ParseMIMEType()
			{
				char byte;
				do
				{
					if (!file.getChar(&byte)) throw std::runtime_error("Invalie MIME type in frame of mp3 file");
					MIMEType.append(byte);
				}
				while (byte != '\0');
			}

			void APIC::ParsePictureType()
			{
				char data;
				if (!file.getChar(&data)) throw std::runtime_error("Invalid picture type in frame of mp3 file");
				unsigned short numeric=data;
				if (numeric > 14) throw std::out_of_range("Unrecognized picture type in frame of mp3 file");
				pictureType=static_cast<PictureType>(numeric);
			}

			void APIC::ParseDescription()
			{
				int chunkSize=encoding == Encoding::UTF_16 || encoding == Encoding::UTF_16BE ? 2 : 1;
				QByteArray terminator(chunkSize,'\0');
				QByteArray data;
				do
				{
					data=file.read(chunkSize);
					if (data.size() < chunkSize) throw std::runtime_error("Invalid description in frame of mp3 file");
					description.append(data);
				}
				while (data != terminator);
			}

			void APIC::ParsePictureData()
			{
				quint32 length=DataSize();
				QByteArray data=file.read(length);
				if (data.size() < length) throw std::runtime_error("Invalid image data in frame of mp3 file");
				picture=QImage::fromData(data);
			}

			const QImage& APIC::Picture() const
			{
				return picture;
			}

			quint32 APIC::DataSize()
			{
				int value=size-2-MIMEType.size()-description.size(); // 2 = 1 byte for encoding, 1 byte for picture type
				return value;
			}
		}
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
			emit Recognized({details.value("login").toString(),details.value("id").toString(),details.value("display_name").toString(),details.value("profile_image_url").toString(),details.value("description").toString()});
		},{
			{"login",username}
		},{
			{"Authorization",StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
			{"Client-ID",security.ClientID()}
		});
	};
}
