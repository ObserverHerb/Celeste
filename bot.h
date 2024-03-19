#pragma once

#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMediaPlayer>
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
	LIMIT,
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
	using NativeCommandFlagLookup=std::unordered_map<QString,NativeCommandFlag>;
	Bot(Music::Player &musicPlayer,Security &security,QObject *parent=nullptr);
	Bot(const Bot& other)=delete;
	Bot& operator=(const Bot &other)=delete;
	void ToggleEmoteOnly();
	void EmoteOnly(bool enable);
	void SaveViewerAttributes(bool reset);
	const Command::Lookup& Commands() const;
	const Command::Lookup& DeserializeCommands(const QJsonDocument &json);
	QJsonDocument LoadDynamicCommands();
	const File::List& DeserializeVibePlaylist(const QJsonDocument &json);
	QJsonDocument LoadVibePlaylist();
	ApplicationSetting& ArrivalSound();
	ApplicationSetting& PortraitVideo();
	ApplicationSetting& CheerVideo();
	ApplicationSetting& SubscriptionSound();
	ApplicationSetting& RaidSound();
	ApplicationSetting& InactivityCooldown();
	ApplicationSetting& HelpCooldown();
	ApplicationSetting& TextWallThreshold();
	ApplicationSetting& TextWallSound();
protected:
	using BadgeIconURLsLookup=std::unordered_map<QString,std::unordered_map<QString,QString>>;
	using CommandTypeLookup=std::unordered_map<QString,CommandType>;
	Command::Lookup commands;
	Command::Lookup redemptions;
	NativeCommandFlagLookup nativeCommandFlags;
	std::unordered_map<QString,Viewer::Attributes> viewers;
	Music::Player &vibeKeeper;
	Music::Player roaster;
	QTimer inactivityClock;
	QTimer helpClock;
	QDateTime lastRaid;
	Security &security;
	ApplicationSetting settingInactivityCooldown;
	ApplicationSetting settingHelpCooldown;
	ApplicationSetting settingTextWallThreshold;
	ApplicationSetting settingTextWallSound;
	ApplicationSetting settingRoasts;
	ApplicationSetting settingPortraitVideo;
	ApplicationSetting settingArrivalSound;
	ApplicationSetting settingCheerVideo;
	ApplicationSetting settingSubscriptionSound;
	ApplicationSetting settingRaidSound;
	ApplicationSetting settingRaidInterruptDuration;
	ApplicationSetting settingDeniedCommandVideo;
	ApplicationSetting settingCommandCooldown;
	ApplicationSetting settingUptimeHistory;
	ApplicationSetting settingCommandNameAgenda;
	ApplicationSetting settingCommandNameStreamCategory;
	ApplicationSetting settingCommandNameStreamTitle;
	ApplicationSetting settingCommandNameCommands;
	ApplicationSetting settingCommandNameEmote;
	ApplicationSetting settingCommandNameFollowage;
	ApplicationSetting settingCommandNameHTML;
	ApplicationSetting settingCommandNameLimit;
	ApplicationSetting settingCommandNamePanic;
	ApplicationSetting settingCommandNameShoutout;
	ApplicationSetting settingCommandNameSong;
	ApplicationSetting settingCommandNameTimezone;
	ApplicationSetting settingCommandNameUptime;
	ApplicationSetting settingCommandNameTotalTime;
	ApplicationSetting settingCommandNameVibe;
	ApplicationSetting settingCommandNameVibeVolume;
	static BadgeIconURLsLookup badgeIconURLs;
	static std::chrono::milliseconds launchTimestamp;
	static const CommandTypeLookup COMMAND_TYPE_LOOKUP;
	void DeclareCommand(const Command &&command,NativeCommandFlag flag);
	void StageRedemptionCommand(const QString &name,const QJsonObject &jsonObject);
	bool LoadViewerAttributes();
	void LoadRoasts();
	void LoadBadgeIconURLs();
	void StartClocks();
	std::optional<CommandType> ValidCommandType(const QString &type);
	void DownloadEmote(Chat::Emote &emote);
	std::optional<QString> DownloadBadgeIcon(const QString &badge,const QString &version);
	std::optional<QString> ParseCommand(QStringView &message);
	bool DispatchCommand(const QString name,const Chat::Message &chatMessage,const QString &login,bool html=true);
	void DispatchCommand(const Command &command,const QString &login);
	void DispatchArrival(const QString &login);
	void DispatchVideo(Command command);
	void DispatchCommandList();
	void DispatchFollowage(const Viewer::Local &viewer);
	void DispatchPanic(const QString &name);
	void DispatchShoutout(Command command);
	void DispatchUptime(bool total);
	void DispatchHelpText();
	void ToggleLimitViewer(const QString &target);
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
	void ShowCommandList(std::vector<std::tuple<QString,QStringList,QString>> descriptions);
	void ShowCommand(const QString &name,const QString &description);
	void Panic(const QString &text);
	void Pulse(const QString &trigger,const QString &command);
	void Shoutout(const QString &name,const QString &description,const QImage &profileImage);
	void ShowCurrentSong(const QString &song,const QString &album,const QString &artist,const QImage coverArt);
	void ShowCurrentSong(const QString &song,const QString &artist,const QImage coverArt);
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
	void AnnounceDeniedCommand(const QString &videoPath);
	void Welcomed(const QString &user);
public slots:
	void ParseChatMessage(const QString &prefix,const QString &source,const QStringList &parameters,const QString &message);
	void DispatchCommand(JSON::SignalPayload *response,const QString &name,const QString &login);
	void Ping();
	void Subscription(const QString &login,const QString &displayName);
	void Redemption(const QString &login,const QString &name,const QString &rewardTitle,const QString &message);
	void Raid(const QString &viewer,const unsigned int viewers);
	void Cheer(const QString &viewer,const unsigned int count,const QString &message);
	void SuppressMusic();
	void RestoreMusic();
	QJsonDocument SerializeCommands(const Command::Lookup &entries);
	bool SaveDynamicCommands(const QJsonDocument &json);
	QJsonDocument SerializeVibePlaylist(const File::List &songs);
	bool SaveVibePlaylist(const QJsonDocument &json);
};
