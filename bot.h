#pragma once

#include <QObject>
#include <QString>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QDateTime>
#include <QTimer>
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

class Bot : public QObject
{
	Q_OBJECT
	using TagMap=std::unordered_map<QString,QString>;
	using BadgeMap=std::unordered_map<QString,unsigned int>;
public:
	Bot(PrivateSetting &settingAdministrator,PrivateSetting &settingOAuthToken,PrivateSetting &settingClientID,QObject *parent=nullptr);
protected:
	std::unordered_map<QString,Command> commands;
	std::unordered_map<QString,NativeCommandFlag> nativeCommandFlags;
	std::unordered_map<QString,Viewer::Local> viewers;
	QMediaPlayer *vibeKeeper;
	Volume::Fader *vibeFader;
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
	ApplicationSetting settingCheerVideo;
	ApplicationSetting settingSubscriptionSound;
	ApplicationSetting settingRaidSound;
	ApplicationSetting settingRaidInterruptDuration;
	static std::chrono::milliseconds launchTimestamp;
	bool LoadDynamicCommands();
	void LoadVibePlaylist();
	void LoadRoasts();
	void StartClocks();
	TagMap TakeTags(QStringList &messageSegments);
	std::optional<QStringList> TakeHostmask(QStringList &messageSegments);
	Viewer::Remote* TakeViewer(QStringList &hostmaskSegments);
	const std::vector<Media::Emote> ParseEmotes(const TagMap &tags,const QString &message);
	void DownloadEmote(const Media::Emote &emote);
	const BadgeMap ParseBadges(const TagMap &tags);
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
	void ChatMessage(const QString &name,const QString &message,const std::vector<Media::Emote> &emotes,const QColor color,bool action);
	void RefreshChat();
	void AnnounceArrival(const QString &name,QImage profileImage,const QString &audioPath);
	void PlayVideo(const QString &path);
	void PlayAudio(const QString &name,const QString &message,const QString &path);
	void SetAgenda(const QString &agenda);
	void ShowCommandList(std::vector<std::pair<QString,QString>> descriptions);
	void ShowCommand(const QString &name,const QString &description);
	void Panic(const QString &text);
	void Shoutout(const QString &name,const QString &description,const QImage &profileImage);
	void ShowCurrentSong(const QString &song,const QString &album,const QString &artist,const QImage coverArt);
	void ShowTimezone(const QString &timezone);
	void ShowUptime(std::chrono::hours hours,std::chrono::minutes minutes,std::chrono::seconds seconds);
	void ShowPortraitVideo(const QString &path);
	void AnnounceSubscription(const QString &name,const QString &audioPath);
	void AnnounceRaid(const QString &viewer,const unsigned int viewers,const QString &audioPath);
	void AnnounceCheer(const QString &viewer,const unsigned int count,const QString &message,const QString &videoPath);
public slots:
	void ParseChatMessage(const QString &message);
	void Ping();
	void Subscription(const QString &viewer);
	void Raid(const QString &viewer,const unsigned int viewers);
	void Cheer(const QString &viewer,const unsigned int count,const QString &message);
};
