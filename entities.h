#pragma once

#include <QObject>
#include <QImage>
#include <QUrl>
#include <QMediaPlayer>
#include <QMediaMetaData>
#include <QAudioOutput>
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

namespace File
{
	class List
	{
	public:
		List() { }
		List(const QString &path,const QStringList &filter={});
		List(const QStringList &files);
		const QString File(const int index) const;
		const QString First();
		const QString Random();
		const QString Unique();
		int RandomIndex();
		const QStringList& operator()() const;
	protected:
		QStringList files;
		int currentIndex;
		void Shuffle();
		void Reshuffle();
	};
}

class Command
{
public:
	using Lookup=std::unordered_map<QString,Command>;
	using Entry=std::pair<const QString,Command>;
	Command() : Command({},{},CommandType::BLANK,false,true,{},{},{},{}) { }
	Command(const QString &name,const QString &description,const CommandType &type,bool protect=false) : Command(name,description,type,false,true,{},{},{},{},protect) { }
	Command(const QString &name,const QString &description,const CommandType &type,bool random,bool duplicates,const QString &path,const QStringList &filters,const QString &message,const QStringList &viewers,bool protect=false) : name(name), description(description), type(type), random(random), duplicates(duplicates), protect(protect), path(path), files(std::make_shared<File::List>(path,filters)), message(message), viewers(viewers), parent(nullptr) { }
	Command(const QString &name,Command* const parent);
	Command(const Command &command,const QString &message) : name(command.name), description(command.description), type(command.type), random(command.random), duplicates(command.duplicates), protect(command.protect), path(command.path), files(command.files), message(message), viewers(command.viewers), parent(nullptr) { }
	Command(const Command &other) : name(other.name), description(other.description), type(other.type), random(other.random), duplicates(other.duplicates), protect(other.protect), path(other.path), files(other.files), message(other.message), viewers(other.viewers), parent(nullptr) { }
	const QString& Name() const { return name; }
	const QString& Description() const { return description; }
	CommandType Type() const { return type; }
	bool Random() const { return random; }
	bool Duplicates() const { return duplicates; }
	bool Protected() const { return protect; }
	const QString& Path() const { return path; }
	const QString File();
	const QString& Message() const { return message; }
	const QStringList& Viewers() const { return viewers; }
	const Command* Parent() const { return parent; }
	const std::vector<Command*>& Children() const { return children; }
	static QStringList FileListFilters(const CommandType type);
protected:
	QString name;
	QString description;
	CommandType type;
	bool random;
	bool duplicates;
	bool protect;
	QString path;
	std::shared_ptr<File::List> files;
	QString message;
	QStringList viewers; //! the names of the viewers needed in chat to trigger the command
	Command *parent;
	std::vector<Command*> children;
};

namespace Music
{
	const QString SETTINGS_CATEGORY_VOLUME="Volume";

	class Player : public QObject
	{
		Q_OBJECT
	public:
		Player(bool loop,int initialVolume,QObject *parent=nullptr);
		void DuckVolume(bool duck);
		void Volume(int volume);
		void Volume(int targetVolume,std::chrono::seconds duration);
		void Stop();
		bool Playing() const;
		QString SongTitle() const;
		QString AlbumTitle() const;
		QString AlbumArtist() const;
		QImage AlbumCoverArt() const;
		QString Filename() const;
		void Sources(const File::List &sources);
		const File::List& Sources();
		ApplicationSetting& SuppressedVolume();
	protected:
		QMediaPlayer player;
		QAudioOutput output;
		File::List sources;
		QMetaObject::Connection autoPlay;
		bool loop;
		ApplicationSetting settingSuppressedVolume;
		QPropertyAnimation volumeAdjustment;
		static const char *ERROR_LOADING;
		static const char *OPERATION_LOADING;
		bool Next();
		int TranslateVolume(qreal volume);
		qreal TranslateVolume(int volume);
	signals:
		void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("music player")) const;
		void PlaylistLoaded();
	public slots:
		void Start();
	protected slots:
		void StateChanged(QMediaPlayer::PlaybackState state);
		void MediaStatusChanged(QMediaPlayer::MediaStatus status);
		void MediaError(QMediaPlayer::Error error,const QString &errorString);
		void VolumeMuted();
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
				APIC,
				TIT2,
				TALB,
				TPE1
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

			class TIT2
			{
			public:
				TIT2(QFile &file,quint32 size);
				const QString& Title() const;
			protected:
				QFile &file;
				quint32 size;
				Encoding encoding;
				QString title;
				void ParseEncoding();
				void ParseTitle();
			};
			using TALB=TIT2;
			using TPE1=TIT2;
		}

		class Tag
		{
		public:
			Tag(const QString &filename);
			~Tag();
			const QImage& AlbumCoverFront() const;
			const QString& Title() const;
			const QString& AlbumTitle() const;
			const QString& Artist() const;
		protected:
			QFile file;
			std::unique_ptr<Frame::APIC> APIC;
			std::unique_ptr<Frame::TIT2> TIT2;
			std::unique_ptr<Frame::TALB> TALB;
			std::unique_ptr<Frame::TPE1> TPE1;
			void Destroy();
		};
	}
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
		bool limited { false };
		bool subscribed { false };
		std::chrono::time_point<std::chrono::system_clock> commandTimestamp;
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
