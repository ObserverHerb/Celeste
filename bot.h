#pragma once

#include <QObject>
#include <QString>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QDateTime>
#include <QTimer>
#include <QNetworkAccessManager>
#include <unordered_map>
#include "entities.h"
#include "settings.h"

enum class NativeCommandFlag
{
	AGENDA,
	COMMANDS,
	PANIC,
	SHOUTOUT,
	SONG,
	TIMEZONE,
	UPTIME,
	VIBE,
	VOLUME
};

namespace Vibe
{
	const QString SETTINGS_CATEGORY_VOLUME="Volume";

	class Fader : public QObject
	{
		Q_OBJECT
	public:
		Fader(QMediaPlayer *player,const QString &arguments,QObject *parent=nullptr) : Fader(player,0,static_cast<std::chrono::seconds>(0),parent)
		{
			Parse(arguments);
		}
		Fader(QMediaPlayer *player,unsigned int volume,std::chrono::seconds duration,QObject *parent=nullptr) : QObject(parent),
			settingDefaultDuration(SETTINGS_CATEGORY_VOLUME,"DefaultFadeSeconds",5),
			player(player),
			initialVolume(static_cast<unsigned int>(player->volume())),
			targetVolume(std::clamp<int>(volume,0,100)),
			startTime(TimeConvert::Now()),
			duration(TimeConvert::Milliseconds(duration))
		{
			connect(this,&Fader::AdjustmentNeeded,this,&Fader::Adjust,Qt::QueuedConnection);
		}
		void Parse(const QString &text)
		{
			QStringList values=text.split(" ",StringConvert::Split::Behavior(StringConvert::Split::Behaviors::SKIP_EMPTY_PARTS));
			if (values.size() < 1) throw std::range_error("No volume specified");
			targetVolume=StringConvert::PositiveInteger(values[0]);
			duration=values.size() > 1 ? static_cast<std::chrono::seconds>(StringConvert::PositiveInteger(values[1])) : settingDefaultDuration;
		}
		void Start()
		{
			emit Print(QString("Adjusting volume from %1% to %2% over %3 seconds").arg(
				StringConvert::Integer(initialVolume),
				StringConvert::Integer(targetVolume),
				StringConvert::Integer(TimeConvert::Seconds(duration).count())
			));
			emit AdjustmentNeeded();
		}
		void Stop()
		{
			disconnect(this,&Fader::AdjustmentNeeded,this,&Fader::Adjust);
			deleteLater();
		}
		void Abort()
		{
			Stop();
			player->setVolume(initialVolume); // FIXME: volume doesn't return to initial value
		}
	protected:
		ApplicationSetting settingDefaultDuration;
		QMediaPlayer *player;
		unsigned int initialVolume;
		unsigned int targetVolume;
		std::chrono::milliseconds startTime;
		std::chrono::milliseconds duration;
		double Step(const double &secondsPassed)
		{
			// uses x^4 method described here: https://www.dr-lex.be/info-stuff/volumecontrols.html#ideal3
			// no need to find an ideal curve, this is a bot, not a DAW
			double totalSeconds=static_cast<double>(duration.count())/TimeConvert::Milliseconds(TimeConvert::OneSecond()).count();
			int volumeDifference=targetVolume-initialVolume;
			return std::pow(secondsPassed/totalSeconds,4)*(volumeDifference)+initialVolume;
		}
	signals:
		void AdjustmentNeeded();
		void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("volume fader"));
	public slots:
		void Adjust()
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
	};
}

class Bot : public QObject
{
	Q_OBJECT
	using TagMap=std::unordered_map<QString,QString>;
public:
	Bot(PrivateSetting &settingAdministrator,PrivateSetting &settingOAuthToken,PrivateSetting &settingClientID,QObject *parent=nullptr);
protected:
	std::unordered_map<QString,Command> commands;
	std::unordered_map<QString,NativeCommandFlag> nativeCommandFlags;
	std::unordered_map<QString,Viewer::Local> viewers;
	QNetworkAccessManager *downloadManager;
	QMediaPlayer *vibeKeeper;
	Vibe::Fader *vibeFader;
	QMediaPlaylist vibeSources;
	QMediaPlayer *roaster;
	QMediaPlaylist roastSources;
	QTimer inactivityClock;
	QTimer helpClock;
	QDateTime lastRaid;
	PrivateSetting &settingAdministrator;
	PrivateSetting &settingOAuthToken;
	PrivateSetting &settingClientID;
	ApplicationSetting settingInactivityCooldown;
	ApplicationSetting settingHelpCooldown;
	ApplicationSetting settingVibePlaylist;
	ApplicationSetting settingRoasts;
	ApplicationSetting settingPortraitVideo;
	ApplicationSetting settingArrivalSound;
	ApplicationSetting settingSubscriptionSound;
	ApplicationSetting settingRaidSound;
	ApplicationSetting settingRaidInterruptDuration;
	static std::chrono::milliseconds launchTimestamp;
	bool LoadDynamicCommands();
	void LoadVibePlaylist();
	void LoadRoasts();
	void StartClocks();
	bool Broadcaster(TagMap &tags);
	TagMap TakeTags(QStringList &messageSegments);
	std::optional<QStringList> TakeHostmask(QStringList &messageSegments);
	Viewer::Remote* TakeViewer(QStringList &hostmaskSegments);
	const std::vector<Media::Emote> ParseEmotes(const TagMap &tags,const QString &message);
	void DownloadEmote(const Media::Emote &emote);
	bool DispatchCommand(const QString name,const QString parameters,const Viewer::Local viewer,bool broadcaster);
	void DispatchArrival(Viewer::Local viewer);
	void DispatchVideo(Command command);
	void DispatchRandomVideo(Command command);
	void DispatchCommandList();
	void DispatchPanic();
	void DispatchShoutout(Command command);
	void DispatchUptime();
	void ToggleVibeKeeper();
	void AdjustVibeVolume(Command command);
signals:
	void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("bot core"));
	void DispatchCommand(Command command,const Viewer::Local viewer);
	void ChatMessage(const QString &name,const QString &message,const std::vector<Media::Emote> &emotes,const QColor color,bool action);
	void RefreshChat();
	void AnnounceArrival(const QString &name,QImage profileImage,const QString &audioPath);
	void PlayVideo(const QString &path);
	void PlayAudio(const QString &viewer,const QString &message,const QString &path);
	void SetAgenda(const QString &agenda);
	void ShowCommandList(std::vector<std::pair<QString,QString>> descriptions);
	void ShowCommand(const QString &name,const QString &description);
	void Panic(const QString &text);
	void Shoutout(const QString &name,const QString &description,const QImage &profileImage);
	void ShowCurrentSong(const QString &song,const QString &album,const QString &artist,const QImage coverArt);
	void ShowTimezone(const QString &timezone);
	void ShowUptime(std::chrono::hours hours,std::chrono::minutes minutes,std::chrono::seconds seconds);
	void ShowPortraitVideo(const QString &path);
public slots:
	void ParseChatMessage(const QString &message);
	void Ping();
};
