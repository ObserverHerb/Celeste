#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <algorithm>
#include <cstring>
#include "entities.h"
#include "globals.h"
#include "network.h"
#include "twitch.h"

Q_DECLARE_METATYPE(std::chrono::milliseconds)

Command::Command(const QString &name,Command* const parent) : name(name), description(parent->description), type(parent->type), random(parent->random), duplicates(parent->duplicates), protect(parent->protect), path(parent->path), files(parent->files), message(parent->message), parent(parent)
{
	parent->children.push_back(this);
}

QStringList Command::FileListFilters(const CommandType type)
{
	QStringList filters={"*.*"};
	if (type == CommandType::VIDEO) filters={"*.mp4"};
	if (type == CommandType::AUDIO) filters={"*.mp3"};
	return filters;
}

const QString Command::File()
{
	if (random)
	{
		if (duplicates)
			return files->Random();
		else
			return files->Unique();
	}
	return files->First();
}

namespace File
{
	List::List(const QString &path,const QStringList &filters) : currentIndex(0)
	{
		const QFileInfo pathInfo(path);
		if (pathInfo.isDir())
		{
			const QFileInfoList fileInfoList=QDir(path).entryInfoList(filters);
			for (const QFileInfo &fileInfo : fileInfoList)
			{
				if (fileInfo.isFile()) files.push_back(fileInfo.absoluteFilePath());
			}
		}
		else
		{
			files.push_back(pathInfo.absoluteFilePath());
		}
		Shuffle();
	}

	List::List(const QStringList &list) : currentIndex(0)
	{
		files=list;
		Shuffle();
	}

	const QString List::File(const int index) const
	{
		if (index < 0 || index >= files.size()) throw std::out_of_range("Index not valid for file list");
		return files.at(index);
	}

	const QString List::First()
	{
		if (files.size() < 1) throw std::out_of_range("File list has no first item");
		return files.front();
	}

	const QString List::Random()
	{
		if (files.isEmpty()) throw std::out_of_range("Cannot select random item from empty list");
		return File(RandomIndex());
	}

	const QString List::Unique()
	{
		if (files.isEmpty()) throw std::out_of_range("Cannot select unique item from empty list");
		const QString candidate=files.at(currentIndex);
		if (++currentIndex == files.size()) Reshuffle();
		return candidate;
	}

	int List::RandomIndex()
	{
		return Random::Bounded(files);
	}

	const QStringList& List::operator()() const
	{
		return files;
	}

	void List::Shuffle()
	{
		Random::Shuffle(files);
		currentIndex=0;
	}

	void List::Reshuffle()
	{
		QString last=files.takeLast();
		Shuffle();
		files.append(last);
	}
}

namespace Music
{
	const char *Player::ERROR_LOADING="Failed to load vibe playlist";
	const char *Player::OPERATION_LOADING="load vibe playlist";

	Player::Player(bool loop,int initialVolume,QObject *parent) : QObject(parent),
		player(this),
		output(this),
		loop(loop),
		settingSuppressedVolume("Volume","SuppressedLevel",10),
		volumeAdjustment(&output,"volume")
	{
		player.setAudioOutput(&output);
		player.audioOutput()->setVolume(TranslateVolume(initialVolume));

		connect(&player,&QMediaPlayer::errorOccurred,this,&Player::MediaError);
		connect(&player,&QMediaPlayer::playbackStateChanged,this,&Player::StateChanged);
		connect(&player,&QMediaPlayer::mediaStatusChanged,this,&Player::MediaStatusChanged);
		connect(&volumeAdjustment,&QPropertyAnimation::finished,this,&Player::VolumeMuted);
	}

	void Player::Start(QUrl source)
	{
		Next(source);
		Start();
	}

	void Player::Start()
	{
		if (!Empty()) player.play();
	}

	bool Player::Empty()
	{
		if (sources().isEmpty())
		{
			emit Print("No songs available to play.");
			return true;
		}
		return false;
	}

	void Player::Stop()
	{
		player.pause();
	}

	bool Player::Playing() const
	{
		return player.playbackState() == QMediaPlayer::PlayingState;
	}

	void Player::DuckVolume(bool duck)
	{
		if (TranslateVolume(player.audioOutput()->volume()) < static_cast<int>(settingSuppressedVolume)) return;

		if (duck)
		{
			if (volumeAdjustment.state() == QAbstractAnimation::Running) volumeAdjustment.pause();
			volumeAdjustment.setStartValue(player.audioOutput()->volume());
			player.audioOutput()->setVolume(TranslateVolume(static_cast<int>(settingSuppressedVolume)));
		}
		else
		{
			qreal originalVolume=volumeAdjustment.startValue().toFloat();
			if (player.audioOutput()->volume() < originalVolume) player.audioOutput()->setVolume(originalVolume);
			if (volumeAdjustment.state() == QAbstractAnimation::Paused) volumeAdjustment.resume();
		}
	}

	void Player::Volume(int volume)
	{
		player.audioOutput()->setVolume(TranslateVolume(volume));
	}

	void Player::Volume(int targetVolume,std::chrono::seconds duration)
	{
		if (volumeAdjustment.state() == QAbstractAnimation::Paused) return;

		emit Print(QString("Adjusting volume from %1% to %2% over %3 seconds").arg(
			StringConvert::Integer(TranslateVolume(player.audioOutput()->volume())),
			StringConvert::Integer(targetVolume),
			StringConvert::Integer(duration.count())
		),"volume fade");

		if (volumeAdjustment.state() == QAbstractAnimation::Running) volumeAdjustment.stop();
		volumeAdjustment.setDuration(TimeConvert::Milliseconds(duration).count());
		volumeAdjustment.setStartValue(player.audioOutput()->volume());
		volumeAdjustment.setEndValue(TranslateVolume(targetVolume));
		volumeAdjustment.start();
	}

	void Player::VolumeMuted()
	{
		if (player.audioOutput()->volume() == 0.0f)
		{
			emit Print(QString("Silencing vibe player"));
			player.stop();
		}
	}

	Metadata Player::Metadata() const
	{
		try
		{
			ID3::Tag tag(Filename());
			auto title=tag.Title();
			auto album=tag.AlbumTitle();
			auto artist=tag.Artist();
			auto cover=tag.AlbumCoverFront();
			return {
				.title=title ? *title : QString{},
				.album=album ? *album : QString{},
				.artist=artist ? *artist : QString{},
				.cover=cover ? *cover : QImage{},
				.valid=true
			};
		}

		catch (const std::runtime_error &exception)
		{
			emit Print(exception.what());
		}

		return {};
	}

	QString Player::Filename() const
	{
		return player.source().toLocalFile();
	}

	void Player::Sources(const File::List &sources)
	{
		player.stop();
		this->sources=sources;
		Next();
	}

	const File::List& Player::Sources()
	{
		return sources;
	}

	void Player::StateChanged(QMediaPlayer::PlaybackState state)
	{
		switch (state)
		{
		case QMediaPlayer::PlayingState:
			try
			{
				ID3::Tag tag(Filename());
				auto title=tag.Title();
				if (!title) break;
				auto album=tag.AlbumTitle();
				QString song{"Now playing "};
				song.append(*title);
				if (album) song.append(" by ").append(*album);
				emit Print(song);
			}

			catch (const std::runtime_error &exception)
			{
				emit Print(exception.what());
			}

			break;
		case QMediaPlayer::StoppedState:
			disconnect(autoPlay);
			break;
		case QMediaPlayer::PausedState:
			break;
		}
	}

	void Player::MediaStatusChanged(QMediaPlayer::MediaStatus status)
	{
		if (status == QMediaPlayer::EndOfMedia)
		{
			if (Next() && loop)
			{
				autoPlay=connect(&player,&QMediaPlayer::mediaStatusChanged,this,[this](QMediaPlayer::MediaStatus status) {
					if (status == QMediaPlayer::LoadedMedia) Start();
				});
			}
		}
	}

	void Player::MediaError(QMediaPlayer::Error error,const QString &errorString)
	{
		Q_UNUSED(error)
		disconnect(autoPlay);
		emit Print(QString{"Failed to start: %1"}.arg(errorString));
	}

	bool Player::Next(QUrl source)
	{
		try
		{
			player.setSource(source.isEmpty() ? QUrl::fromLocalFile(sources.Unique()) : source);
		}

		catch (const std::runtime_error &exception)
		{
			emit Print(QString{"Could not queue next song: %1"}.arg(exception.what()));
			player.stop();
			return false;
		}

		catch (const std::out_of_range &exception)
		{
			emit Print(QString{"Invalid song list: %1"}.arg(exception.what()));
			return false;
		}

		return true;
	}

	int Player::TranslateVolume(qreal volume)
	{
		return static_cast<int>(std::clamp(volume,0.0,1.0)*100);
	}

	qreal Player::TranslateVolume(int volume)
	{
		return static_cast<qreal>(std::clamp(volume,0,100))/100;
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
			quint32 mask=0x7F000000;
			quint8 out=0;
			static_cast<void>(out); // silence spurious "unused variable" warnings
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
				{"APIC",Frame::Frame::APIC},
				{"TIT2",Frame::Frame::TIT2},
				{"TALB",Frame::Frame::TALB},
				{"TPE1",Frame::Frame::TPE1}
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
					case Frame::Frame::TIT2:
						TIT2=std::make_unique<Frame::TIT2>(file,frameHeader.Size());
						break;
					case Frame::Frame::TALB:
						TALB=std::make_unique<Frame::TALB>(file,frameHeader.Size());
						break;
					case Frame::Frame::TPE1:
						TPE1=std::make_unique<Frame::TPE1>(file,frameHeader.Size());
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

		Tag::Candidate<const QImage> Tag::AlbumCoverFront() const
		{
			if (APIC)
				return APIC->Picture();
			else
				return std::nullopt;
		}

		Tag::Candidate<const QString> Tag::Title() const
		{
			if (TIT2)
				return TIT2->Title();
			else
				return std::nullopt;
		}

		Tag::Candidate<const QString> Tag::AlbumTitle() const
		{
			if (TALB)
				return TALB->Title();
			else
				return std::nullopt;
		}

		Tag::Candidate<const QString> Tag::Artist() const
		{
			if (TPE1)
				return TPE1->Title();
			else
				return std::nullopt;
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

			TIT2::TIT2(QFile &file,quint32 size) : file(file), size(size)
			{
				ParseEncoding();
				ParseTitle();
			}

			void TIT2::ParseEncoding()
			{
				char data;
				if (!file.getChar(&data)) throw std::runtime_error("Invalid encoding in frame of mp3 file");
				unsigned short numeric=data;
				if (numeric > 3) throw std::out_of_range("Unrecognized encoding in frame of mp3 file");
				encoding=static_cast<Encoding>(numeric);
			}

			void TIT2::ParseTitle()
			{
				int chunkSize=encoding == Encoding::UTF_16 || encoding == Encoding::UTF_16BE ? 2 : 1;
				QByteArray terminator(chunkSize,'\0');
				QByteArray chunk;
				QByteArray data;
				do
				{
					chunk=file.read(chunkSize);
					if (chunk.size() < chunkSize) throw std::runtime_error("Invalid description in frame of mp3 file");
					if (chunk == QByteArray::fromHex("FFFE") || chunk == QByteArray::fromHex("FEFF")) continue;
					data.append(chunk);
				}
				while (chunk != terminator);
				title=data;
			}

			const QString& TIT2::Title() const
			{
				return title;
			}
		}
	}
}

namespace Viewer
{
	namespace ProfileImage
	{
		Remote::Remote(const QUrl &profileImageURL)
		{
			Network::Request::Send(profileImageURL,Network::Method::GET,[this](QNetworkReply *reply) {
				if (reply->error())
					emit Print(QString("Failed: %1").arg(reply->errorString()),"profile image retrieval");
				else
					emit Retrieved(std::make_shared<QImage>(QImage::fromData(reply->readAll())));
				this->deleteLater();
			});
		}

		Remote::operator QImage() const
		{
			return image;
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
		Network::Request::Send({Twitch::Endpoint(Twitch::ENDPOINT_USERS)},Network::Method::GET,[this](QNetworkReply* reply) {
			const char *OPERATION="request viewer information";

			try
			{
				switch (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
				{
				case 400:
					throw std::runtime_error("Invalid or missing ID or login parameter");
				case 401:
					throw std::runtime_error("Authentication failed");
				}

				if (reply->error()) throw std::runtime_error("Unknown error obtaining viewer information");

				const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
				if (!parsedJSON) throw std::runtime_error(std::string("Failed: "+parsedJSON.error.toStdString()));
				QJsonArray data=parsedJSON().object().value("data").toArray();
				if (data.size() < 1) throw std::runtime_error("Invalid user");
				QJsonObject details=data.at(0).toObject();
				emit Recognized({details.value("login").toString(),details.value("id").toString(),details.value("display_name").toString(),details.value("profile_image_url").toString(),details.value("description").toString()});
			}

			catch (const std::runtime_error &exception)
			{
				emit Print(exception.what(),OPERATION);
				emit Unrecognized();
			}

			this->deleteLater();
		},{
			{"login",username}
		},{
			{"Authorization",StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
			{"Client-ID",security.ClientID()}
		});
	};
}

namespace JSON
{
	void SignalPayload::Dispatch()
	{
		emit Deliver(payload);
		deleteLater();
	}
}
