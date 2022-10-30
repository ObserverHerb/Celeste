#pragma once

#include <QObject>
#include <QImage>
#include <QUrl>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QPropertyAnimation>
#include <QFile>
#include <memory>
#include "settings.h"
#include "security.h"

enum class CommandType
{
	BLANK,
	NATIVE,
	VIDEO,
	AUDIO,
	PULSAR
};

class Command
{
public:
	Command() : Command(QString(),QString(),CommandType::BLANK,false,QString(),QString()) { }
	Command(const QString &name,const QString &description,const CommandType &type,bool protect=false) : Command(name,description,type,false,QString(),QString(),protect) { }
	Command(const QString &name,const QString &description,const CommandType &type,bool random,const QString &path,const QString &message,bool protect=false) : name(name), description(description), type(type), random(random), protect(protect), path(path), message(message) { }
	Command(const Command &command,const QString &message) : name(command.name), description(command.description), type(command.type), random(command.random), protect(command.protect), path(command.path), message(message) { }
	const QString& Name() const { return name; }
	const QString& Description() const { return description; }
	CommandType Type() const { return type; }
	bool Random() const { return random; }
	bool Protected() const { return protect; }
	const QString& Path() const { return path; }
	const QString& Message() const { return message; }
protected:
	QString name;
	QString description;
	CommandType type;
	bool random;
	bool protect;
	QString path;
	QString message;
};

namespace Music
{
	const QString SETTINGS_CATEGORY_VOLUME="Volume";

	class Player : public QObject
	{
		Q_OBJECT
	public:
		Player(QObject *parent);
		void DuckVolume(bool duck);
		void Volume(unsigned int volume);
		void Volume(unsigned int targetVolume,std::chrono::seconds duration);
		void Load(const QString &location);
		void Load(const QUrl &location);
		void Start();
		void Stop();
		bool Playing() const;
		QString SongTitle() const;
		QString AlbumTitle() const;
		QString AlbumArtist() const;
		QImage AlbumCoverArt() const;
		QString Filename() const;
	protected:
		QMediaPlayer *player;
		QMediaPlaylist sources;
		ApplicationSetting settingSuppressedVolume;
		QPropertyAnimation volumeAdjustment;
	signals:
		void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("music player")) const;
	protected slots:
		void StateChanged(QMediaPlayer::State state);
		void ConvertError(QMediaPlayer::Error error);
		void PlaylistLoaded();
	};

	namespace ID3
	{
		quint32 SyncSafe(const char *value);

		class Header
		{
		public:
			Header(QFile &file);
			quint32 Size() const;
		protected:
			QFile &file;
			unsigned short versionMajor;
			unsigned short versionMinor;
			bool unsynchronization;
			bool extendedHeader;
			quint32 size;
			void ParseIdentifier();
			void ParseVersion();
			void ParseFlags();
			void ParseSize();
		};

		namespace Frame
		{
			enum class Frame
			{
				APIC
			};

			enum class PictureType
			{
				OTHER=0,
				FILE_ICON,
				OTHER_FILE_ICON,
				COVER_FRONT,
				COVER_BACK,
				LEAFLET_PAGE,
				CD_LABEL,
				LEAD_ARTIST,
				ARTIST,
				CONDUCTOR,
				BAND,
				COMPOSER,
				LYRICIST,
				RECORDING_LOCATION,
				DURING_RECORDING,
				DURING_PERFORMANCE,
				VIDEO_SCREEN_CAPTURE,
				A_BRIGHT_COLORED_FISH,
				ILLUSTRATION,
				BAND_LOGO,
				STUDIO_LOGO
			};

			enum class Encoding
			{
				ISO_8859_1=0,
				UTF_16,
				UTF_16BE,
				UTF_8
			};

			class Header
			{
			public:
				Header(QFile &file);
				QByteArray ID() const;
				quint32 Size() const;
			protected:
				QFile &file;
				QByteArray id;
				quint32 size;
				void ParseID();
				void ParseSize();
			};

			class APIC
			{
			public:
				APIC(QFile &file,quint32 size);
				const QImage& Picture() const;
			protected:
				QFile &file;
				quint32 size;
				Encoding encoding;
				QByteArray MIMEType;
				PictureType pictureType;
				QByteArray description;
				QImage picture;
				void ParseEncoding();
				void ParseMIMEType();
				void ParsePictureType();
				void ParseDescription();
				void ParsePictureData();
				quint32 DataSize();
			};
		}

		class Tag
		{
		public:
			Tag(const QString &filename);
			~Tag();
			const QImage& AlbumCoverFront() const;
		protected:
			QFile file;
			std::unique_ptr<Frame::APIC> APIC;
			void Destroy();
		};
	}
}

namespace File
{
	class List
	{
	public:
		List(const QString &path);
		List(const QStringList &files);
		const QString File(const int index);
		const QString First();
		const QString Random();
		int RandomIndex();
	protected:
		QStringList files;
	};
}

namespace Viewer
{
	namespace ProfileImage
	{
		class Remote : public QObject
		{
			Q_OBJECT
		public:
			Remote(const QUrl &profileImageURL);
			operator QImage() const;
		protected:
			QImage image;
		signals:
			void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("profile image retrieval"));
			void Retrieved(const QImage &image);
		};
	}

	class Local
	{
	public:
		Local(const QString &name,const QString &id,const QString &displayName,QUrl profileImageURL,const QString &description) : name(name), id(id), displayName(displayName), profileImageURL(profileImageURL), description(description) { }
		const QString& Name() const;
		const QString& ID() const;
		const QString& DisplayName() const;
		ProfileImage::Remote* ProfileImage() const;
		const QString& Description() const;
	protected:
		QString name;
		QString id;
		QString displayName;
		QUrl profileImageURL;
		QString description;
	};

	class Remote : public QObject
	{
		Q_OBJECT
	public:
		Remote(Security &security,const QString &username);
	protected:
		QString name;
		void DownloadProfileImage(const QString &url);
	signals:
		void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("viewer retrieval"));
		void Recognized(const Viewer::Local &viewer);
	};

	struct Attributes
	{
		bool commands { true };
		bool welcomed { false };
		bool bot { false };
	};
}

namespace Chat
{
	struct Emote
	{
		QString name;
		QString id;
		QString path;
		unsigned int start { 0 };
		unsigned int end { 0 };
		bool operator<(const Emote &other) const { return start < other.start; }
	};

	struct Message
	{
		QString sender;
		QString text;
		QColor color;
		QStringList badges;
		std::vector<Chat::Emote> emotes;
		bool action { false };
		bool broadcaster { false };
		bool moderator { false };
		bool Privileged() const { return broadcaster || moderator; }
	};
}
