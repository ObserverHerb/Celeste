#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QApplication>
#include <QTimeZone>
#include <QNetworkReply>
#include "bot.h"
#include "globals.h"

const char *COMMANDS_LIST_FILENAME="commands.json";
const char *NETWORK_HEADER_AUTHORIZATION="Authorization";
const char *NETWORK_HEADER_CLIENT_ID="Client-Id";
const char *QUERY_PARAMETER_BROADCASTER_ID="broadcaster_id";
const char *QUERY_PARAMETER_MODERATOR_ID="moderator_id";
const char *JSON_KEY_COMMAND_NAME="command";
const char *JSON_KEY_COMMAND_ALIASES="aliases";
const char *JSON_KEY_COMMAND_DESCRIPTION="description";
const char *JSON_KEY_COMMAND_TYPE="type";
const char *JSON_KEY_COMMAND_RANDOM_PATH="random";
const char *JSON_KEY_COMMAND_PATH="path";
const char *JSON_KEY_COMMAND_MESSAGE="message";
const char *JSON_KEY_DATA="data";
const char *SETTINGS_CATEGORY_EVENTS="Events";
const char *SETTINGS_CATEGORY_VIBE="Vibe";
const char *SETTINGS_CATEGORY_COMMANDS="Commands";
const char *TWITCH_API_ENDPOINT_EMOTE_URL="https://static-cdn.jtvnw.net/emoticons/v1/%1/1.0";
const char *TWITCH_API_ENDPOINT_CHAT_SETTINGS="https://api.twitch.tv/helix/chat/settings";
const char *TWITCH_API_ENDPOINT_STREAM_INFORMATION="https://api.twitch.tv/helix/streams";
const char *TWITCH_API_ENDPOINT_CHANNEL_INFORMATION="https://api.twitch.tv/helix/channels";
const char *TWITCH_API_ENDPOINT_GAME_INFORMATION="https://api.twitch.tv/helix/games";
const char *TWITCH_API_ENDPOING_USER_FOLLOWS="https://api.twitch.tv/helix/users/follows";
const char *TWITCH_API_ENDPOINT_BADGES="https://api.twitch.tv/helix/chat/badges/global";
const char16_t *TWITCH_SYSTEM_ACCOUNT_NAME=u"jtv";
const char16_t *CHAT_BADGE_BROADCASTER=u"broadcaster";
const char16_t *CHAT_BADGE_MODERATOR=u"moderator";
const char16_t *CHAT_TAG_DISPLAY_NAME=u"display-name";
const char16_t *CHAT_TAG_BADGES=u"badges";
const char16_t *CHAT_TAG_COLOR=u"color";
const char16_t *CHAT_TAG_EMOTES=u"emotes";

const std::unordered_map<QString,CommandType> COMMAND_TYPES={
	{"native",CommandType::NATIVE},
	{"video",CommandType::VIDEO},
	{"announce",CommandType::AUDIO}
};

std::unordered_map<QString,std::unordered_map<QString,QString>> Bot::badgeIconURLs;
std::chrono::milliseconds Bot::launchTimestamp=TimeConvert::Now();

Bot::Bot(Security &security,QObject *parent) : QObject(parent),
	vibeKeeper(new QMediaPlayer(this)),
	vibeFader(nullptr),
	roaster(new QMediaPlayer(this)),
	security(security),
	settingInactivityCooldown(SETTINGS_CATEGORY_EVENTS,"InactivityCooldown",1800000),
	settingHelpCooldown(SETTINGS_CATEGORY_EVENTS,"HelpCooldown",300000),
	settingVibePlaylist(SETTINGS_CATEGORY_VIBE,"Playlist"),
	settingTextWallThreshold(SETTINGS_CATEGORY_EVENTS,"TextWallThreshold",400),
	settingTextWallSound(SETTINGS_CATEGORY_EVENTS,"TextWallSound"),
	settingRoasts(SETTINGS_CATEGORY_EVENTS,"Roasts"),
	settingPortraitVideo(SETTINGS_CATEGORY_EVENTS,"Portrait"),
	settingArrivalSound(SETTINGS_CATEGORY_EVENTS,"Arrival"),
	settingCheerVideo(SETTINGS_CATEGORY_EVENTS,"Cheer"),
	settingSubscriptionSound(SETTINGS_CATEGORY_EVENTS,"Subscription"),
	settingRaidSound(SETTINGS_CATEGORY_EVENTS,"Raid"),
	settingRaidInterruptDuration(SETTINGS_CATEGORY_EVENTS,"RaidInterruptDelay",60000),
	settingHostSound(SETTINGS_CATEGORY_EVENTS,"Host"),
	settingUptimeHistory(SETTINGS_CATEGORY_COMMANDS,"UptimeHistory",0)
{
	Command agendaCommand("agenda","Set the agenda of the stream, displayed in the header of the chat window",CommandType::NATIVE,true);
	Command categoryCommand("category","Change the stream category",CommandType::NATIVE,true);
	Command commandsCommand("commands","List all of the commands Celeste recognizes",CommandType::NATIVE,false);
	Command emoteCommand("emote","Toggle emote only mode in chat",CommandType::NATIVE,true);
	Command followageCommand("followage","Show how long a user has followed the broadcaster",CommandType::NATIVE,false);
	Command panicCommand("panic","Crash Celeste",CommandType::NATIVE,true);
	Command shoutOutCommand("so","Call attention to another streamer's channel",CommandType::NATIVE,false);
	Command songCommand("song","Show the title, album, and artist of the song that is currently playing",CommandType::NATIVE,false);
	Command timezoneCommand("timezone","Display the timezone of the system the bot is running on",CommandType::NATIVE,false);
	Command titleCommand("title","Change the stream title",CommandType::NATIVE,true);
	Command uptimeCommand("uptime","Show how long the bot has been connected",CommandType::NATIVE,false);
	Command totalTimeCommand("totaltime","Show how many total hours stream has ever been live",CommandType::NATIVE,false);
	Command vibeCommand("vibe","Start the playlist of music for the stream",CommandType::NATIVE,true);
	Command volumeCommand("volume","Adjust the volume of the vibe keeper",CommandType::NATIVE,true);
	commands.insert({
		{agendaCommand.Name(),agendaCommand},
		{categoryCommand.Name(),categoryCommand},
		{commandsCommand.Name(),commandsCommand},
		{emoteCommand.Name(),emoteCommand},
		{followageCommand.Name(),followageCommand},
		{panicCommand.Name(),panicCommand},
		{shoutOutCommand.Name(),shoutOutCommand},
		{songCommand.Name(),songCommand},
		{timezoneCommand.Name(),timezoneCommand},
		{titleCommand.Name(),titleCommand},
		{totalTimeCommand.Name(),totalTimeCommand},
		{uptimeCommand.Name(),uptimeCommand},
		{vibeCommand.Name(),vibeCommand},
		{volumeCommand.Name(),volumeCommand}
	});
	nativeCommandFlags.insert({
		{agendaCommand.Name(),NativeCommandFlag::AGENDA},
		{categoryCommand.Name(),NativeCommandFlag::CATEGORY},
		{commandsCommand.Name(),NativeCommandFlag::COMMANDS},
		{emoteCommand.Name(),NativeCommandFlag::EMOTE},
		{followageCommand.Name(),NativeCommandFlag::FOLLOWAGE},
		{panicCommand.Name(),NativeCommandFlag::PANIC},
		{shoutOutCommand.Name(),NativeCommandFlag::SHOUTOUT},
		{songCommand.Name(),NativeCommandFlag::SONG},
		{timezoneCommand.Name(),NativeCommandFlag::TIMEZONE},
		{titleCommand.Name(),NativeCommandFlag::TITLE},
		{totalTimeCommand.Name(),NativeCommandFlag::TOTAL_TIME},
		{uptimeCommand.Name(),NativeCommandFlag::UPTIME},
		{vibeCommand.Name(),NativeCommandFlag::VIBE},
		{volumeCommand.Name(),NativeCommandFlag::VOLUME}
	});
	if (!LoadDynamicCommands()) emit Print("Failed to load commands"); // do this after creating native commands or aliases to native commands won't work

	if (settingVibePlaylist) LoadVibePlaylist();
	if (settingRoasts) LoadRoasts();
	LoadBadgeIconURLs();
	StartClocks();

	lastRaid=QDateTime::currentDateTime().addMSecs(static_cast<qint64>(0)-static_cast<qint64>(settingRaidInterruptDuration));
	vibeKeeper->setVolume(0);
}

bool Bot::LoadDynamicCommands()
{
	QFile commandListFile(Filesystem::DataPath().filePath(COMMANDS_LIST_FILENAME));
	if (!commandListFile.open(QIODevice::ReadWrite))
	{
		emit Print(QString("Failed to open command list file: %1").arg(commandListFile.fileName()));
		return false;
	}

	QJsonParseError jsonError;
	QByteArray data=commandListFile.readAll();
	if (data.isEmpty()) data="[]";
	QJsonDocument json=QJsonDocument::fromJson(data,&jsonError);
	if (json.isNull())
	{
		emit Print(jsonError.errorString());
		return false;
	}

	const QJsonArray objects=json.array();
	for (const QJsonValue &jsonValue : objects)
	{
		QJsonObject jsonObject=jsonValue.toObject();
		const QString name=jsonObject.value(JSON_KEY_COMMAND_NAME).toString();
		if (!commands.contains(name))
		{
			commands[name]={
				name,
				jsonObject.value(JSON_KEY_COMMAND_DESCRIPTION).toString(),
				COMMAND_TYPES.at(jsonObject.value(JSON_KEY_COMMAND_TYPE).toString()),
				jsonObject.contains(JSON_KEY_COMMAND_RANDOM_PATH) ? jsonObject.value(JSON_KEY_COMMAND_RANDOM_PATH).toBool() : false,
				jsonObject.value(JSON_KEY_COMMAND_PATH).toString(),
				jsonObject.contains(JSON_KEY_COMMAND_MESSAGE) ? jsonObject.value(JSON_KEY_COMMAND_MESSAGE).toString() : QString()
			};
		}
		if (jsonObject.contains(JSON_KEY_COMMAND_ALIASES))
		{
			const QJsonArray aliases=jsonObject.value(JSON_KEY_COMMAND_ALIASES).toArray();
			for (const QJsonValue &jsonValue : aliases)
			{
				const QString alias=jsonValue.toString();
				commands[alias]={
					alias,
					commands.at(name).Description(),
					commands.at(name).Type(),
					commands.at(name).Random(),
					commands.at(name).Path(),
					commands.at(name).Message(),
				};
				if (commands.at(alias).Type() == CommandType::NATIVE) nativeCommandFlags.insert({alias,nativeCommandFlags.at(name)});
			}
		}
	}

	return true;
}

void Bot::LoadVibePlaylist()
{
	connect(&vibeSources,&QMediaPlaylist::loadFailed,this,[this]() {
		emit Print(QString("Failed to load vibe playlist: %1").arg(vibeSources.errorString()));
	});
	connect(&vibeSources,&QMediaPlaylist::loaded,this,[this]() {
		vibeSources.shuffle();
		vibeSources.setCurrentIndex(Random::Bounded(0,vibeSources.mediaCount()));
		vibeKeeper->setPlaylist(&vibeSources);
		emit Print("Vibe playlist loaded!");
	});
	connect(vibeKeeper,QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),this,[this](QMediaPlayer::Error error) {
		emit Print(QString("Vibe Keeper failed to start: %1").arg(vibeKeeper->errorString()));
	});
	connect(vibeKeeper,&QMediaPlayer::stateChanged,this,[this](QMediaPlayer::State state) {
		if (state == QMediaPlayer::PlayingState) emit Print(QString("Now playing %1 by %2").arg(vibeKeeper->metaData("Title").toString(),vibeKeeper->metaData("AlbumArtist").toString()));
	});
	vibeSources.load(QUrl::fromLocalFile(settingVibePlaylist));
}

void Bot::LoadRoasts()
{
	connect(&roastSources,&QMediaPlaylist::loadFailed,this,[this]() {
		emit Print(QString("Failed to load roasts: %1").arg(roastSources.errorString()));
	});
	connect(&roastSources,&QMediaPlaylist::loaded,this,[this]() {
		roastSources.shuffle();
		roastSources.setCurrentIndex(Random::Bounded(0,roastSources.mediaCount()));
		roaster->setPlaylist(&roastSources);
		connect(&inactivityClock,&QTimer::timeout,roaster,&QMediaPlayer::play);
		emit Print("Roasts loaded!");
	});
	roastSources.load(QUrl::fromLocalFile(settingRoasts));
	connect(roaster,QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),this,[this](QMediaPlayer::Error error) {
		emit Print(QString("Roaster failed to start: %1").arg(vibeKeeper->errorString()));
	});
	connect(roaster,&QMediaPlayer::mediaStatusChanged,this,[this](QMediaPlayer::MediaStatus status) {
		if (status == QMediaPlayer::EndOfMedia)
		{
			roaster->stop();
			roastSources.setCurrentIndex(Random::Bounded(0,roastSources.mediaCount()));
		}
	});
}

void Bot::LoadBadgeIconURLs()
{
	Network::Request({TWITCH_API_ENDPOINT_BADGES},Network::Method::GET,[](QNetworkReply *reply) {
		const char *JSON_KEY_ID="id";
		const char *JSON_KEY_SET_ID="set_id";
		const char *JSON_KEY_VERSIONS="versions";
		const char *JSON_KEY_IMAGE_URL="image_url_1x";

		const QJsonDocument json=QJsonDocument::fromJson(reply->readAll());
		if (json.isNull()) return;
		const QJsonObject object=json.object();
		if (object.isEmpty() || !object.contains(JSON_KEY_DATA)) return;
		const QJsonArray sets=object.value(JSON_KEY_DATA).toArray();
		for (const QJsonValue &set : sets)
		{
			const QJsonObject object=set.toObject();
			if (!object.contains(JSON_KEY_SET_ID) || !object.contains(JSON_KEY_VERSIONS)) continue;
			const QString setID=object.value(JSON_KEY_SET_ID).toString();
			const QJsonArray versions=object.value(JSON_KEY_VERSIONS).toArray();
			for (const QJsonValue &version : versions)
			{
				const QJsonObject object=version.toObject();
				if (!object.contains(JSON_KEY_ID) || !object.contains(JSON_KEY_IMAGE_URL)) continue;
				const QString setVersion=object.value(JSON_KEY_ID).toVariant().toString();
				const QString versionURL=object.value(JSON_KEY_IMAGE_URL).toString();
				badgeIconURLs[setID][setVersion]=versionURL;
			}
		}
	},{},{
		{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()}
	});
}

void Bot::StartClocks()
{
	inactivityClock.setInterval(TimeConvert::Interval(std::chrono::milliseconds(settingInactivityCooldown)));
	if (settingPortraitVideo)
	{
		connect(&inactivityClock,&QTimer::timeout,this,[this]() {
			emit PlayVideo(settingPortraitVideo);
		});
	}
	inactivityClock.start();

	helpClock.setInterval(TimeConvert::Interval(std::chrono::milliseconds(settingHelpCooldown)));
	connect(&helpClock,&QTimer::timeout,this,[this]() {
		std::unordered_map<QString,Command>::iterator candidate=std::next(std::begin(commands),Random::Bounded(commands));
		emit ShowCommand(candidate->second.Name(),candidate->second.Description());
	});
	helpClock.start();
}

void Bot::Ping()
{
	if (settingPortraitVideo)
		emit ShowPortraitVideo(settingPortraitVideo);
	else
		emit Print("Letting Twitch server know we're still here...");
}

void Bot::Redemption(const QString &name,const QString &rewardTitle,const QString &message)
{
	if (rewardTitle == "Crash Celeste")
	{
		DispatchPanic(name);
		vibeKeeper->pause();
		return;
	}
	emit AnnounceRedemption(name,rewardTitle,message);
}

void Bot::Subscription(const QString &viewer)
{
	if (static_cast<QString>(settingSubscriptionSound).isEmpty())
	{
		emit Print("No audio path set for subscriptions","announce subscription");
		return;
	}
	emit AnnounceSubscription(viewer,settingSubscriptionSound);
}

void Bot::Raid(const QString &viewer,const unsigned int viewers)
{
	lastRaid=QDateTime::currentDateTime();
	emit AnnounceRaid(viewer,viewers,settingRaidSound);
}

void Bot::Cheer(const QString &viewer,const unsigned int count,const QString &message)
{
	if (static_cast<QString>(settingCheerVideo).isEmpty())
	{
		emit Print("No video path set for cheers","announce cheer");
		return;
	}
	emit AnnounceCheer(viewer,count,message,settingCheerVideo);
}

void Bot::DispatchArrival(const QString &login)
{
	if (static_cast<QString>(settingArrivalSound).isEmpty())
	{
		emit Print("No audio path set for arrivals","announce arrival");
		return;
	}

	Viewer::Remote *viewer=new Viewer::Remote(security,login);
	connect(viewer,&Viewer::Remote::Print,this,&Bot::Print);
	connect(viewer,&Viewer::Remote::Recognized,viewer,[this](Viewer::Local viewer) {
		if (security.Administrator() != viewer.Name() && QDateTime::currentDateTime().toMSecsSinceEpoch()-lastRaid.toMSecsSinceEpoch() > static_cast<qint64>(settingRaidInterruptDuration))
		{
			Viewer::ProfileImage::Remote *profileImage=viewer.ProfileImage();
			connect(profileImage,&Viewer::ProfileImage::Remote::Retrieved,profileImage,[this,viewer](const QImage &profileImage) {
				emit AnnounceArrival(viewer.DisplayName(),profileImage,File::List(settingArrivalSound).Random());
			});
			connect(profileImage,&Viewer::ProfileImage::Remote::Print,this,&Bot::Print);
		}
	});
}

void Bot::ParseChatMessage(const QString &message)
{
	if (message.size() < 1) return;
	std::optional<QStringView> window;

	QStringView remainingText(message);
	QStringView sender;
	QStringView login;
	QColor color;
	QStringList badgeIconPaths;
	bool broadcaster=false;
	bool moderator=false;
	std::vector<Chat::Emote> emotes;

	// Twitch appears to use ":" as a delimiter and every message is divided into three sections
	// if a section is missing, it will still have its delimiters
	if (remainingText.at(0) != ':')
	{
		// tag section is always first and not preceeded by a colon
		// since we now know the section exists, consume up to the SPACE,
		// NOT COLON because there are not spaces in the tags but there are
		// colons in the emotes tag, and apparently there is always
		// a space before each section after the first
		window=StringView::Take(remainingText,' ');
		if (window)
		{
			std::unordered_map<QStringView,QStringView> tags;
			while (!window->isEmpty())
			{
				std::optional<QStringView> pair=StringView::Take(*window,';');
				if (!pair) continue; // skip malformated badges rather than making a visible fuss
				std::optional<QStringView> key=StringView::Take(*pair,'=');
				if (!key) continue; // same as above
				std::optional<QStringView> value=StringView::Last(*pair,'=');
				if (!value) continue; // I'll rely on "missing" to represent an empty value
				tags[*key]=*value;
			}
			if (tags.contains(CHAT_TAG_DISPLAY_NAME)) sender=tags.at(CHAT_TAG_DISPLAY_NAME);
			if (tags.contains(CHAT_TAG_COLOR)) color=tags.at(CHAT_TAG_COLOR).toString();

			// badges
			if (tags.contains(CHAT_TAG_BADGES))
			{
				std::unordered_map<QStringView,QStringView> badges;
				QStringView &versions=tags.at(CHAT_TAG_BADGES);
				while (!versions.isEmpty())
				{
					std::optional<QStringView> pair=StringView::Take(versions,',');
					if (!pair) continue;
					std::optional<QStringView> name=StringView::Take(*pair,'/');
					if (!name) continue;
					std::optional<QStringView> version=StringView::Last(*pair,'/');
					if (!version) continue; // a badge must have a version
					badges.insert({*name,*version});
					std::optional<QString> badgeIconPath=DownloadBadgeIcon(name->toString(),version->toString());
					if (!badgeIconPath) continue;
					badgeIconPaths.append(*badgeIconPath);
				}
				if (badges.contains(CHAT_BADGE_BROADCASTER) && badges.at(CHAT_BADGE_BROADCASTER) == u"1") broadcaster=true;
				if (badges.contains(CHAT_BADGE_MODERATOR) && badges.at(CHAT_BADGE_MODERATOR) == u"1") moderator=true;
			}

			// emotes
			if (tags.contains(CHAT_TAG_EMOTES))
			{
				QStringView &entries=tags.at(CHAT_TAG_EMOTES);
				while (!entries.isEmpty())
				{
					std::optional<QStringView> entry=StringView::Take(entries,'/');
					if (!entry) continue;
					std::optional<QStringView> id=StringView::Take(*entry,':');
					if (!id) continue;
					while(!entry->isEmpty())
					{
						std::optional<QStringView> occurrence=StringView::Take(*entry,',');
						std::optional<QStringView> left=StringView::First(*occurrence,'-');
						std::optional<QStringView> right=StringView::Last(*occurrence,'-');
						if (!left || !right) continue;
						unsigned int start=StringConvert::PositiveInteger(left->toString());
						unsigned int end=StringConvert::PositiveInteger(right->toString());
						const Chat::Emote emote {
							.id=id->toString(),
							.start=start,
							.end=end
						};
						emotes.push_back(emote);
					}
				}
				std::sort(emotes.begin(),emotes.end());
			}
		}
	}
	window=StringView::Take(remainingText,':'); // the colon must be consumed whether the tag section was empty or not

	window=StringView::Take(remainingText,':');
	if (window)
	{
		// hostmask
		std::optional<QStringView> hostmask=StringView::Take(*window,' ');
		if (!hostmask) return;
		std::optional<QStringView> user=StringView::Take(*hostmask,'!');
		if (!user) return;
		login=*user;

		// type
		std::optional<QStringView> type=StringView::Take(*window,' ');
		if (!type) return;

		// channel
		std::optional<QStringView> channel=StringView::Take(*window,' ');
		if (!channel) return;
	}

	if (login == TWITCH_SYSTEM_ACCOUNT_NAME)
	{
		if (DispatchChatNotification(remainingText.toString())) return;
	}

	// determine if this is a command, and if so, process it as such
	// and if it's valid, we're done
	window=remainingText;
	if (std::optional<QStringView> command=StringView::Take(*window,' '); command)
	{
		if (command->size() > 0 && command->at(0) == '!')
		{
			if (DispatchCommand(command->mid(1).toString(),window->toString(),login.toString(),broadcaster || moderator)) return;
		}
	}

	if (const QString name=login.toString(); !broadcaster)
	{
		if (const std::unordered_set<QString>::const_iterator viewer=viewers.find(name); viewer == viewers.end())
		{
			DispatchArrival(name);
			viewers.emplace(name);
		}
	}

	// determine if the message is an action
	bool action=false;
	remainingText=remainingText.trimmed();
	QString before=remainingText.toString();
	if (const QString ACTION("\001ACTION"); remainingText.startsWith(ACTION))
	{
		remainingText=remainingText.mid(ACTION.size(),remainingText.lastIndexOf('\001')-ACTION.size()).trimmed();
		action=true;
	}
	QString after=remainingText.toString();

	// set emote name and check for wall of text
	int emoteCharacterCount=0;
	for (Chat::Emote &emote : emotes)
	{
		const QStringView name=remainingText.mid(emote.start,1+emote.end-emote.start); // end is an index, not a size, so we have to add 1 to get the size
		emoteCharacterCount+=name.size();
		emote.name=name.toString();
		DownloadEmote(emote); // once we know the emote name, we can determine the path, which means we can download it (download will set the path in the struct)
	}
	if (remainingText.size()-emoteCharacterCount > static_cast<int>(settingTextWallThreshold)) emit AnnounceTextWall(message,settingTextWallSound);

	emit ChatMessage(sender.toString(),remainingText.toString(),emotes,badgeIconPaths,color,action);
	inactivityClock.start();
}

std::optional<QString> Bot::DownloadBadgeIcon(const QString &badge,const QString &version)
{
	if (!badgeIconURLs.contains(badge) || !badgeIconURLs.at(badge).contains(version)) return std::nullopt;
	QString badgeIconURL=badgeIconURLs.at(badge).at(version);
	static const QString CHAT_BADGE_ICON_PATH_TEMPLATE=Filesystem::TemporaryPath().filePath(QString("%1_%2.png"));
	QString badgePath=CHAT_BADGE_ICON_PATH_TEMPLATE.arg(badge,version);
	if (!QFile(badgePath).exists())
	{
		Network::Request(badgeIconURL,Network::Method::GET,[this,badgeIconURL,badgePath](QNetworkReply *downloadReply) {
			if (downloadReply->error())
			{
				emit Print(QString("Failed to download badge %1: %2").arg(badgeIconURL,downloadReply->errorString()));
				return;
			}
			if (!QImage::fromData(downloadReply->readAll()).save(badgePath)) emit Print(QString("Failed to save badge %2").arg(badgePath));
			emit RefreshChat();
		});
	}
	return badgePath;
}

void Bot::DownloadEmote(Chat::Emote &emote)
{
	emote.path=Filesystem::TemporaryPath().filePath(QString("%1.png").arg(emote.id));
	if (!QFile(emote.path).exists())
	{
		Network::Request(QString(TWITCH_API_ENDPOINT_EMOTE_URL).arg(emote.id),Network::Method::GET,[this,emote](QNetworkReply *downloadReply) {
			if (downloadReply->error())
			{
				emit Print(QString("Failed to download emote %1: %2").arg(emote.name,downloadReply->errorString()));
				return;
			}
			if (!QImage::fromData(downloadReply->readAll()).save(emote.path)) emit Print(QString("Failed to save emote %1 to %2").arg(emote.name,emote.path));
			emit RefreshChat();
		});
	}
}

bool Bot::DispatchCommand(const QString name,const QString parameters,const QString &login,bool privileged)
{
	if (commands.find(name) == commands.end()) return false;
	Command command=parameters.isEmpty() ? commands.at(name) : Command(commands.at(name),parameters);

	if (command.Protected() && !privileged)
	{
		emit Print(QString(R"(The command "!%1" is protected but requester is not authorized")").arg(command.Name()));
		return false;
	}

	Viewer::Remote *viewer=new Viewer::Remote(security,login);
	connect(viewer,&Viewer::Remote::Print,this,&Bot::Print);
	connect(viewer,&Viewer::Remote::Recognized,viewer,[this,command,parameters](Viewer::Local viewer) {
		switch (command.Type())
		{
		case CommandType::VIDEO:
			DispatchVideo(command);
			break;
		case CommandType::AUDIO:
			emit PlayAudio(viewer.DisplayName(),command.Message(),command.Path());
			break;
		case CommandType::NATIVE:
			switch (nativeCommandFlags.at(command.Name()))
			{
			case NativeCommandFlag::AGENDA:
				emit SetAgenda(command.Message());
				break;
			case NativeCommandFlag::CATEGORY:
				StreamCategory(command.Message());
				break;
			case NativeCommandFlag::COMMANDS:
				DispatchCommandList();
				break;
			case NativeCommandFlag::EMOTE:
				ToggleEmoteOnly();
				break;
			case NativeCommandFlag::FOLLOWAGE:
				DispatchFollowage(viewer.Name());
				break;
			case NativeCommandFlag::PANIC:
				DispatchPanic(viewer.DisplayName());
				break;
			case NativeCommandFlag::SHOUTOUT:
				DispatchShoutout(command);
				break;
			case NativeCommandFlag::SONG:
				emit ShowCurrentSong(vibeKeeper->metaData("Title").toString(),vibeKeeper->metaData("AlbumTitle").toString(),vibeKeeper->metaData("AlbumArtist").toString(),vibeKeeper->metaData("CoverArtImage").value<QImage>());
				break;
			case NativeCommandFlag::TIMEZONE:
				emit ShowTimezone(QDateTime::currentDateTime().timeZone().displayName(QDateTime::currentDateTime().timeZone().isDaylightTime(QDateTime::currentDateTime()) ? QTimeZone::DaylightTime : QTimeZone::StandardTime,QTimeZone::LongName));
				break;
			case NativeCommandFlag::TITLE:
				StreamTitle(command.Message());
				break;
			case NativeCommandFlag::TOTAL_TIME:
				DispatchUptime(true);
				break;
			case NativeCommandFlag::UPTIME:
				DispatchUptime(false);
				break;
			case NativeCommandFlag::VIBE:
				ToggleVibeKeeper();
				break;
			case NativeCommandFlag::VOLUME:
				AdjustVibeVolume(command);
				break;
			}
			break;
		case CommandType::BLANK:
			break;
		};
	});

	return true;
}

bool Bot::DispatchChatNotification(const QStringView &message)
{
	if (message.contains(u"is now hosting you."))
	{
		std::optional<QStringView> host=StringView::First(message,' ');
		if (!host) return false;
		emit AnnounceHost(host->toString(),settingHostSound);
		return true;
	}
	return false;
}

void Bot::DispatchVideo(Command command)
{
	if (command.Random())
		DispatchRandomVideo(command);
	else
		emit PlayVideo(command.Path());
}

void Bot::DispatchRandomVideo(Command command)
{
	QDir directory(command.Path());
	QStringList videos=directory.entryList(QStringList() << "*.mp4",QDir::Files);
	if (videos.size() < 1)
	{
		emit Print("No videos found");
		return;
	}
	emit PlayVideo(directory.absoluteFilePath(videos.at(Random::Bounded(videos))));
}

void Bot::DispatchCommandList()
{
	std::vector<std::pair<QString,QString>> descriptions;
	for (const std::pair<const QString,Command> &command : commands)
	{
		if (!command.second.Protected()) descriptions.push_back({command.first,command.second.Description()});
	}
	emit ShowCommandList(descriptions);
}

void Bot::DispatchFollowage(const QString &name)
{
	Viewer::Remote *broadcaster=new Viewer::Remote(security,security.Administrator());
	connect(broadcaster,&Viewer::Remote::Print,this,&Bot::Print);
	connect(broadcaster,&Viewer::Remote::Recognized,broadcaster,[this,name](Viewer::Local broadcaster) {
		Viewer::Remote *viewer=new Viewer::Remote(security,name);
		connect(viewer,&Viewer::Remote::Print,this,&Bot::Print);
		connect(viewer,&Viewer::Remote::Recognized,viewer,[this,broadcaster](Viewer::Local viewer) {
			Network::Request({TWITCH_API_ENDPOING_USER_FOLLOWS},Network::Method::GET,[this,broadcaster,viewer](QNetworkReply *reply) {
				QJsonParseError jsonError;
				QJsonDocument json=QJsonDocument::fromJson(reply->readAll(),&jsonError);
				if (json.isNull() || !json.object().contains("data"))
				{
					emit Print("Something went wrong obtaining stream information");
					return;
				}
				QJsonArray data=json.object().value("data").toArray();
				if (data.size() < 1)
				{
					emit Print("Response from requesting stream information was incomplete ");
					return;
				}
				QJsonObject details=data.at(0).toObject();
				QDateTime start=QDateTime::fromString(details.value("followed_at").toString(),Qt::ISODate);
				std::chrono::milliseconds duration=static_cast<std::chrono::milliseconds>(start.msecsTo(QDateTime::currentDateTimeUtc()));
				std::chrono::years years=std::chrono::duration_cast<std::chrono::years>(duration);
				std::chrono::months months=std::chrono::duration_cast<std::chrono::months>(duration-years);
				std::chrono::days days=std::chrono::duration_cast<std::chrono::days>(duration-years-months);
				emit ShowFollowage(viewer.DisplayName(),years,months,days);
			},{
				{"from_id",viewer.ID()},
				{"to_id",broadcaster.ID()}
			},{
				{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
				{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
				{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
			});
		});
	});
}

void Bot::DispatchPanic(const QString &name)
{
	QFile outputFile(Filesystem::DataPath().absoluteFilePath("panic.txt"));
	QString outputText;
	if (!outputFile.open(QIODevice::ReadOnly)) return;
	outputText=QString(outputFile.readAll()).arg(name);
	outputFile.close();
	QString date=QDateTime::currentDateTime().toString("ddd d hh:mm:ss");
	outputText=outputText.split("\n").join(QString("\n%1 ").arg(date));
	emit Panic(date+"\n"+outputText);
}

void Bot::DispatchShoutout(Command command)
{
	Viewer::Remote *viewer=new Viewer::Remote(security,QString(command.Message()).remove("@"));
	connect(viewer,&Viewer::Remote::Recognized,viewer,[this](Viewer::Local viewer) {
		Viewer::ProfileImage::Remote *profileImage=viewer.ProfileImage();
		connect(profileImage,&Viewer::ProfileImage::Remote::Retrieved,profileImage,[this,viewer](const QImage &profileImage) {
			emit Shoutout(viewer.DisplayName(),viewer.Description(),profileImage);
		});
	});
	connect(viewer,&Viewer::Remote::Print,this,&Bot::Print);
}

void Bot::DispatchUptime(bool total)
{
	Network::Request({TWITCH_API_ENDPOINT_STREAM_INFORMATION},Network::Method::GET,[this,total](QNetworkReply *reply) {
		QJsonParseError jsonError;
		QJsonDocument json=QJsonDocument::fromJson(reply->readAll(),&jsonError);
		if (json.isNull() || !json.object().contains("data"))
		{
			emit Print("Something went wrong obtaining stream information");
			return;
		}
		QJsonArray data=json.object().value("data").toArray();
		if (data.size() < 1)
		{
			emit Print("Response from requesting stream information was incomplete ");
			return;
		}
		QJsonObject details=data.at(0).toObject();
		QDateTime start=QDateTime::fromString(details.value("started_at").toString(),Qt::ISODate);
		std::chrono::milliseconds duration=static_cast<std::chrono::milliseconds>(start.msecsTo(QDateTime::currentDateTimeUtc()));
		if (total) duration+=std::chrono::minutes(static_cast<qint64>(settingUptimeHistory));
		std::chrono::hours hours=std::chrono::duration_cast<std::chrono::hours>(duration);
		std::chrono::minutes minutes=std::chrono::duration_cast<std::chrono::minutes>(duration-hours);
		std::chrono::seconds seconds=std::chrono::duration_cast<std::chrono::seconds>(duration-hours-minutes);
		if (total)
			emit ShowTotalTime(hours,minutes,seconds);
		else
			emit ShowUptime(hours,minutes,seconds);
	},{
		{"user_login",security.Administrator()}
	},{
		{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
	});
}

void Bot::ToggleVibeKeeper()
{
	if (vibeKeeper->state() == QMediaPlayer::PlayingState)
	{
		emit Print("Pausing the vibes...");
		vibeKeeper->pause();
		return;
	}
	vibeKeeper->play();
}

void Bot::AdjustVibeVolume(Command command)
{
	if (command.Message().isEmpty())
	{
		if (vibeFader)
		{
			vibeFader->Abort();
			emit Print("Aborting volume change...");
		}
		return;
	}

	try
	{
		vibeFader=new Volume::Fader(vibeKeeper,command.Message(),this);
		connect(vibeFader,&Volume::Fader::Print,this,&Bot::Print);
		vibeFader->Start();
	}
	catch (const std::range_error &exception)
	{
		emit Print(QString("%1: %2").arg("Failed to adjust volume",exception.what()));
	}
}

void Bot::ToggleEmoteOnly()
{
	Viewer::Remote *broadcaster=new Viewer::Remote(security,security.Administrator());
	connect(broadcaster,&Viewer::Remote::Print,this,&Bot::Print);
	connect(broadcaster,&Viewer::Remote::Recognized,broadcaster,[this](Viewer::Local broadcaster) {
		Network::Request({TWITCH_API_ENDPOINT_CHAT_SETTINGS},Network::Method::GET,[this,broadcaster](QNetworkReply *reply) {
			QJsonParseError jsonError;
			QJsonDocument json=QJsonDocument::fromJson(reply->readAll(),&jsonError);
			if (json.isNull() || !json.object().contains("data"))
			{
				emit Print("Something went wrong changing emote only mode");
				return;
			}
			QJsonArray data=json.object().value("data").toArray();
			if (data.size() < 1)
			{
				emit Print("Response was incomplete changing emote only mode");
				return;
			}
			QJsonObject details=data.at(0).toObject();
			if (details.value("emote_mode").toBool())
				EmoteOnly(false,broadcaster.ID());
			else
				EmoteOnly(true,broadcaster.ID());
		},{
			{QUERY_PARAMETER_BROADCASTER_ID,broadcaster.ID()},
			{QUERY_PARAMETER_MODERATOR_ID,broadcaster.ID()}
		},{
			{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
			{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
			{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
		});
	});
}

void Bot::EmoteOnly(bool enable,const QString &broadcasterID)
{
	// just use broadcasterID for both
	// this is for authorizing the moderator at the API-level, which we're not worried about here
	// it's always going to be the broadcaster running the bot and access is protected through
	// DispatchCommand()
	Network::Request({TWITCH_API_ENDPOINT_CHAT_SETTINGS},Network::Method::PATCH,[this](QNetworkReply *reply) {
		if (reply->error()) emit Print(QString("Something went wrong setting emote only: %1").arg(reply->errorString()));
	},{
		{QUERY_PARAMETER_BROADCASTER_ID,broadcasterID},
		{QUERY_PARAMETER_MODERATOR_ID,broadcasterID}
	},{
		{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
	},QJsonDocument(QJsonObject({{"emote_mode",enable}})).toJson(QJsonDocument::Compact));
}

void Bot::StreamTitle(const QString &title)
{
	Viewer::Remote *broadcaster=new Viewer::Remote(security,security.Administrator());
	connect(broadcaster,&Viewer::Remote::Print,this,&Bot::Print);
	connect(broadcaster,&Viewer::Remote::Recognized,broadcaster,[this,title](Viewer::Local broadcaster) {
		Network::Request({TWITCH_API_ENDPOINT_CHANNEL_INFORMATION},Network::Method::PATCH,[this,broadcaster,title](QNetworkReply *reply) {
			if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 204)
			{
				emit Print("Failed to change stream title");
				return;
			}
			emit Print(QString(R"(Stream title changed to "%1")").arg(title));
		},{
			{QUERY_PARAMETER_BROADCASTER_ID,broadcaster.ID()}
		},{
			{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
			{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
			{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
		},
		{
			QJsonDocument(QJsonObject({{"title",title}})).toJson(QJsonDocument::Compact)
		});
	});
}

void Bot::StreamCategory(const QString &category)
{
	Viewer::Remote *broadcaster=new Viewer::Remote(security,security.Administrator());
	connect(broadcaster,&Viewer::Remote::Print,this,&Bot::Print);
	connect(broadcaster,&Viewer::Remote::Recognized,broadcaster,[this,category](Viewer::Local broadcaster) {
		Network::Request({TWITCH_API_ENDPOINT_GAME_INFORMATION},Network::Method::GET,[this,broadcaster,category](QNetworkReply *reply) {
			QJsonParseError jsonError;
			QJsonDocument json=QJsonDocument::fromJson(reply->readAll(),&jsonError);
			if (json.isNull() || !json.object().contains("data"))
			{
				emit Print("Something went wrong finding stream category");
				return;
			}
			QJsonArray data=json.object().value("data").toArray();
			if (data.size() < 1)
			{
				emit Print("Response was incomplete finding stream category");
				return;
			}
			Network::Request({TWITCH_API_ENDPOINT_CHANNEL_INFORMATION},Network::Method::PATCH,[this,broadcaster,category](QNetworkReply *reply) {
				if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 204)
				{
					emit Print("Failed to change stream category");
					return;
				}
				emit Print(QString(R"(Stream category changed to "%1")").arg(category));
			},{
				{QUERY_PARAMETER_BROADCASTER_ID,broadcaster.ID()}
			},{
				{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
				{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
				{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
			},
			{
				QJsonDocument(QJsonObject({{"game_id",data.at(0).toObject().value("id").toString()}})).toJson(QJsonDocument::Compact)
			});
		},{
			{"name",category}
		},{
			{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
			{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
			{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
		});
	});
}
