#include <QFile>

#include <QApplication>
#include <QTimeZone>
#include <QNetworkReply>
#include "bot.h"
#include "globals.h"

const char *COMMANDS_LIST_FILENAME="commands.json";
const char *COMMAND_TYPE_NATIVE="native";
const char *COMMAND_TYPE_AUDIO="announce";
const char *COMMAND_TYPE_VIDEO="video";
const char *COMMAND_TYPE_PULSAR="pulsar";
const char *VIEWER_ATTRIBUTES_FILENAME="viewers.json";
const char *VIBE_PLAYLIST_FILENAME="songs.json";
const char *NETWORK_HEADER_AUTHORIZATION="Authorization";
const char *NETWORK_HEADER_CLIENT_ID="Client-Id";
const char *QUERY_PARAMETER_BROADCASTER_ID="broadcaster_id";
const char *QUERY_PARAMETER_MODERATOR_ID="moderator_id";
const char *JSON_KEY_COMMAND_NAME="command";
const char *JSON_KEY_COMMAND_ALIASES="aliases";
const char *JSON_KEY_COMMAND_DESCRIPTION="description";
const char *JSON_KEY_COMMAND_TYPE="type";
const char *JSON_KEY_COMMAND_RANDOM_PATH="random";
const char *JSON_KEY_COMMAND_DUPLICATES="duplicates";
const char *JSON_KEY_COMMAND_PROTECTED="protected";
const char *JSON_KEY_COMMAND_PATH="path";
const char *JSON_KEY_COMMAND_MESSAGE="message";
const char *JSON_KEY_COMMAND_REDEMPTION="redemption";
const char *JSON_KEY_DATA="data";
const char *JSON_KEY_COMMANDS="commands";
const char *JSON_KEY_WELCOME="welcomed";
const char *JSON_KEY_BOT="bot";
const char *JSON_ARRAY_EMPTY="[]";
const char *SETTINGS_CATEGORY_EVENTS="Events";
const char *SETTINGS_CATEGORY_VIBE="Vibe";
const char *SETTINGS_CATEGORY_COMMANDS="Commands";
const char *TWITCH_API_ENDPOINT_EMOTE_URL="https://static-cdn.jtvnw.net/emoticons/v1/%1/1.0";
const char *TWITCH_API_ENDPOINT_CHAT_SETTINGS="https://api.twitch.tv/helix/chat/settings";
const char *TWITCH_API_ENDPOINT_STREAM_INFORMATION="https://api.twitch.tv/helix/streams";
const char *TWITCH_API_ENDPOINT_CHANNEL_INFORMATION="https://api.twitch.tv/helix/channels";
const char *TWITCH_API_ENDPOINT_GAME_INFORMATION="https://api.twitch.tv/helix/games";
const char *TWITCH_API_ENDPOINT_USER_FOLLOWS="https://api.twitch.tv/helix/users/follows";
const char *TWITCH_API_ENDPOINT_BADGES="https://api.twitch.tv/helix/chat/badges/global";
const char *TWITCH_API_ENDPOINT_CHATTERS="https://api.twitch.tv/helix/chat/chatters";
const char *TWITCH_API_OPERATION_STREAM_INFORMATION="stream information";
const char *TWITCH_API_OPERATION_USER_FOLLOWS="user follow details";
const char *TWITCH_API_OPERATION_EMOTE_ONLY="emote only";
const char *TWITCH_API_OPERATION_STREAM_TITLE="stream title";
const char *TWITCH_API_OPERATION_STREAM_CATEGORY="stream category";
const char *TWITCH_API_OPERATION_LOAD_BADGES="badges";
const char *TWITCH_API_ERROR_TEMPLATE_INCOMPLETE="Response from requesting %1 was incomplete";
const char *TWITCH_API_ERROR_TEMPLATE_UNKNOWN="Something went wrong obtaining %1";
const char *TWITCH_API_ERROR_TEMPLATE_JSON_PARSE="Error parsing %1 JSON: %2";
const char *TWITCH_API_ERROR_AUTH="Auth token or client ID missing or invalid";
const char16_t *TWITCH_SYSTEM_ACCOUNT_NAME=u"jtv";
const char16_t *CHAT_BADGE_BROADCASTER=u"broadcaster";
const char16_t *CHAT_BADGE_MODERATOR=u"moderator";
const char16_t *CHAT_TAG_DISPLAY_NAME=u"display-name";
const char16_t *CHAT_TAG_BADGES=u"badges";
const char16_t *CHAT_TAG_COLOR=u"color";
const char16_t *CHAT_TAG_EMOTES=u"emotes";
const char *FILE_OPERATION_CREATE="create";
const char *FILE_OPERATION_OPEN="open";
const char *FILE_OPERATION_PARSE="parse";
const char *FILE_OPERATION_WRITE="write";
const char *FILE_ERROR_TEMPLATE_COMMANDS_LIST="Failed to %1 command list file: %2";
const char *FILE_ERROR_TEMPLATE_VIBE_PLAYLIST="Failed to %1 vibe playlist list file: %2";

const std::unordered_map<QString,CommandType> Bot::COMMAND_TYPE_LOOKUP={
	{COMMAND_TYPE_NATIVE,CommandType::NATIVE},
	{COMMAND_TYPE_VIDEO,CommandType::VIDEO},
	{COMMAND_TYPE_AUDIO,CommandType::AUDIO},
	{COMMAND_TYPE_PULSAR,CommandType::PULSAR}
};

Bot::BadgeIconURLsLookup Bot::badgeIconURLs;
std::chrono::milliseconds Bot::launchTimestamp=TimeConvert::Now();

Bot::Bot(Security &security,QObject *parent) : QObject(parent),
	vibeKeeper(this,true),
	roaster(this,false),
	security(security),
	settingInactivityCooldown(SETTINGS_CATEGORY_EVENTS,"InactivityCooldown",1800000),
	settingHelpCooldown(SETTINGS_CATEGORY_EVENTS,"HelpCooldown",300000),
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
	settingDeniedCommandVideo(SETTINGS_CATEGORY_COMMANDS,"Denied"),
	settingUptimeHistory(SETTINGS_CATEGORY_COMMANDS,"UptimeHistory",0),
	settingCommandNameAgenda(SETTINGS_CATEGORY_COMMANDS,"Agenda","agenda"),
	settingCommandNameStreamCategory(SETTINGS_CATEGORY_COMMANDS,"StreamCategory","category"),
	settingCommandNameStreamTitle(SETTINGS_CATEGORY_COMMANDS,"StreamTitle","title"),
	settingCommandNameCommands(SETTINGS_CATEGORY_COMMANDS,"Commands","commands"),
	settingCommandNameEmote(SETTINGS_CATEGORY_COMMANDS,"EmoteOnly","emote"),
	settingCommandNameFollowage(SETTINGS_CATEGORY_COMMANDS,"Followage","followage"),
	settingCommandNameHTML(SETTINGS_CATEGORY_COMMANDS,"HTML","html"),
	settingCommandNamePanic(SETTINGS_CATEGORY_COMMANDS,"Panic","panic"),
	settingCommandNameShoutout(SETTINGS_CATEGORY_COMMANDS,"Shoutout","so"),
	settingCommandNameSong(SETTINGS_CATEGORY_COMMANDS,"Song","song"),
	settingCommandNameTimezone(SETTINGS_CATEGORY_COMMANDS,"Timezone","timezone"),
	settingCommandNameUptime(SETTINGS_CATEGORY_COMMANDS,"Uptime","uptime"),
	settingCommandNameTotalTime(SETTINGS_CATEGORY_COMMANDS,"TotalTime","totaltime"),
	settingCommandNameVibe(SETTINGS_CATEGORY_COMMANDS,"Vibe","vibe"),
	settingCommandNameVibeVolume(SETTINGS_CATEGORY_COMMANDS,"VibeVolume","volume")
{
	DeclareCommand({settingCommandNameAgenda,"Set the agenda of the stream, displayed in the header of the chat window",CommandType::NATIVE,true},NativeCommandFlag::AGENDA);
	DeclareCommand({settingCommandNameStreamCategory,"Change the stream category",CommandType::NATIVE,true},NativeCommandFlag::CATEGORY);
	DeclareCommand({settingCommandNameStreamTitle,"Change the stream title",CommandType::NATIVE,true},NativeCommandFlag::TITLE);
	DeclareCommand({settingCommandNameCommands,"List all of the commands Celeste recognizes",CommandType::NATIVE,false},NativeCommandFlag::COMMANDS);
	DeclareCommand({settingCommandNameEmote,"Toggle emote only mode in chat",CommandType::NATIVE,true},NativeCommandFlag::EMOTE);
	DeclareCommand({settingCommandNameFollowage,"Show how long a user has followed the broadcaster",CommandType::NATIVE,false},NativeCommandFlag::FOLLOWAGE);
	DeclareCommand({settingCommandNameHTML,"Format the chat message as HTML",CommandType::NATIVE,false},NativeCommandFlag::HTML);
	DeclareCommand({settingCommandNamePanic,"Crash Celeste",CommandType::NATIVE,true},NativeCommandFlag::PANIC);
	DeclareCommand({settingCommandNameShoutout,"Call attention to another streamer's channel",CommandType::NATIVE,false},NativeCommandFlag::SHOUTOUT);
	DeclareCommand({settingCommandNameSong,"Show the title, album, and artist of the song that is currently playing",CommandType::NATIVE,false},NativeCommandFlag::SONG);
	DeclareCommand({settingCommandNameTimezone,"Display the timezone of the system the bot is running on",CommandType::NATIVE,false},NativeCommandFlag::TIMEZONE);
	DeclareCommand({settingCommandNameUptime,"Show how long the bot has been connected",CommandType::NATIVE,false},NativeCommandFlag::TOTAL_TIME);
	DeclareCommand({settingCommandNameTotalTime,"Show how many total hours stream has ever been live",CommandType::NATIVE,false},NativeCommandFlag::UPTIME);
	DeclareCommand({settingCommandNameVibe,"Start the playlist of music for the stream",CommandType::NATIVE,true},NativeCommandFlag::VIBE);
	DeclareCommand({settingCommandNameVibeVolume,"Adjust the volume of the vibe keeper",CommandType::NATIVE,true},NativeCommandFlag::VOLUME);
	LoadViewerAttributes();

	if (settingRoasts) LoadRoasts();
	LoadBadgeIconURLs();
	StartClocks();

	lastRaid=QDateTime::currentDateTime().addMSecs(static_cast<qint64>(0)-static_cast<qint64>(settingRaidInterruptDuration));

	connect(&vibeKeeper,&Music::Player::Print,this,&Bot::Print);
}

void Bot::DeclareCommand(const Command &&command,NativeCommandFlag flag)
{
	commands.insert({{command.Name(),command}});
	nativeCommandFlags.insert({{command.Name(),flag}});
}

QJsonDocument Bot::LoadDynamicCommands()
{
	QFile commandListFile(Filesystem::DataPath().filePath(COMMANDS_LIST_FILENAME));
	if (!commandListFile.exists())
	{
		if (!Filesystem::Touch(commandListFile)) throw std::runtime_error(QString{FILE_ERROR_TEMPLATE_COMMANDS_LIST}.arg(FILE_OPERATION_CREATE,commandListFile.fileName()).toStdString());
	}
	if (!commandListFile.open(QIODevice::ReadOnly)) throw std::runtime_error(QString{FILE_ERROR_TEMPLATE_COMMANDS_LIST}.arg(FILE_OPERATION_OPEN,commandListFile.fileName()).toStdString());

	QJsonParseError jsonError;
	QByteArray data=commandListFile.readAll();
	if (data.isEmpty()) data=JSON_ARRAY_EMPTY;
	const JSON::ParseResult parsedJSON=JSON::Parse(data);
	if (!parsedJSON) throw std::runtime_error(QString{FILE_ERROR_TEMPLATE_COMMANDS_LIST}.arg(FILE_OPERATION_PARSE,parsedJSON.error).toStdString());
	return parsedJSON();
}

const Command::Lookup& Bot::DeserializeCommands(const QJsonDocument &json)
{
	const QJsonArray objects=json.array();
	for (const QJsonValue &jsonValue : objects)
	{
		QJsonObject jsonObject=jsonValue.toObject();

		// check if this is triggered by a redemption
		auto redemptionName=jsonObject.find(JSON_KEY_COMMAND_REDEMPTION);
		if (redemptionName != jsonObject.end())
		{
			StageRedemptionCommand(redemptionName->toString(),jsonObject);
			continue;
		}

		const QString name=jsonObject.value(JSON_KEY_COMMAND_NAME).toString();
		if (!commands.contains(name))
		{
			std::optional<CommandType> type=ValidCommandType(jsonObject.value(JSON_KEY_COMMAND_TYPE).toString());
			if (!type) continue;

			commands.insert({name,{
				name,
				jsonObject.value(JSON_KEY_COMMAND_DESCRIPTION).toString(),
				*type,
				Container::Resolve(jsonObject,JSON_KEY_COMMAND_RANDOM_PATH,false).toBool(),
				Container::Resolve(jsonObject,JSON_KEY_COMMAND_DUPLICATES,true).toBool(),
				jsonObject.value(JSON_KEY_COMMAND_PATH).toString(),
				Command::FileListFilters(*type),
				Container::Resolve(jsonObject,JSON_KEY_COMMAND_MESSAGE,{}).toString(),
				Container::Resolve(jsonObject,JSON_KEY_COMMAND_PROTECTED,false).toBool()
			}});
		}

		auto jsonObjectAliases=jsonObject.find(JSON_KEY_COMMAND_ALIASES);
		if (jsonObjectAliases == jsonObject.end()) continue;
		const QJsonArray aliases=jsonObjectAliases->toArray();
		for (const QJsonValue &jsonValue : aliases)
		{
			const QString alias=jsonValue.toString();
			const Command &command=commands.at(name);
			const CommandType type=command.Type();
			commands.try_emplace(alias,alias,&commands.at(name));
			if (type == CommandType::NATIVE) nativeCommandFlags.insert({alias,nativeCommandFlags.at(name)});
		}
	}
	return commands;
}

QJsonDocument Bot::SerializeCommands(const Command::Lookup &entries)
{
	NativeCommandFlagLookup mergedNativeCommandFlags;
	QJsonArray array;
	std::unordered_map<QString,QStringList> aliases;
	for (const Command::Entry &entry : entries)
	{
		const Command &command=entry.second;

		if (command.Parent())
		{
			aliases[command.Parent()->Name()].push_back(command.Name());
			continue; // if this is just an alias, move on without processing it as a full command
		}

		QJsonObject object;
		object.insert(JSON_KEY_COMMAND_NAME,command.Name());

		QString type;
		switch (command.Type())
		{
		case CommandType::NATIVE:
			mergedNativeCommandFlags.insert(nativeCommandFlags.extract(command.Name()));
			continue;
		case CommandType::AUDIO:
			object.insert(JSON_KEY_COMMAND_TYPE,COMMAND_TYPE_AUDIO);
			object.insert(JSON_KEY_COMMAND_DESCRIPTION,command.Description());
			object.insert(JSON_KEY_COMMAND_PATH,command.Path());
			if (command.Random())
			{
				object.insert(JSON_KEY_COMMAND_RANDOM_PATH,true);
				object.insert(JSON_KEY_COMMAND_DUPLICATES,command.Duplicates());
			}
			if (!command.Message().isEmpty()) object.insert(JSON_KEY_COMMAND_MESSAGE,command.Message());
			if (command.Protected()) object.insert(JSON_KEY_COMMAND_PROTECTED,command.Protected());
			break;
		case CommandType::VIDEO:
			object.insert(JSON_KEY_COMMAND_TYPE,COMMAND_TYPE_VIDEO);
			object.insert(JSON_KEY_COMMAND_DESCRIPTION,command.Description());
			object.insert(JSON_KEY_COMMAND_PATH,command.Path());
			if (command.Random())
			{
				object.insert(JSON_KEY_COMMAND_RANDOM_PATH,true);
				object.insert(JSON_KEY_COMMAND_DUPLICATES,command.Duplicates());
			}
			if (command.Protected()) object.insert(JSON_KEY_COMMAND_PROTECTED,command.Protected());
			break;
		case CommandType::PULSAR:
			object.insert(JSON_KEY_COMMAND_TYPE,COMMAND_TYPE_PULSAR);
			object.insert(JSON_KEY_COMMAND_DESCRIPTION,command.Description());
			if (command.Protected()) object.insert(JSON_KEY_COMMAND_PROTECTED,command.Protected());
			break;
		}

		array.append(object);
	}

	for (QJsonValueRef value : array)
	{
		// if aliases exist for this object, attach the array of aliases to it
		QJsonObject object=value.toObject();
		QString name=object.value(JSON_KEY_COMMAND_NAME).toString();
		auto candidate=aliases.find(name);
		if (candidate != aliases.end())
		{
			QJsonArray names;
			QStringList nodes=aliases.extract(candidate).mapped();
			for (const QString &alias : nodes) names.append(alias);
			object.insert(JSON_KEY_COMMAND_ALIASES,names);
		}
		value=object;
	}

	// catch the aliases that were for native commands,
	// which normally aren't listed in the commands file
	for (const std::pair<QString,QStringList> &pair : aliases)
	{
		QJsonArray names;
		for (const QString &name : pair.second) names.append(name);
		array.append(QJsonObject({
			{JSON_KEY_COMMAND_NAME,pair.first},
			{JSON_KEY_COMMAND_ALIASES,names}
		}));
	}

	this->commands.swap(commands);
	nativeCommandFlags.swap(mergedNativeCommandFlags);

	return QJsonDocument(array);
}

bool Bot::SaveDynamicCommands(const QJsonDocument &json)
{
	const char *OPERATION="save dynamic commands";
	QFile commandListFile(Filesystem::DataPath().filePath(COMMANDS_LIST_FILENAME));
	if (!commandListFile.open(QIODevice::WriteOnly))
	{
		emit Print(QString{FILE_ERROR_TEMPLATE_COMMANDS_LIST}.arg(FILE_OPERATION_OPEN,commandListFile.fileName()),OPERATION);
		return false;
	}
	if (commandListFile.write(json.toJson(QJsonDocument::Indented)) <= 0)
	{
		emit Print(QString{FILE_ERROR_TEMPLATE_COMMANDS_LIST}.arg(FILE_OPERATION_WRITE,commandListFile.fileName()),OPERATION);
		return false;
	}
	return true;
}

void Bot::StageRedemptionCommand(const QString &name,const QJsonObject &jsonObject)
{
	std::optional<CommandType> type=ValidCommandType(jsonObject.value(JSON_KEY_COMMAND_TYPE).toString());
	if (!type) return;

	redemptions[name]={
		name,
		jsonObject.value(JSON_KEY_COMMAND_DESCRIPTION).toString(),
		*type,
		Container::Resolve(jsonObject,JSON_KEY_COMMAND_RANDOM_PATH,false).toBool(),
		Container::Resolve(jsonObject,JSON_KEY_COMMAND_DUPLICATES,true).toBool(),
		jsonObject.value(JSON_KEY_COMMAND_PATH).toString(),
		Command::FileListFilters(*type),
		jsonObject.contains(JSON_KEY_COMMAND_MESSAGE) ? jsonObject.value(JSON_KEY_COMMAND_MESSAGE).toString() : QString(),
		false
	};
}

const Command::Lookup& Bot::Commands() const
{
	return commands;
}

bool Bot::LoadViewerAttributes() // FIXME: have this throw an exception rather than return a bool
{
	QFile commandListFile(Filesystem::DataPath().filePath(VIEWER_ATTRIBUTES_FILENAME));
	if (!commandListFile.open(QIODevice::ReadWrite))
	{
		emit Print(QString("Failed to open user attributes file: %1").arg(commandListFile.fileName()));
		return false;
	}

	QJsonParseError jsonError;
	QByteArray data=commandListFile.readAll();
	if (data.isEmpty()) data="{}";
	QJsonDocument json=QJsonDocument::fromJson(data,&jsonError);
	if (json.isNull())
	{
		emit Print(jsonError.errorString());
		return false;
	}

	const QJsonObject entries=json.object();
	for (QJsonObject::const_iterator viewer=entries.begin(); viewer != entries.end(); ++viewer)
	{
		const QJsonObject attributes=viewer->toObject();
		viewers[viewer.key()]={
			Container::Resolve(attributes,JSON_KEY_COMMANDS,true).toBool(),
			Container::Resolve(attributes,JSON_KEY_WELCOME,false).toBool(),
			Container::Resolve(attributes,JSON_KEY_BOT,false).toBool(),
		};
	}

	return true;
}

void Bot::SaveViewerAttributes(bool resetWelcomes)
{
	QFile viewerAttributesFile(Filesystem::DataPath().filePath(VIEWER_ATTRIBUTES_FILENAME));
	if (!viewerAttributesFile.open(QIODevice::ReadWrite)) return; // FIXME: how can we report the error here while closing?

	QJsonObject entries;
	for (const std::pair<QString,Viewer::Attributes> &viewer : viewers)
	{
		QJsonObject attributes={
			{JSON_KEY_COMMANDS,viewer.second.commands},
			{JSON_KEY_WELCOME,resetWelcomes ? false : viewer.second.welcomed},
			{JSON_KEY_BOT,viewer.second.bot}
		};
		entries.insert(viewer.first,attributes);
	}

	viewerAttributesFile.write(QJsonDocument(entries).toJson(QJsonDocument::Indented));
}

const File::List& Bot::DeserializeVibePlaylist(const QJsonDocument &json)
{
	vibeKeeper.Sources(File::List{json.toVariant().toStringList()});
	return vibeKeeper.Sources();
}

QJsonDocument Bot::LoadVibePlaylist()
{
	static const char *OPERATION="load vibe playlist";
	QFile songListFile(Filesystem::DataPath().filePath(VIBE_PLAYLIST_FILENAME));

	if (!songListFile.exists())
	{
		if (!Filesystem::Touch(songListFile)) throw std::runtime_error(QString{FILE_ERROR_TEMPLATE_VIBE_PLAYLIST}.arg(FILE_OPERATION_CREATE,songListFile.fileName()).toStdString());
	}

	if (!songListFile.open(QIODevice::ReadOnly))
	{
		emit Print(QString{FILE_ERROR_TEMPLATE_VIBE_PLAYLIST}.arg(FILE_OPERATION_OPEN,songListFile.errorString()),OPERATION);
		return {};
	}

	QByteArray data=songListFile.readAll();
	if (data.isEmpty()) data=JSON_ARRAY_EMPTY;
	const JSON::ParseResult parsedJSON=JSON::Parse(data);
	if (!parsedJSON)
	{
		emit Print(QString{FILE_ERROR_TEMPLATE_VIBE_PLAYLIST}.arg(FILE_OPERATION_PARSE,parsedJSON.error),OPERATION);
		return {};
	}

	emit Print("Playlist loaded!",OPERATION);
	return parsedJSON();
}

QJsonDocument Bot::SerializeVibePlaylist(const File::List &songs)
{
	return QJsonDocument::fromVariant(songs());
}

bool Bot::SaveVibePlaylist(const QJsonDocument &json)
{
	static const char *OPERATION="save vibe playlist";
	QFile songListFile(Filesystem::DataPath().filePath(VIBE_PLAYLIST_FILENAME));
	if (!songListFile.open(QIODevice::WriteOnly))
	{
		emit Print(QString{FILE_ERROR_TEMPLATE_VIBE_PLAYLIST}.arg(FILE_OPERATION_OPEN,songListFile.fileName()),OPERATION);
		return false;
	}
	if (songListFile.write(json.toJson(QJsonDocument::Indented)) <= 0)
	{
		emit Print(QString{FILE_ERROR_TEMPLATE_VIBE_PLAYLIST}.arg(FILE_OPERATION_WRITE,songListFile.fileName()),OPERATION);
		return false;
	}
	return true;
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
		emit Print(QString("Roaster failed to start: %1").arg(roaster->errorString()));
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
	Network::Request({TWITCH_API_ENDPOINT_BADGES},Network::Method::GET,[this](QNetworkReply *reply) {
		static const char *JSON_KEY_ID="id";
		static const char *JSON_KEY_SET_ID="set_id";
		static const char *JSON_KEY_VERSIONS="versions";
		static const char *JSON_KEY_IMAGE_URL="image_url_1x";

		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_JSON_PARSE).arg(TWITCH_API_OPERATION_LOAD_BADGES,parsedJSON.error));
			return;
		}

		const QJsonObject object=parsedJSON().object();
		auto jsonFieldData=object.find(JSON_KEY_DATA);
		if (jsonFieldData == object.end()) return;
		for (const QJsonValue &set : jsonFieldData->toArray())
		{
			const QJsonObject objectBadgeSet=set.toObject();
			auto jsonFieldSetID=objectBadgeSet.find(JSON_KEY_SET_ID);
			auto jsonFieldVersions=objectBadgeSet.find(JSON_KEY_VERSIONS);
			if (jsonFieldSetID == objectBadgeSet.end() || jsonFieldVersions == objectBadgeSet.end()) return;
			for (const QJsonValue &version : jsonFieldVersions->toArray())
			{
				const QJsonObject objectImageVersions=version.toObject();
				auto jsonFieldID=objectImageVersions.find(JSON_KEY_ID);
				auto jsonFieldVersionURL=objectImageVersions.find(JSON_KEY_IMAGE_URL);
				if (jsonFieldID == objectImageVersions.end() || jsonFieldVersionURL == objectImageVersions.end()) return;
				badgeIconURLs[jsonFieldSetID->toString()][jsonFieldID->toString()]=jsonFieldVersionURL->toString();
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

void Bot::Redemption(const QString &login,const QString &name,const QString &rewardTitle,const QString &message)
{
	if (rewardTitle == "Crash Celeste")
	{
		DispatchPanic(name);
		vibeKeeper.Stop();
		return;
	}

	if (auto redemption=redemptions.find(rewardTitle); redemption != redemptions.end())
	{
		const Command &command=redemption->second;
		DispatchCommand(command,login);
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

void Bot::SuppressMusic()
{
	vibeKeeper.DuckVolume(true);
}

void Bot::RestoreMusic()
{
	vibeKeeper.DuckVolume(false);
}

void Bot::Chatters()
{
	Network::Request({TWITCH_API_ENDPOINT_CHATTERS},Network::Method::GET,[this](QNetworkReply *reply) {
		switch (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
		{
		case 400:
			emit Print(QStringLiteral("Invalid broadcaster ID"));
			return;
		case 401:
			emit Print(QStringLiteral("Not authorized to obtain list of chatters (moderator:read:chatters)"));
			return;
		case 403:
			emit Print(QStringLiteral("User requesting list of chatters is not a moderator"));
			return;
		}

		if (reply->error())
		{
			emit Print(QStringLiteral("Failed to obtain list of chatters"));
			return;
		}

		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			emit Print(QStringLiteral("Invalid response obtaining list of chatters"));
			return;
		}

		const QJsonObject object=parsedJSON().object();
		auto jsonFieldData=object.find(JSON_KEY_DATA);
		if (jsonFieldData == object.end()) return;

		const QJsonArray array=jsonFieldData->toArray();
		QStringList names;
		for (const QJsonValue &value : array) names.append(value.toObject().value("user_login").toString());
		emit Chatters(names);
	},{
		{QUERY_PARAMETER_BROADCASTER_ID,security.AdministratorID()},
		{QUERY_PARAMETER_MODERATOR_ID,security.AdministratorID()},
		{"first","1000"}
	},{
		{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()}
	});
}

void Bot::DispatchArrival(const QString &login)
{
	if (auto viewer=viewers.find(login); viewer != viewers.end())
	{
		if (viewer->second.bot || viewer->second.welcomed) return;
	}
	else
	{
		viewers.insert({login,{}});
	}

	if (!settingArrivalSound) return; // this isn't an error; clearing the setting is how you turn arrival announcements off

	Viewer::Remote *viewer=new Viewer::Remote(security,login);
	connect(viewer,&Viewer::Remote::Print,this,&Bot::Print);
	connect(viewer,&Viewer::Remote::Recognized,viewer,[this](const Viewer::Local &viewer) {
		if (security.Administrator() == viewer.Name() || QDateTime::currentDateTime().toMSecsSinceEpoch()-lastRaid.toMSecsSinceEpoch() < static_cast<qint64>(settingRaidInterruptDuration)) return;
		Viewer::ProfileImage::Remote *profileImage=viewer.ProfileImage();
		connect(profileImage,&Viewer::ProfileImage::Remote::Retrieved,profileImage,[this,viewer](const QImage &profileImage) {
			emit AnnounceArrival(viewer.DisplayName(),profileImage,File::List(settingArrivalSound).Random());
			viewers.at(viewer.Name()).welcomed=true;
			SaveViewerAttributes(false);
		});
		connect(profileImage,&Viewer::ProfileImage::Remote::Print,this,&Bot::Print);
	});
}

void Bot::ParseChatMessage(const QString &prefix,const QString &source,const QStringList &parameters,const QString &message)
{
	using StringViewLookup=std::unordered_map<QStringView,QStringView>;
	using StringViewTakeResult=std::optional<QStringView>;

	std::optional<QStringView> window;

	QStringView remainingText(message);
	QStringView login;
	Chat::Message chatMessage;

	// parse tags
	window=prefix;
	StringViewLookup tags;
	while (!window->isEmpty())
	{
		StringViewTakeResult pair=StringView::Take(*window,';');
		if (!pair) continue; // skip malformated badges rather than making a visible fuss
		StringViewTakeResult key=StringView::Take(*pair,'=');
		if (!key) continue; // same as above
		StringViewTakeResult value=StringView::Last(*pair,'=');
		if (!value) continue; // I'll rely on "missing" to represent an empty value
		tags[*key]=*value;
	}
	if (auto displayName=tags.find(CHAT_TAG_DISPLAY_NAME); displayName != tags.end()) chatMessage.sender=displayName->second.toString();
	if (auto tagColor=tags.find(CHAT_TAG_COLOR); tagColor != tags.end()) chatMessage.color=tagColor->second.toString();

	// badges
	if (auto tagBadges=tags.find(CHAT_TAG_BADGES); tagBadges != tags.end())
	{
		StringViewLookup badges;
		QStringView &versions=tagBadges->second;
		while (!versions.isEmpty())
		{
			StringViewTakeResult pair=StringView::Take(versions,',');
			if (!pair) continue;
			StringViewTakeResult name=StringView::Take(*pair,'/');
			if (!name) continue;
			StringViewTakeResult version=StringView::Last(*pair,'/');
			if (!version) continue; // a badge must have a version
			badges.insert({*name,*version});
			std::optional<QString> badgeIconPath=DownloadBadgeIcon(name->toString(),version->toString());
			if (!badgeIconPath) continue;
			chatMessage.badges.append(*badgeIconPath);
		}
		if (auto badgeBroadcaster=badges.find(CHAT_BADGE_BROADCASTER); badgeBroadcaster != badges.end() && badgeBroadcaster->second == u"1") chatMessage.broadcaster=true;
		if (auto badgeModerator=badges.find(CHAT_BADGE_MODERATOR); badgeModerator != badges.end() && badgeModerator->second == u"1") chatMessage.moderator=true;
	}

	// emotes
	if (auto tagEmotes=tags.find(CHAT_TAG_EMOTES); tagEmotes != tags.end())
	{
		QStringView &entries=tagEmotes->second;
		while (!entries.isEmpty())
		{
			StringViewTakeResult entry=StringView::Take(entries,'/');
			if (!entry) continue;
			StringViewTakeResult id=StringView::Take(*entry,':');
			if (!id) continue;
			while(!entry->isEmpty())
			{
				StringViewTakeResult occurrence=StringView::Take(*entry,',');
				StringViewTakeResult left=StringView::First(*occurrence,'-');
				StringViewTakeResult right=StringView::Last(*occurrence,'-');
				if (!left || !right) continue;
				unsigned int start=StringConvert::PositiveInteger(left->toString());
				unsigned int end=StringConvert::PositiveInteger(right->toString());
				const Chat::Emote emote {
					.id=id->toString(),
					.start=start,
					.end=end
				};
				chatMessage.emotes.push_back(emote);
			}
		}
		std::sort(chatMessage.emotes.begin(),chatMessage.emotes.end());
	}

	window=source;

	// hostmask
	// TODO: break these down in the Channel class, not here
	StringViewTakeResult hostmask=StringView::Take(*window,' ');
	if (!hostmask) return;
	StringViewTakeResult user=StringView::Take(*hostmask,'!');
	if (!user) return;
	login=*user;

	if (login == TWITCH_SYSTEM_ACCOUNT_NAME)
	{
		if (DispatchChatNotification(remainingText.toString())) return;
	}

	// determine if this is a command, and if so, process it as such
	// and if it's valid, we're done
	window=message;
	if (StringViewTakeResult command=StringView::Take(*window,' '); command)
	{
		if (command->size() > 0 && command->at(0) == '!')
		{
			chatMessage.text=window->toString().trimmed();
			if (DispatchCommand(command->mid(1).toString(),chatMessage,login.toString())) return;
		}
	}

	if (!chatMessage.broadcaster) DispatchArrival(login.toString());

	// determine if the message is an action
	remainingText=remainingText.trimmed();
	if (const QString ACTION("\001ACTION"); remainingText.startsWith(ACTION))
	{
		remainingText=remainingText.mid(ACTION.size(),remainingText.lastIndexOf('\001')-ACTION.size()).trimmed();
		chatMessage.action=true;
	}

	// set emote name and check for wall of text
	int emoteCharacterCount=0;
	for (Chat::Emote &emote : chatMessage.emotes)
	{
		const QStringView name=remainingText.mid(emote.start,1+emote.end-emote.start); // end is an index, not a size, so we have to add 1 to get the size
		emoteCharacterCount+=name.size();
		emote.name=name.toString();
		DownloadEmote(emote); // once we know the emote name, we can determine the path, which means we can download it (download will set the path in the struct)
	}
	if (remainingText.size()-emoteCharacterCount > static_cast<int>(settingTextWallThreshold)) emit AnnounceTextWall(message,settingTextWallSound);

	chatMessage.text=remainingText.toString().toHtmlEscaped();
	emit ChatMessage(chatMessage);
	inactivityClock.start();
}

std::optional<QString> Bot::DownloadBadgeIcon(const QString &badge,const QString &version)
{
	auto badgeIconVersions=badgeIconURLs.find(badge);
	if (badgeIconVersions == badgeIconURLs.end()) return std::nullopt;
	auto badgeIconVersion=badgeIconVersions->second.find(version);
	if (badgeIconVersion == badgeIconVersions->second.end()) return std::nullopt;
	const QString badgeIconURL=badgeIconVersion->second;
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

bool Bot::DispatchCommand(const QString name,const Chat::Message &chatMessage,const QString &login)
{
	auto candidate=commands.find(name);
	if (candidate == commands.end()) return false;
	const Command &command=candidate->second;

	if (command.Protected() && !chatMessage.Privileged())
	{
		emit AnnounceDeniedCommand(File::List(settingDeniedCommandVideo).Random());
		emit Print(QString(R"(The command "!%1" is protected but requester is not authorized")").arg(command.Name()));
		return false;
	}

	if (command.Type() == CommandType::NATIVE && nativeCommandFlags.at(command.Name()) == NativeCommandFlag::HTML)
	{
		emit ChatMessage({
			.sender=chatMessage.sender,
			.text=chatMessage.text,
			.color=chatMessage.color,
			.badges=chatMessage.badges,
			.emotes={},
			.action=chatMessage.action,
			.broadcaster=chatMessage.broadcaster,
			.moderator=chatMessage.moderator
		});
		return true;
	}

	DispatchCommand(chatMessage.text.isEmpty() ? command : Command{command,chatMessage.text},login);
	return true;
}

void Bot::DispatchCommand(const Command &command,const QString &login)
{
	Viewer::Remote *viewer=new Viewer::Remote(security,login);
	connect(viewer,&Viewer::Remote::Print,this,&Bot::Print);
	connect(viewer,&Viewer::Remote::Recognized,viewer,[this,command](const Viewer::Local &viewer) {
		switch (command.Type())
		{
		case CommandType::VIDEO:
			DispatchVideo(command);
			break;
		case CommandType::AUDIO:
			emit PlayAudio(viewer.DisplayName(),command.Message(),command.Path());
			break;
		case CommandType::PULSAR:
			emit Pulse(command.Message());
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
				DispatchFollowage(viewer);
				break;
			case NativeCommandFlag::PANIC:
				DispatchPanic(viewer.DisplayName());
				break;
			case NativeCommandFlag::SHOUTOUT:
				DispatchShoutout(command);
				break;
			case NativeCommandFlag::SONG:
				emit ShowCurrentSong(vibeKeeper.SongTitle(),vibeKeeper.AlbumTitle(),vibeKeeper.AlbumArtist(),vibeKeeper.AlbumCoverArt());
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
	// FIXME: What if there are no videos in the directory?
	emit PlayVideo(command.File());
}

void Bot::DispatchCommandList()
{
	std::vector<std::tuple<QString,QStringList,QString>> descriptions;
	for (const std::pair<const QString,Command> &pair : commands)
	{
		const Command &command=pair.second;
		if (command.Parent() || command.Protected()) continue;
		QStringList aliases;
		const std::vector<Command*> &children=command.Children();
		for (const Command *child : children) aliases.append(child->Name());
		descriptions.push_back({command.Name(),aliases,command.Description()});
	}
	emit ShowCommandList(descriptions);
}

void Bot::DispatchFollowage(const Viewer::Local &viewer)
{
	Network::Request({TWITCH_API_ENDPOINT_USER_FOLLOWS},Network::Method::GET,[this,viewer](QNetworkReply *reply) {
		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_JSON_PARSE).arg(TWITCH_API_OPERATION_USER_FOLLOWS,parsedJSON.error));
			return;
		}

		const QJsonObject object=parsedJSON().object();
		auto jsonFieldData=object.find(JSON_KEY_DATA);
		if (jsonFieldData == object.end())
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_UNKNOWN).arg(TWITCH_API_OPERATION_USER_FOLLOWS));
			return;
		}

		const QJsonArray jsonUsers=jsonFieldData->toArray();
		if (jsonUsers.size() < 1)
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_INCOMPLETE).arg(TWITCH_API_OPERATION_USER_FOLLOWS));
			return;
		}

		const QJsonObject jsonUserDetails=jsonUsers.at(0).toObject(); // because I fill in both from_id and to_id fields, there is only one result, so just grab the first one out of the array
		auto jsonFieldFollowDate=jsonUserDetails.find("followed_at");
		if (jsonFieldFollowDate == jsonUserDetails.end())
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_INCOMPLETE).arg(TWITCH_API_OPERATION_USER_FOLLOWS));
			return;
		}
		const QDateTime start=QDateTime::fromString(jsonFieldFollowDate->toString(),Qt::ISODate);
		std::chrono::milliseconds duration=static_cast<std::chrono::milliseconds>(start.msecsTo(QDateTime::currentDateTimeUtc()));
		std::chrono::years years=std::chrono::duration_cast<std::chrono::years>(duration);
		std::chrono::months months=std::chrono::duration_cast<std::chrono::months>(duration-years);
		std::chrono::days days=std::chrono::duration_cast<std::chrono::days>(duration-years-months);
		emit ShowFollowage(viewer.DisplayName(),years,months,days);
	},{
		{"from_id",viewer.ID()},
		{"to_id",security.AdministratorID()}
	},{
		{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
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
	connect(viewer,&Viewer::Remote::Recognized,viewer,[this](const Viewer::Local &viewer) {
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
		switch (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
		{
		case 400:
			emit Print(QStringLiteral("Invalid or missing type query parameter"),TWITCH_API_OPERATION_STREAM_INFORMATION);
			return;
		case 401:
			emit Print(TWITCH_API_ERROR_AUTH,TWITCH_API_OPERATION_STREAM_INFORMATION);
			return;
		}

		if (reply->error())
		{
			emit Print(QStringLiteral("Failed to obtain stream information"),TWITCH_API_OPERATION_STREAM_INFORMATION);
			return;
		}

		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_JSON_PARSE).arg(TWITCH_API_OPERATION_STREAM_INFORMATION,parsedJSON.error));
			return;
		}

		const QJsonObject object=parsedJSON().object();
		auto jsonFieldData=object.find(JSON_KEY_DATA);
		if (jsonFieldData == object.end())
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_UNKNOWN).arg(TWITCH_API_OPERATION_STREAM_INFORMATION));
			return;
		}

		const QJsonArray jsonUsers=jsonFieldData->toArray();
		if (jsonUsers.size() < 1)
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_INCOMPLETE).arg(TWITCH_API_OPERATION_STREAM_INFORMATION));
			return;
		}

		const QJsonObject details=jsonUsers.at(0).toObject(); // we're only specifying one user ID so just grab the first result in the array
		auto jsonFieldStartDate=details.find("started_at");
		if (jsonFieldStartDate == details.end())
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_INCOMPLETE).arg(TWITCH_API_OPERATION_STREAM_INFORMATION));
			return;
		}

		const QDateTime start=QDateTime::fromString(jsonFieldStartDate->toString(),Qt::ISODate);
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
	if (vibeKeeper.Playing())
	{
		emit Print("Pausing the vibes...");
		vibeKeeper.Stop();
		return;
	}
	vibeKeeper.Start();
}

void Bot::AdjustVibeVolume(Command command)
{
	try
	{
		QStringView window(command.Message());
		std::optional<QStringView> targetVolume=StringView::Take(window,' ');
		if (!targetVolume) throw std::runtime_error("Target volume is missing");
		if (window.size() < 1) throw std::runtime_error("Duration for volume change is missing");
		vibeKeeper.Volume(StringConvert::PositiveInteger(targetVolume->toString()),std::chrono::seconds(StringConvert::PositiveInteger(window.toString())));
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(QString("%1: %2").arg("Failed to adjust volume",exception.what()));
	}
}

void Bot::ToggleEmoteOnly()
{
	Network::Request({TWITCH_API_ENDPOINT_CHAT_SETTINGS},Network::Method::GET,[this](QNetworkReply *reply) {
		switch (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
		{
		case 400:
			emit Print(QStringLiteral("Missing broadcaster_id query parameter"),TWITCH_API_OPERATION_EMOTE_ONLY);
			return;
		case 401:
			emit Print(TWITCH_API_ERROR_AUTH,TWITCH_API_OPERATION_EMOTE_ONLY);
			return;
		}

		if (reply->error())
		{
			emit Print(QStringLiteral("Failed to toggle emote-only chat"),TWITCH_API_OPERATION_EMOTE_ONLY);
			return;
		}

		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_JSON_PARSE).arg(TWITCH_API_OPERATION_EMOTE_ONLY,parsedJSON.error));
			return;
		}

		const QJsonObject object=parsedJSON().object();
		auto jsonFieldData=object.find(JSON_KEY_DATA);
		if (jsonFieldData == object.end())
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_UNKNOWN).arg(TWITCH_API_OPERATION_EMOTE_ONLY));
			return;
		}

		const QJsonArray details=jsonFieldData->toArray();
		if (details.size() < 1)
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_INCOMPLETE).arg(TWITCH_API_OPERATION_EMOTE_ONLY));
			return;
		}

		const QJsonObject fields=details.at(0).toObject();
		auto jsonFieldEmoteMode=fields.find("emote_mode");
		if (jsonFieldEmoteMode == fields.end())
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_INCOMPLETE).arg(TWITCH_API_OPERATION_EMOTE_ONLY));
			return;
		}

		if (jsonFieldEmoteMode->toBool())
			EmoteOnly(false);
		else
			EmoteOnly(true);
	},{
		{QUERY_PARAMETER_BROADCASTER_ID,security.AdministratorID()},
		{QUERY_PARAMETER_MODERATOR_ID,security.AdministratorID()}
	},{
		{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
	});
}

void Bot::EmoteOnly(bool enable)
{
	// just use broadcasterID for both
	// this is for authorizing the moderator at the API-level, which we're not worried about here
	// it's always going to be the broadcaster running the bot and access is protected through
	// DispatchCommand()
	Network::Request({TWITCH_API_ENDPOINT_CHAT_SETTINGS},Network::Method::PATCH,[this](QNetworkReply *reply) {
		if (reply->error()) emit Print(QString("Something went wrong setting emote only: %1").arg(reply->errorString()));
	},{
		{QUERY_PARAMETER_BROADCASTER_ID,security.AdministratorID()},
		{QUERY_PARAMETER_MODERATOR_ID,security.AdministratorID()}
	},{
		{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
	},QJsonDocument(QJsonObject({{"emote_mode",enable}})).toJson(QJsonDocument::Compact));
}

void Bot::StreamTitle(const QString &title)
{
	Network::Request({TWITCH_API_ENDPOINT_CHANNEL_INFORMATION},Network::Method::PATCH,[this,title](QNetworkReply *reply) {
		switch (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
		{
		case 400:
			emit Print(QStringLiteral("A query parameter is invalid"),TWITCH_API_OPERATION_STREAM_TITLE);
			return;
		case 401:
			emit Print("Missing or invalid OAuth token, client ID, or broadcaster ID; or bot does not have required scope (channel:manage:broadcast)",TWITCH_API_OPERATION_STREAM_TITLE);
			return;
		case 500:
			emit Print("Something when wrong on Twitch's end",TWITCH_API_OPERATION_STREAM_TITLE);
			return;
		}

		if (reply->error() || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 204)
		{
			emit Print(QStringLiteral("Failed to change stream title"),TWITCH_API_OPERATION_STREAM_TITLE);
			return;
		}

		emit Print(QString(R"(Stream title changed to "%1")").arg(title),TWITCH_API_OPERATION_STREAM_TITLE);
	},{
		{QUERY_PARAMETER_BROADCASTER_ID,security.AdministratorID()}
	},{
		{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
	},
	{
		QJsonDocument(QJsonObject({{"title",title}})).toJson(QJsonDocument::Compact)
	});
}

void Bot::StreamCategory(const QString &category)
{
	Network::Request({TWITCH_API_ENDPOINT_GAME_INFORMATION},Network::Method::GET,[this,category](QNetworkReply *reply) {
		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			Print (QString(TWITCH_API_ERROR_TEMPLATE_JSON_PARSE).arg(TWITCH_API_OPERATION_STREAM_CATEGORY,parsedJSON.error));
			return;
		}

		const QJsonObject object=parsedJSON().object();
		auto jsonFieldData=object.find(JSON_KEY_DATA);
		if (jsonFieldData == object.end())
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_UNKNOWN).arg(TWITCH_API_OPERATION_STREAM_CATEGORY));
			return;
		}

		const QJsonArray details=jsonFieldData->toArray();
		if (details.size() < 1)
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_INCOMPLETE).arg(TWITCH_API_OPERATION_STREAM_CATEGORY));
			return;
		}

		const QJsonObject fields=details.at(0).toObject();
		auto jsonFieldID=fields.find("id");
		if (jsonFieldID == fields.end())
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_INCOMPLETE).arg(TWITCH_API_OPERATION_STREAM_CATEGORY));
			return;
		}

		const QString categoryID=jsonFieldID->toString();
		Network::Request({TWITCH_API_ENDPOINT_CHANNEL_INFORMATION},Network::Method::PATCH,[this,category,categoryID](QNetworkReply *reply) {
			if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 204)
			{
				emit Print("Failed to change stream category");
				return;
			}
			emit Print(QString(R"(Stream category changed to "%1")").arg(category));
		},{
			{QUERY_PARAMETER_BROADCASTER_ID,security.AdministratorID()}
		},{
			{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
			{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
			{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
		},
		{
			QJsonDocument(QJsonObject({{"game_id",categoryID}})).toJson(QJsonDocument::Compact)
		});
	},{
		{"name",category}
	},{
		{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
	});
}

std::optional<CommandType> Bot::ValidCommandType(const QString &type)
{
	auto candidate=COMMAND_TYPE_LOOKUP.find(type);
	if (candidate == COMMAND_TYPE_LOOKUP.end())
	{
		emit Print(QString("Command type '%1' doesn't exist").arg(candidate->first));
		return std::nullopt;
	}
	return candidate->second;
}

ApplicationSetting& Bot::ArrivalSound()
{
	return settingArrivalSound;
}

ApplicationSetting& Bot::PortraitVideo()
{
	return settingPortraitVideo;
}

ApplicationSetting& Bot::CheerVideo()
{
	return settingCheerVideo;
}

ApplicationSetting& Bot::SubscriptionSound()
{
	return settingSubscriptionSound;
}

ApplicationSetting& Bot::RaidSound()
{
	return settingRaidSound;
}

ApplicationSetting& Bot::InactivityCooldown()
{
	return settingInactivityCooldown;
}

ApplicationSetting& Bot::HelpCooldown()
{
	return settingHelpCooldown;
}

ApplicationSetting& Bot::TextWallThreshold()
{
	return settingTextWallThreshold;
}

ApplicationSetting& Bot::TextWallSound()
{
	return settingTextWallSound;
}