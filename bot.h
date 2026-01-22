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
	PLAYLIST,
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
	File::List DeserializeVibePlaylist(const QJsonDocument &json);
	QJsonDocument LoadVibePlaylist();
	const File::List& SetVibePlaylist(const File::List &files);
	Settings::Bot& Settings();
protected:
	using BadgeIconURLsLookup=std::unordered_map<QString,std::unordered_map<QString,QString>>;
	using CommandTypeLookup=std::unordered_map<QString,CommandType>;
	Command::Lookup commands;
	Command::Lookup redemptions;
	NativeCommandFlagLookup nativeCommandFlags;
	std::unordered_map<QString,Viewer::Attributes> viewers;
	std::unordered_map<QString,std::vector<QString>> userMessageCrossReference;
	Music::Player &vibeKeeper;
	Music::Player roaster;
	QTimer inactivityClock;
	QTimer helpClock;
	QTimer adScheduleClock;
	QTimer adStartingClock;
	QTimer adFinishedClock;
	QDateTime nextAd;
	int adBreakDuration;
	QDateTime lastRaid;
	Security &security;
	Settings::Bot settings;
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
	int ParseEmoteNamesAndDownloadImages(std::vector<Chat::Emote> &emotes,const QStringView &textWindow);
	void DownloadEmote(Chat::Emote &emote);
	std::optional<QString> DownloadBadgeIcon(const QString &badge,const QString &version);
	std::optional<QString> ParseCommandIfExists(QStringView &message);
	bool DispatchCommandViaChatMessage(const QString &name,const Chat::Message chatMessage,const QString &login);
	void DispatchCommandViaCommandObject(const Command &command,const QString &login);
	void DispatchArrival(const QString &login);
	void DispatchVideo(Command command);
	void DispatchCommandList();
	void DispatchFollowage(const Viewer::Local &viewer);
	void DispatchPanic(const QString &name);
	void DispatchShoutout(Command command);
	void DispatchShoutout(const QString &streamer);
	void DispatchUptime(bool total);
	void DispatchHelpText();
	void ToggleLimitViewer(const QString &target);
	void ToggleVibeKeeper();
	void AdjustVibeVolume(Command command);
	void ChangeVibePlaylist(const QString &name);
	void StreamTitle(const QString &title);
	void StreamCategory(const QString &category);
signals:
	void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("bot core"));
	void ChatMessage(std::shared_ptr<Chat::Message> message);
	void DeleteChatMessage(const QString &id);
	void RefreshChat();
	void AnnounceArrival(const QString &name,std::shared_ptr<QImage> profileImage,const QString &audioPath);
	void PlayVideo(const QString &path);
	void PlayAudio(const QString &name,const QString &message,const QString &path);
	void SetAgenda(const QString &agenda);
	void ShowCommandList(std::vector<std::tuple<QString,QStringList,QString>> descriptions);
	void ShowCommand(const QString &name,const QString &description);
	void Panic(const QString &text);
	void Pulse(const QString &trigger,const QString &command);
	void Shoutout(const QString &name,const QString &description,std::shared_ptr<QImage> profileImage);
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
	void AnnounceAdBreakStarting(const QString &videoPath);
	void AnnounceAdBreakFinished(const QString &videoPath);
	void Welcomed(const QString &user);
public slots:
	void ParseChatMessage(const QString &prefix,const QString &source,const QStringList &parameters,const QString &message);
	void ParseChatMessageDeletion(const QString &prefix);
	void DispatchCommandViaSubsystem(JSON::SignalPayload *response,const QString &name,const QString &login);
	void RequestAdSchedule();
	void Ping();
	void Subscription(const QString &login,const QString &displayName);
	void Redemption(const QString &login,const QString &name,const QString &rewardTitle,const QString &message);
	void Raid(const QString &viewer,const unsigned int viewers);
	void Cheer(const QString &viewer,const unsigned int count,const QString &message);
	void AdsStarting();
	void AdsFinished();
	void SuppressMusic();
	void RestoreMusic();
	QJsonDocument SerializeCommands(const Command::Lookup &entries);
	bool SaveDynamicCommands(const QJsonDocument &json);
	QJsonDocument SerializeVibePlaylist(const File::List &files);
	bool SaveVibePlaylist(const QJsonDocument &json);
};
