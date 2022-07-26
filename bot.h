#pragma once

#include <QObject>
#include <QString>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QDateTime>
#include <QTimer>
#include <unordered_map>
#include <unordered_set>
#include "entities.h"
#include "settings.h"
#include "security.h"

enum class NativeCommandFlag
{
	AGENDA,
	CATEGORY,
	COMMANDS,
	EMOTE,
	FOLLOWAGE,
	HTML,
	PANIC,
	SHOUTOUT,
	SONG,
	TIMEZONE,
	TITLE,
	TOTAL_TIME,
	UPTIME,
	VIBE,
	VOLUME
};

class Bot : public QObject
{
	Q_OBJECT
public:
	Bot(Security &security,QObject *parent=nullptr);
	Bot(const Bot& other)=delete;
	Bot& operator=(const Bot &other)=delete;
	void ToggleEmoteOnly();
	void EmoteOnly(bool enable,const QString &broadcasterID);
protected:
	std::unordered_map<QString,Command> commands;
	std::unordered_map<QString,NativeCommandFlag> nativeCommandFlags;
	std::unordered_set<QString> viewers;
	Music::Player *vibeKeeper;
	QMediaPlayer *roaster;
	QMediaPlaylist roastSources;
	QTimer inactivityClock;
	QTimer helpClock;
	QDateTime lastRaid;
	Security &security;
	ApplicationSetting settingInactivityCooldown;
	ApplicationSetting settingHelpCooldown;
	ApplicationSetting settingVibePlaylist;
	ApplicationSetting settingTextWallThreshold;
	ApplicationSetting settingTextWallSound;
	ApplicationSetting settingRoasts;
	ApplicationSetting settingPortraitVideo;
	ApplicationSetting settingArrivalSound;
	ApplicationSetting settingCheerVideo;
	ApplicationSetting settingSubscriptionSound;
	ApplicationSetting settingRaidSound;
	ApplicationSetting settingRaidInterruptDuration;
	ApplicationSetting settingHostSound;
	ApplicationSetting settingUptimeHistory;
	static std::unordered_map<QString,std::unordered_map<QString,QString>> badgeIconURLs;
	static std::chrono::milliseconds launchTimestamp;
	void DeclareCommand(const Command &&command,NativeCommandFlag flag);
	bool LoadDynamicCommands();
	void LoadRoasts();
	void LoadBadgeIconURLs();
	void StartClocks();
	void DownloadEmote(Chat::Emote &emote);
	std::optional<QString> DownloadBadgeIcon(const QString &badge,const QString &version);
	bool DispatchCommand(const QString name,const Chat::Message &chatMessage,const QString &login);
	void DispatchArrival(const QString &login);
	bool DispatchChatNotification(const QStringView &message);
	void DispatchVideo(Command command);
	void DispatchRandomVideo(Command command);
	void DispatchCommandList();
	void DispatchFollowage(const QString &name);
	void DispatchPanic(const QString &name);
	void DispatchShoutout(Command command);
	void DispatchUptime(bool total);
	void ToggleVibeKeeper();
	void AdjustVibeVolume(Command command);
	void StreamTitle(const QString &title);
	void StreamCategory(const QString &category);
signals:
	void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("bot core"));
	void ChatMessage(const Chat::Message &message);
	void RefreshChat();
	void AnnounceArrival(const QString &name,QImage profileImage,const QString &audioPath);
	void PlayVideo(const QString &path);
	void PlayAudio(const QString &name,const QString &message,const QString &path);
	void SetAgenda(const QString &agenda);
	void ShowCommandList(std::vector<std::pair<QString,QString>> descriptions);
	void ShowCommand(const QString &name,const QString &description);
	void Panic(const QString &text);
	void Pulse(const QString &trigger);
	void Shoutout(const QString &name,const QString &description,const QImage &profileImage);
	void ShowCurrentSong(const QString &song,const QString &album,const QString &artist,const QImage coverArt);
	void ShowFollowage(const QString &name,std::chrono::years years,std::chrono::months months,std::chrono::days days);
	void ShowTimezone(const QString &timezone);
	void ShowTotalTime(std::chrono::hours hours,std::chrono::minutes minutes,std::chrono::seconds seconds);
	void ShowUptime(std::chrono::hours hours,std::chrono::minutes minutes,std::chrono::seconds seconds);
	void ShowPortraitVideo(const QString &path);
	void AnnounceRedemption(const QString &name,const QString &rewardTitle,const QString &message);
	void AnnounceSubscription(const QString &name,const QString &audioPath);
	void AnnounceRaid(const QString &viewer,const unsigned int viewers,const QString &audioPath);
	void AnnounceCheer(const QString &viewer,const unsigned int count,const QString &message,const QString &videoPath);
	void AnnounceTextWall(const QString &message,const QString &audioPath);
	void AnnounceHost(const QString &hostingChannel,const QString &audioPath);
public slots:
	void ParseChatMessage(const QString &prefix,const QString &source,const QStringList &parameters,const QString &message);
	void Ping();
	void Subscription(const QString &viewer);
	void Redemption(const QString &name,const QString &rewardTitle,const QString &message);
	void Raid(const QString &viewer,const unsigned int viewers);
	void Cheer(const QString &viewer,const unsigned int count,const QString &message);
	void SuppressMusic();
	void RestoreMusic();
};
