#include <QFile>

#include <QApplication>
#include <QTimeZone>
#include <QNetworkReply>
#include <ranges>
#include "bot.h"
#include "globals.h"
#include "network.h"
#include "twitch.h"

const char *COMMANDS_LIST_FILENAME="commands.json";
const char *COMMAND_TYPE_NATIVE="native";
const char *COMMAND_TYPE_AUDIO="announce";
const char *COMMAND_TYPE_VIDEO="video";
const char *COMMAND_TYPE_PULSAR="pulsar";
const char COMMAND_PREFIX='!';
const char *VIEWER_ATTRIBUTES_FILENAME="viewers.json";
const char *VIEWER_ATTRIBUTES_ERROR="Failed to add viewer to list of viewers";
const char *VIBE_PLAYLIST_FILENAME="songs.json";
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
const char *JSON_KEY_COMMAND_VIEWERS="viewers";
const char *JSON_KEY_COMMANDS="commands";
const char *JSON_KEY_WELCOME="welcomed";
const char *JSON_KEY_BOT="bot";
const char *JSON_KEY_LIMIT_COMMANDS="limited";
const char *JSON_KEY_SUBSCRIBED="subscribed";
const char *JSON_ARRAY_EMPTY="[]";
const char *SETTINGS_CATEGORY_VIBE="Vibe";
const char *SETTINGS_CATEGORY_COMMANDS="Commands";
const char *SETTINGS_CATEGORY_EVENTS="Events";
const char *TWITCH_API_OPERATION_STREAM_INFORMATION="stream information";
const char *TWITCH_API_OPERATION_USER_FOLLOWS="user follow details";
const char *TWITCH_API_OPERATION_EMOTE_ONLY="emote only";
const char *TWITCH_API_OPERATION_STREAM_TITLE="stream title";
const char *TWITCH_API_OPERATION_STREAM_CATEGORY="stream category";
const char *TWITCH_API_OPERATION_LOAD_BADGES="badges";
const char *TWITCH_API_OPERATION_SHOUTOUT="shoutout";
const char *TWITCH_API_ERROR_TEMPLATE_INCOMPLETE="Response from requesting %1 was incomplete";
const char *TWITCH_API_ERROR_TEMPLATE_UNKNOWN="Something went wrong obtaining %1";
const char *TWITCH_API_ERROR_TEMPLATE_JSON_PARSE="Error parsing %1 JSON: %2";
const char *TWITCH_API_ERROR_AUTH="Auth token or client ID missing or invalid";
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

const Bot::CommandTypeLookup Bot::COMMAND_TYPE_LOOKUP={
	{COMMAND_TYPE_NATIVE,CommandType::NATIVE},
	{COMMAND_TYPE_VIDEO,CommandType::VIDEO},
	{COMMAND_TYPE_AUDIO,CommandType::AUDIO},
	{COMMAND_TYPE_PULSAR,CommandType::PULSAR}
};

Bot::BadgeIconURLsLookup Bot::badgeIconURLs;
std::chrono::milliseconds Bot::launchTimestamp=TimeConvert::Now();

Bot::Bot(Music::Player &musicPlayer,Security &security,QObject *parent) : QObject(parent),
	vibeKeeper(musicPlayer),
	roaster(false,100,this),
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
	settingDeniedCommandVideo(SETTINGS_CATEGORY_COMMANDS,"Denied"),
	settingCommandCooldown(SETTINGS_CATEGORY_COMMANDS,"Cooldown",10), // in minutes
	settingUptimeHistory(SETTINGS_CATEGORY_COMMANDS,"UptimeHistory",0),
	settingCommandNameAgenda(SETTINGS_CATEGORY_COMMANDS,"Agenda","agenda"),
	settingCommandNameStreamCategory(SETTINGS_CATEGORY_COMMANDS,"StreamCategory","category"),
	settingCommandNameStreamTitle(SETTINGS_CATEGORY_COMMANDS,"StreamTitle","title"),
	settingCommandNameCommands(SETTINGS_CATEGORY_COMMANDS,"Commands","commands"),
	settingCommandNameEmote(SETTINGS_CATEGORY_COMMANDS,"EmoteOnly","emote"),
	settingCommandNameFollowage(SETTINGS_CATEGORY_COMMANDS,"Followage","followage"),
	settingCommandNameHTML(SETTINGS_CATEGORY_COMMANDS,"HTML","html"),
	settingCommandNameLimit(SETTINGS_CATEGORY_COMMANDS,"Limit","limit"),
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
	DeclareCommand({settingCommandNameLimit,"Limit frequency of viewer's commands with a cooldown",CommandType::NATIVE,true},NativeCommandFlag::LIMIT);
	DeclareCommand({settingCommandNamePanic,"Crash Celeste",CommandType::NATIVE,true},NativeCommandFlag::PANIC);
	DeclareCommand({settingCommandNameShoutout,"Call attention to another streamer's channel",CommandType::NATIVE,false},NativeCommandFlag::SHOUTOUT);
	DeclareCommand({settingCommandNameSong,"Show the title, album, and artist of the song that is currently playing",CommandType::NATIVE,false},NativeCommandFlag::SONG);
	DeclareCommand({settingCommandNameTimezone,"Display the timezone of the system the bot is running on",CommandType::NATIVE,false},NativeCommandFlag::TIMEZONE);
	DeclareCommand({settingCommandNameUptime,"Show how long the bot has been connected",CommandType::NATIVE,false},NativeCommandFlag::UPTIME);
	DeclareCommand({settingCommandNameTotalTime,"Show how many total hours stream has ever been live",CommandType::NATIVE,false},NativeCommandFlag::TOTAL_TIME);
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
				Container::Resolve(jsonObject,JSON_KEY_COMMAND_VIEWERS,{}).toVariant().toStringList(),
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
	for (const auto& [name,command] : entries)
	{
		if (command.Parent())
		{
			aliases[command.Parent()->Name()].push_back(name);
			continue; // if this is just an alias, move on without processing it as a full command
		}

		QJsonObject object;
		object.insert(JSON_KEY_COMMAND_NAME,name);

		switch (command.Type())
		{
		case CommandType::BLANK:
			throw std::logic_error("UI should not allow empty command type");
		case CommandType::NATIVE:
			mergedNativeCommandFlags.insert(nativeCommandFlags.extract(name));
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
			if (!command.Viewers().empty()) object.insert(JSON_KEY_COMMAND_VIEWERS,QJsonArray::fromStringList(command.Viewers()));
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
			if (!command.Viewers().empty()) object.insert(JSON_KEY_COMMAND_VIEWERS,QJsonArray::fromStringList(command.Viewers()));
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
			const QStringList nodes=aliases.extract(candidate).mapped();
			for (const QString &alias : nodes) names.append(alias);
			object.insert(JSON_KEY_COMMAND_ALIASES,names);
		}
		value=object;
	}

	// catch the aliases that were for native commands,
	// which normally aren't listed in the commands file
	for (const auto& [commandName,aliasNames] : aliases)
	{
		QJsonArray namesArray;
		for (const QString &aliasName : aliasNames) namesArray.append(aliasName);
		array.append(QJsonObject({
			{JSON_KEY_COMMAND_NAME,commandName},
			{JSON_KEY_COMMAND_ALIASES,namesArray}
		}));
	}

	commands=entries;
	nativeCommandFlags.swap(mergedNativeCommandFlags);

	return QJsonDocument(array);
}

bool Bot::SaveDynamicCommands(const QJsonDocument &json)
{
	QFile commandListFile(Filesystem::DataPath().filePath(COMMANDS_LIST_FILENAME));
	bool result=true;

	try
	{
		if (!commandListFile.open(QIODevice::WriteOnly)) throw std::runtime_error(FILE_OPERATION_OPEN);
		if (commandListFile.write(json.toJson(QJsonDocument::Indented)) <= 0) throw std::runtime_error(FILE_OPERATION_WRITE);
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(QString{FILE_ERROR_TEMPLATE_COMMANDS_LIST}.arg(exception.what(),commandListFile.fileName()),"save dynamic commands");
		result=false;
	}

	commandListFile.close();
	return result;
}

void Bot::StageRedemptionCommand(const QString &name,const QJsonObject &jsonObject)
{
	std::optional<CommandType> type=ValidCommandType(jsonObject.value(JSON_KEY_COMMAND_TYPE).toString());
	if (!type) return;

	redemptions.try_emplace(name,
		name,
		jsonObject.value(JSON_KEY_COMMAND_DESCRIPTION).toString(),
		*type,
		Container::Resolve(jsonObject,JSON_KEY_COMMAND_RANDOM_PATH,false).toBool(),
		Container::Resolve(jsonObject,JSON_KEY_COMMAND_DUPLICATES,true).toBool(),
		jsonObject.value(JSON_KEY_COMMAND_PATH).toString(),
		Command::FileListFilters(*type),
		jsonObject.contains(JSON_KEY_COMMAND_MESSAGE) ? jsonObject.value(JSON_KEY_COMMAND_MESSAGE).toString() : QString(),
		QStringList(),
		false
	);
}

const Command::Lookup& Bot::Commands() const
{
	return commands;
}

bool Bot::LoadViewerAttributes() // FIXME: have this throw an exception rather than return a bool
{
	QFile viewerAttributesFile(Filesystem::DataPath().filePath(VIEWER_ATTRIBUTES_FILENAME));
	if (!viewerAttributesFile.exists()) return true; // a non-existent attributes file is valid if this is a first run

	if (!viewerAttributesFile.open(QIODevice::ReadOnly))
	{
		emit Print(QString("Failed to open viewer attributes file: %1").arg(viewerAttributesFile.fileName()));
		return false;
	}

	QJsonParseError jsonError;
	QByteArray data=viewerAttributesFile.readAll();
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
			.commands=Container::Resolve(attributes,JSON_KEY_COMMANDS,true).toBool(),
			.welcomed=Container::Resolve(attributes,JSON_KEY_WELCOME,false).toBool(),
			.bot=Container::Resolve(attributes,JSON_KEY_BOT,false).toBool(),
			.limited=Container::Resolve(attributes,JSON_KEY_LIMIT_COMMANDS,false).toBool(),
			.subscribed=Container::Resolve(attributes,JSON_KEY_SUBSCRIBED,false).toBool(),
			.commandTimestamp=std::chrono::system_clock::now()-std::chrono::minutes(static_cast<qint64>(settingCommandCooldown))
		};
	}

	return true;
}

void Bot::SaveViewerAttributes(bool reset)
{
	QFile viewerAttributesFile(Filesystem::DataPath().filePath(VIEWER_ATTRIBUTES_FILENAME));
	if (!viewerAttributesFile.open(QIODevice::WriteOnly)) return; // FIXME: how can we report the error here while closing?

	QJsonObject entries;
	for (const auto& [name,attributes] : viewers)
	{
		entries.insert(name,QJsonObject{
			{JSON_KEY_COMMANDS,attributes.commands},
			{JSON_KEY_WELCOME,reset ? false : attributes.welcomed},
			{JSON_KEY_BOT,attributes.bot},
			{JSON_KEY_LIMIT_COMMANDS,attributes.limited},
			{JSON_KEY_SUBSCRIBED,reset ? false : attributes.subscribed}
		});
	}

	viewerAttributesFile.write(QJsonDocument(entries).toJson(QJsonDocument::Indented));
}

File::List Bot::DeserializeVibePlaylist(const QJsonDocument &json)
{
	return {json.toVariant().toStringList()};
}

const File::List& Bot::SetVibePlaylist(const File::List &files)
{
	vibeKeeper.Sources(files);
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
	QFile songListFile(Filesystem::DataPath().filePath(VIBE_PLAYLIST_FILENAME));
	bool result=true;

	try
	{
		if (!songListFile.open(QIODevice::WriteOnly)) throw std::runtime_error(FILE_OPERATION_OPEN);
		if (songListFile.write(json.toJson(QJsonDocument::Indented)) <= 0) throw std::runtime_error(FILE_OPERATION_WRITE);
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(QString{FILE_ERROR_TEMPLATE_VIBE_PLAYLIST}.arg(exception.what(),songListFile.fileName()),"save vibe playlist");
		result=false;
	}

	songListFile.close();
	return result;
}

void Bot::LoadRoasts()
{
	connect(&roaster,&Music::Player::Print,this,&Bot::Print);
	roaster.Sources(File::List{static_cast<QString>(settingRoasts),Command::FileListFilters(CommandType::AUDIO)});
}

void Bot::LoadBadgeIconURLs()
{
	Network::Request::Send({Twitch::Endpoint(Twitch::ENDPOINT_BADGES)},Network::Method::GET,[this](QNetworkReply *reply) {
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
		auto jsonFieldData=object.find(JSON::Keys::DATA);
		if (jsonFieldData == object.end()) return;
		const QJsonArray jsonFieldArray=jsonFieldData->toArray();
		for (const QJsonValue &set : jsonFieldArray)
		{
			const QJsonObject objectBadgeSet=set.toObject();
			auto jsonFieldSetID=objectBadgeSet.find(JSON_KEY_SET_ID);
			auto jsonFieldVersions=objectBadgeSet.find(JSON_KEY_VERSIONS);
			if (jsonFieldSetID == objectBadgeSet.end() || jsonFieldVersions == objectBadgeSet.end()) return;
			const QJsonArray jsonFieldVersionsArray=jsonFieldVersions->toArray();
			for (const QJsonValue &version : jsonFieldVersionsArray)
			{
				const QJsonObject objectImageVersions=version.toObject();
				auto jsonFieldID=objectImageVersions.find(JSON_KEY_ID);
				auto jsonFieldVersionURL=objectImageVersions.find(JSON_KEY_IMAGE_URL);
				if (jsonFieldID == objectImageVersions.end() || jsonFieldVersionURL == objectImageVersions.end()) return;
				badgeIconURLs[jsonFieldSetID->toString()][jsonFieldID->toString()]=jsonFieldVersionURL->toString(); // NOTE: I'm not sure how to construct in place here
			}
		}
	},{},{
		{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.OAuthToken())},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()}
	});
}

void Bot::StartClocks()
{
	inactivityClock.setInterval(TimeConvert::Interval(std::chrono::milliseconds(settingInactivityCooldown)));
	if (settingRoasts) connect(&inactivityClock,&QTimer::timeout,&roaster,QOverload<>::of(&Music::Player::Start));
	inactivityClock.start();

	helpClock.setInterval(TimeConvert::Interval(std::chrono::milliseconds(settingHelpCooldown)));
	connect(&helpClock,&QTimer::timeout,this,&Bot::DispatchHelpText);
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
		DispatchCommandViaCommandObject(command,login);
		return;
	}

	emit AnnounceRedemption(name,rewardTitle,message);
}

void Bot::Subscription(const QString &login,const QString &displayName)
{
	std::unordered_map<QString,Viewer::Attributes>::iterator viewer;
	if (auto candidate=viewers.find(login); candidate != viewers.end())
	{
		if (candidate->second.subscribed) return;
		viewer=candidate;
	}
	else
	{
		auto inserted=viewers.insert({login,{}});
		if (!inserted.second)
		{
			emit Print(VIEWER_ATTRIBUTES_ERROR,u"announce subscription"_s);
			return;
		}
		viewer=inserted.first;
	}

	if (static_cast<QString>(settingSubscriptionSound).isEmpty())
	{
		emit Print("No audio path set for subscriptions","announce subscription");
		return;
	}
	emit AnnounceSubscription(displayName,settingSubscriptionSound);
	viewer->second.subscribed=true;
}

void Bot::Raid(const QString &viewer,const unsigned int viewers)
{
	lastRaid=QDateTime::currentDateTime();
	if (settingRaidSound) emit AnnounceRaid(viewer,viewers,settingRaidSound);
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

void Bot::DispatchArrival(const QString &login)
{
	if (auto viewer=viewers.find(login); viewer != viewers.end())
	{
		// if the viewer is a bot or is already welcomed, bail
		if (viewer->second.bot || viewer->second.welcomed) return;
	}
	else
	{
		// we've never seen this person before, so add a new object to represent them
		if (!viewers.insert({login,{}}).second)
		{
			emit Print(VIEWER_ATTRIBUTES_ERROR,u"announce arrival"_s);
			return;
		}
	}

	// viewer (whether they've been seen before or not) hasn't been welcomed yet
	Viewer::Remote *viewer=new Viewer::Remote(security,login);
	connect(viewer,&Viewer::Remote::Print,this,&Bot::Print);
	connect(viewer,&Viewer::Remote::Recognized,viewer,[this](const Viewer::Local &viewer) {
		if (security.Administrator() == viewer.Name() || QDateTime::currentDateTime().toMSecsSinceEpoch()-lastRaid.toMSecsSinceEpoch() < static_cast<qint64>(settingRaidInterruptDuration)) return;
		Viewer::ProfileImage::Remote *profileImage=viewer.ProfileImage();
		connect(profileImage,&Viewer::ProfileImage::Remote::Retrieved,profileImage,[this,viewer](std::shared_ptr<QImage> profileImage) {
			// Do we have a sound configured to announce them with? If so, fire the signal.
			if (settingArrivalSound) emit AnnounceArrival(viewer.DisplayName(),profileImage,File::List(settingArrivalSound).Random());

			// Do we have any commands that are triggered by the viewers we've seen?
			for (const Command &candidateCommand : commands | std::views::values | std::views::filter([](const Command &command) {
				return !command.Viewers().isEmpty();
			}))
			{
				// look through the list of viewers attached to the command
				// we're looking for when all of the viewers in the list have been welcomed _except_ the one that just arrived
				bool triggerViewerIsCandidate=false;
				if (std::all_of(candidateCommand.Viewers().begin(),candidateCommand.Viewers().end(),[&triggerViewerIsCandidate,&triggerViewer=viewer,this](const QString &name) {
					auto candidateViewer=viewers.find(name);
					if (candidateViewer != viewers.end()) // if the name from the command is in the list of names we've seen in the channel
					{
						// is this the triggering viewer?
						if (triggerViewer.Name() == candidateViewer->first)
						{
							// if so, we only want to act if they haven't been welcomed yet
							if (candidateViewer->second.welcomed) return false;
							triggerViewerIsCandidate=true;
						}
						else
						{
							// otherwise, we want to make sure this person has been welcomed already
							if (!candidateViewer->second.welcomed) return false;
						}
						return true;
					}
					return false;
				}) && triggerViewerIsCandidate) emit DispatchCommandViaCommandObject(candidateCommand,security.Administrator());
			}

			// save the viewer object and its attributes, marking it as welcomed
			viewers.at(viewer.Name()).welcomed=true;
			SaveViewerAttributes(false);
			emit Welcomed(viewer.Name());
		});
		connect(profileImage,&Viewer::ProfileImage::Remote::Print,this,&Bot::Print);
	});
}

void Bot::ParseChatMessage(const QString &prefix,const QString &source,const QStringList &parameters,const QString &message)
{
	// As of right now, Twitch doesn't use parameters in front of the chat message
	// so it's always the final parameter, and the final paramater is always the
	// only parameter. This may change in the future, so passing it in and marking it
	// unused to shut the compiler up for now.
	Q_UNUSED(parameters)

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
		tags.try_emplace(*key,*value);
	}
	if (auto displayName=tags.find(CHAT_TAG_DISPLAY_NAME); displayName != tags.end()) chatMessage.displayName=displayName->second.toString();
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
			while (!entry->isEmpty())
			{
				StringViewTakeResult occurrence=StringView::Take(*entry,',');
				StringViewTakeResult left=StringView::First(*occurrence,'-');
				StringViewTakeResult right=StringView::Last(*occurrence,'-');
				if (!left || !right) continue;
				int start=StringConvert::Integer(left->toString());
				int end=StringConvert::Integer(right->toString());
				chatMessage.emotes.emplace_back(Chat::Emote{
					.id=id->toString(),
					.start=start,
					.end=end
				});
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

	// determine if this is a command, and if so, process it as such
	// and if it's valid, we're done
	window=message;
	std::optional<QString> command=ParseCommandIfExists(*window);
	if (command)
	{
		chatMessage.text=window->toString().trimmed();
		DispatchCommandViaChatMessage(*command,chatMessage,login.toString());
		return;
	}

	if (!chatMessage.broadcaster) DispatchArrival(login.toString());

	// determine if the message is an action
	remainingText=remainingText.trimmed();
	if (const QString ACTION("\001ACTION"); remainingText.startsWith(ACTION))
	{
		remainingText=remainingText.mid(ACTION.size(),remainingText.lastIndexOf('\001')-ACTION.size()).trimmed();
		chatMessage.action=true;
	}

	// download emotes (which will set emote names in the process) and check for wall of text
	int emoteCharacterCount=ParseEmoteNamesAndDownloadImages(chatMessage.emotes,remainingText);
	if (remainingText.size()-emoteCharacterCount > static_cast<int>(settingTextWallThreshold) && settingTextWallSound) emit AnnounceTextWall(message,settingTextWallSound);

	chatMessage.text=remainingText.toString();
	emit ChatMessage(std::make_shared<Chat::Message>(chatMessage));
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
		Network::Request::Send(badgeIconURL,Network::Method::GET,[this,badgeIconURL,badgePath](QNetworkReply *downloadReply) {
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

int Bot::ParseEmoteNamesAndDownloadImages(std::vector<Chat::Emote> &emotes,const QStringView &textWindow)
{
	int emoteCharacterCount=0;
	for (Chat::Emote &emote : emotes)
	{
		const QStringView name=textWindow.mid(emote.start,1+emote.end-emote.start); // end is an index, not a size, so we have to add 1 to get the size
		emoteCharacterCount+=name.size();
		emote.name=name.toString();
		DownloadEmote(emote); // once we know the emote name, we can determine the path, which means we can download it (download will set the path in the struct)
	}
	return emoteCharacterCount;
}

void Bot::DownloadEmote(Chat::Emote &emote)
{
	emote.path=Filesystem::TemporaryPath().filePath(QString("%1.png").arg(emote.id));
	if (!QFile(emote.path).exists())
	{
		Network::Request::Send(Twitch::Content(Twitch::ENDPOINT_EMOTES).arg(emote.id),Network::Method::GET,[this,emote](QNetworkReply *downloadReply) {
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

std::optional<QString> Bot::ParseCommandIfExists(QStringView &message)
{
	QStringView command=message.left(message.indexOf(' '));
	if (command.isEmpty() || command.at(0) != COMMAND_PREFIX) return std::nullopt;
	message=message.mid(command.size()+1);
	return {command.trimmed().mid(1).toString()};
}

void Bot::DispatchCommandViaSubsystem(JSON::SignalPayload *response,const QString &name,const QString &login)
{
	// This function provides a way for a subsystem to retrieve the parsed chat
	// message back if that message had a command in it.
	// We use the SignalPayload here as a traffic controller to handle the
	// back-and-forth communication between the subsystems since Qt's signals
	// and slots are one-way.
	const QString message=response->context.toString();
	if (!message.isNull())
	{
		QStringView window{message};
		std::optional<QString> command=ParseCommandIfExists(window);
		if (command)
		{
			// NOTE: there is chat/color for chat color and moderation/moderators for privileged commands if I ever want to implement this
			Chat::Message message{
				.displayName=name,
				.text=window.toString()
			};
			DispatchCommandViaChatMessage(*command,message,login);
			response->context.setValue(message);
		}
	}

	response->Dispatch();
}

bool Bot::DispatchCommandViaChatMessage(const QString &name,Chat::Message chatMessage,const QString &login) // build a command object from a command name and a chat message and forward
{
	// FIRST! determine if command exists
	auto commandCandidate=commands.find(name);
	if (commandCandidate == commands.end()) return false;
	const Command &command=commandCandidate->second;

	// deny command if user must be a mod and isn't
	if (command.Protected() && !chatMessage.Privileged())
	{
		if (const QString file=File::List(settingDeniedCommandVideo).Random(); QFile(file).exists())
			emit AnnounceDeniedCommand(file);
		else
			emit Print("Denial video doesn't exist ("+file+")");

		emit Print(QString(R"(The command "!%1" is protected but requester is not authorized")").arg(command.Name()));
		return false;
	}

	// have the viewer's command privileges been limited?
	if (auto viewerCandidate=viewers.find(login); viewerCandidate != viewers.end())
	{
		Viewer::Attributes &viewer=viewerCandidate->second;
		if (viewer.limited && std::chrono::duration_cast<std::chrono::minutes>(std::chrono::system_clock::now()-viewer.commandTimestamp) < std::chrono::minutes(static_cast<qint64>(settingCommandCooldown)) && !chatMessage.Privileged())
		{
			emit AnnounceDeniedCommand(File::List(settingDeniedCommandVideo).Random());
			return false;
		}
		viewer.commandTimestamp=std::chrono::system_clock::now();
	}

	// command is reformatting text, so feed the formatted chat message back into the system
	if (command.Type() == CommandType::NATIVE && nativeCommandFlags.at(command.Name()) == NativeCommandFlag::HTML)
	{
		int offset=name.size()+QString(COMMAND_PREFIX).size()+1; // the 1 is the space between the command name and the remaining chat message
		for (Chat::Emote &emote : chatMessage.emotes) // since we've stripped off the command name, we need to slide all of the emote indices left
		{
			emote.start-=offset;
			emote.end-=offset;
		}
		ParseEmoteNamesAndDownloadImages(chatMessage.emotes,chatMessage.text);
		chatMessage.html=true;
		emit ChatMessage(std::make_shared<Chat::Message>(chatMessage)); // short circuit by firing off the chat message, but not processing the command any further
		return true;
	}

	DispatchCommandViaCommandObject(chatMessage.text.isEmpty() ? command : Command{command,chatMessage.text},login);
	return true;
}

void Bot::DispatchCommandViaCommandObject(const Command &command,const QString &login)
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
			emit PlayAudio(viewer.DisplayName(),command.Message(),File::List(command.Path()).Random());
			break;
		case CommandType::PULSAR:
			emit Pulse(command.Message(),command.Name());
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
			case NativeCommandFlag::LIMIT:
				ToggleLimitViewer(command.Message());
			case NativeCommandFlag::HTML:
				emit Print("HTML was processed as a command rather than a chat message. This shouldn't happen!");
				break;
			case NativeCommandFlag::PANIC:
				DispatchPanic(viewer.DisplayName());
				break;
			case NativeCommandFlag::SHOUTOUT:
				DispatchShoutout(command);
				break;
			case NativeCommandFlag::SONG:
				if (Music::Metadata metadata=vibeKeeper.Metadata(); !metadata.title.isEmpty() && !metadata.artist.isEmpty() && !metadata.cover.isNull())
				{
					if (metadata.album.isEmpty())
						emit ShowCurrentSong(metadata.title,metadata.artist,metadata.cover);
					else
						emit ShowCurrentSong(metadata.title,metadata.album,metadata.artist,metadata.cover);
				}
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

void Bot::DispatchVideo(Command command)
{
	// FIXME: What if there are no videos in the directory?
	emit PlayVideo(command.File());
}

void Bot::DispatchCommandList()
{
	std::vector<std::tuple<QString,QStringList,QString>> descriptions;
	for (const auto& [name,command] : commands)
	{
		if (command.Parent() || command.Protected()) continue;
		QStringList aliases;
		const std::vector<Command*> &children=command.Children();
		for (const Command *child : children) aliases.append(child->Name());
		descriptions.emplace_back(command.Name(),aliases,command.Description());
	}
	emit ShowCommandList(descriptions);
}

void Bot::DispatchFollowage(const Viewer::Local &viewer)
{
	Network::Request::Send({Twitch::Endpoint(Twitch::ENDPOINT_USER_FOLLOWS)},Network::Method::GET,[this,viewer](QNetworkReply *reply) {
		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			emit Print(QString(TWITCH_API_ERROR_TEMPLATE_JSON_PARSE).arg(TWITCH_API_OPERATION_USER_FOLLOWS,parsedJSON.error));
			return;
		}

		const QJsonObject object=parsedJSON().object();
		auto jsonFieldData=object.find(JSON::Keys::DATA);
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
		{"user_id",viewer.ID()},
		{"broadcaster_id",security.AdministratorID()}
	},{
		{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.OAuthToken())},
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
	DispatchShoutout(QString(command.Message()).remove("@"));
}

void Bot::DispatchShoutout(const QString &streamer)
{
	Viewer::Remote *profile=new Viewer::Remote(security,streamer);
	connect(profile,&Viewer::Remote::Recognized,profile,[this](const Viewer::Local &profile) {
		// native Twitch shoutout
		Network::Request::Send({Twitch::Endpoint(Twitch::ENDPOINT_SHOUTOUTS)},Network::Method::POST,[this,streamerID=profile.ID()](QNetworkReply *reply) {
			// 204 is successful
			switch (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
			{
			case 400:
				emit Print(u"Invalid or missing information in request"_s,TWITCH_API_OPERATION_SHOUTOUT);
				return;
			case 401:
				emit Print(TWITCH_API_ERROR_AUTH,TWITCH_API_OPERATION_SHOUTOUT);
				return;
			case 403:
				emit Print(u"User attempting shoutout is not a moderator"_s,TWITCH_API_OPERATION_SHOUTOUT);
				return;
			case 429:
				emit Print(u"Shoutout feature is still in cooldown"_s,TWITCH_API_OPERATION_SHOUTOUT);
				return;
			}

			if (reply->error())
			{
				emit Print(u"Failed to perform shoutout for unknown reason"_s,TWITCH_API_OPERATION_SHOUTOUT);
				return;
			}
		},{
			{"from_broadcaster_id",security.AdministratorID()},
			{"to_broadcaster_id",profile.ID()},
			{"moderator_id",security.AdministratorID()}
		},{
			{NETWORK_HEADER_AUTHORIZATION,StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.OAuthToken())))},
			{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
			{Network::CONTENT_TYPE,Network::CONTENT_TYPE_FORM} // Error code 400 can also be cause by missing content type
		});
		// bot shoutout
		Viewer::ProfileImage::Remote *profileImage=profile.ProfileImage();
		connect(profileImage,&Viewer::ProfileImage::Remote::Retrieved,profileImage,[this,displayName=profile.DisplayName(),description=profile.Description()](std::shared_ptr<QImage> profileImage) {
			emit Shoutout(displayName,description,profileImage);
		},Qt::QueuedConnection);
	},Qt::QueuedConnection);
	connect(profile,&Viewer::Remote::Print,this,&Bot::Print);
}

void Bot::DispatchUptime(bool total)
{
	Network::Request::Send({Twitch::Endpoint(Twitch::ENDPOINT_STREAM_INFORMATION)},Network::Method::GET,[this,total](QNetworkReply *reply) {
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
		auto jsonFieldData=object.find(JSON::Keys::DATA);
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
		{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.OAuthToken())},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
	});
}

void Bot::DispatchHelpText()
{
	std::vector<const Command*> candidates;
	for (auto candidate=commands.begin(); candidate != commands.end(); ++candidate)
	{
		// NOTE: for some reason I can't completely explain, if I use a range-loop here,
		// I end up with a vector full of the same command
		if (!candidate->second.Protected()) candidates.push_back(&candidate->second);
	}
	const Command *candidate=candidates[Random::Bounded(candidates)];
	emit ShowCommand(candidate->Name(),candidate->Description());
}

void Bot::ToggleLimitViewer(const QString &target)
{
	static const char *OPERATION="LIMIT VIEWER";
	std::unordered_map<QString,Viewer::Attributes>::iterator candidate=viewers.find(target);
	if (candidate == viewers.end())
	{
		emit Print("Could not limit unrecognized viewer",OPERATION);
		return;
	}

	Viewer::Attributes &attributes=candidate->second;
	if (attributes.limited)
	{
		attributes.limited=false;
		emit Print("Unlimiting viewer's command privileges",OPERATION);
	}
	else
	{
		attributes.limited=true;
		emit Print("Limiting viewer's command privileges with cooldown",OPERATION);
	}
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
	Network::Request::Send({Twitch::Endpoint(Twitch::ENDPOINT_CHAT_SETTINGS)},Network::Method::GET,[this](QNetworkReply *reply) {
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
		auto jsonFieldData=object.find(JSON::Keys::DATA);
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
		{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.OAuthToken())},
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
	Network::Request::Send({Twitch::Endpoint(Twitch::ENDPOINT_CHAT_SETTINGS)},Network::Method::PATCH,[this](QNetworkReply *reply) {
		if (reply->error()) emit Print(QString("Something went wrong setting emote only: %1").arg(reply->errorString()));
	},{
		{QUERY_PARAMETER_BROADCASTER_ID,security.AdministratorID()},
		{QUERY_PARAMETER_MODERATOR_ID,security.AdministratorID()}
	},{
		{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.OAuthToken())},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
	},QJsonDocument(QJsonObject({{"emote_mode",enable}})).toJson(QJsonDocument::Compact));
}

void Bot::StreamTitle(const QString &title)
{
	Network::Request::Send({Twitch::Endpoint(Twitch::ENDPOINT_CHANNEL_INFORMATION)},Network::Method::PATCH,[this,title](QNetworkReply *reply) {
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
		{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.OAuthToken())},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
	},
	{
		QJsonDocument(QJsonObject({{"title",title}})).toJson(QJsonDocument::Compact)
	});
}

void Bot::StreamCategory(const QString &category)
{
	Network::Request::Send({Twitch::Endpoint(Twitch::ENDPOINT_GAME_INFORMATION)},Network::Method::GET,[this,category](QNetworkReply *reply) {
		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			Print (QString(TWITCH_API_ERROR_TEMPLATE_JSON_PARSE).arg(TWITCH_API_OPERATION_STREAM_CATEGORY,parsedJSON.error));
			return;
		}

		const QJsonObject object=parsedJSON().object();
		auto jsonFieldData=object.find(JSON::Keys::DATA);
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
		Network::Request::Send({Twitch::Endpoint(Twitch::ENDPOINT_CHANNEL_INFORMATION)},Network::Method::PATCH,[this,category,categoryID](QNetworkReply *reply) {
			if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 204)
			{
				emit Print("Failed to change stream category");
				return;
			}
			emit Print(QString(R"(Stream category changed to "%1")").arg(category));
		},{
			{QUERY_PARAMETER_BROADCASTER_ID,security.AdministratorID()}
		},{
			{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.OAuthToken())},
			{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
			{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON},
		},
		{
			QJsonDocument(QJsonObject({{"game_id",categoryID}})).toJson(QJsonDocument::Compact)
		});
	},{
		{"name",category}
	},{
		{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.OAuthToken())},
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