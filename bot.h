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
#include "security.h"

enum class NativeCommandFlag
{
	AGENDA,
	CATEGORY,
	COMMANDS,
	EMOTE,
	FOLLOWAGE,
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
	using TagMap=std::unordered_map<QString,QString>;
	using BadgeMap=std::unordered_map<QString,QString>;
public:
	Bot(Security &security,QObject *parent=nullptr);
	void ToggleEmoteOnly();
	void EmoteOnly(bool enable,const QString &broadcasterID);
protected:
	std::unordered_map<QString,Command> commands;
	std::unordered_map<QString,NativeCommandFlag> nativeCommandFlags;
	std::unordered_map<QString,Viewer::Local> viewers;
	std::unordered_map<QString,BadgeMap> badgeIconURLs;
	QMediaPlayer *vibeKeeper;
	Volume::Fader *vibeFader;
	QMediaPlaylist vibeSources;
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
	ApplicationSetting settingUptimeHistory;
	static std::chrono::milliseconds launchTimestamp;
	bool LoadDynamicCommands();
	void LoadVibePlaylist();
	void LoadRoasts();
	void LoadBadgeIconURLs();
	void StartClocks();
	std::optional<TagMap> ParseTags(QStringList::const_iterator &messageSegment);
	std::optional<QStringList> ParseHostmask(QStringList::const_iterator &messageSegment);
	Viewer::Remote* TakeViewer(QStringList &hostmaskSegments);
	const std::tuple<std::vector<Media::Emote>,int> ParseEmotes(const TagMap &tags,const QString &message);
	void DownloadEmote(const Media::Emote &emote);
	const BadgeMap ParseBadges(const TagMap &tags);
	void DownloadBadgeIcon(const QString &badgeURL,const QString &badge);
	bool DispatchCommand(const QString name,const QString parameters,const Viewer::Local viewer,bool broadcaster);
	void DispatchArrival(Viewer::Local viewer);
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
	void ChatMessage(const QString &name,const QString &message,const std::vector<Media::Emote> &emotes,const QStringList &badgeIcons,const QColor color,bool action);
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
	void AnnounceHost(const QString &hostingChannel);
public slots:
	void ParseChatMessage(const QString &message);
	void ParseChatNotification(QStringList::const_iterator &messageSegment);
	void Ping();
	void Subscription(const QString &viewer);
	void Redemption(const QString &name,const QString &rewardTitle,const QString &message);
	void Raid(const QString &viewer,const unsigned int viewers);
	void Cheer(const QString &viewer,const unsigned int count,const QString &message);
};
