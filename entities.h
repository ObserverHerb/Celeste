#pragma once

#include <QObject>
#include <QImage>
#include <QUrl>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QPropertyAnimation>
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
	Command(const QString &name,const QString &description,const CommandType &type,bool random,const QString &path,const QString &message,bool protect=false) : name(name), description(description), type(type), random(random), path(path), message(message), protect(protect) { }
	Command(const Command &command,const QString &message) : name(command.name), description(command.description), type(command.type), random(command.random), path(command.path), message(message), protect(command.protect) { }
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
		void Volume(bool duck);
		void Volume(unsigned int volume);
		void Volume(unsigned int targetVolume,std::chrono::seconds duration);
		void Load(const QString &location);
		void Load(const QUrl &location);
		void Start();
		void Stop();
		bool Playing() const;
	protected:
		QMediaPlayer *player;
		QMediaPlaylist sources;
		//ApplicationSetting settingSuppressedVolume;
		QPropertyAnimation volumeAdjustment;
	signals:
		void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("music player"));
	protected slots:
		void StateChanged(QMediaPlayer::State state);
		void ConvertError(QMediaPlayer::Error error);
		void PlaylistLoaded();
	};
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
		void Recognized(Local viewer);
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
