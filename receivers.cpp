#include <QDir>
#include <QImage>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QFutureSynchronizer>
#include <vector>
#include <stdexcept>
#include "globals.h"
#include "settings.h"
#include "receivers.h"

MessageReceiver::MessageReceiver(QObject *parent) noexcept : QObject(parent)
{
	connect(this,&MessageReceiver::Succeeded,this,&MessageReceiver::deleteLater);
	connect(this,&MessageReceiver::Failed,this,&MessageReceiver::deleteLater);
}

void StatusReceiver::Interpret(const QString text)
{
	emit Print("> "+text);
}

void AuthenticationReceiver::Process(const QString data)
{
	if (data.contains(IRC_VALIDATION_AUTHENTICATION))
	{
		emit Print("Server accepted authentication");
		Interpret(data);
		emit Succeeded();
	}
	else
	{
		emit Print("Server did not accept authentication");
		Interpret(data);
		emit Failed();
	}
}

ChannelJoinReceiver::ChannelJoinReceiver(QObject *parent) noexcept  : StatusReceiver(parent)
{
	failureDelay.setSingleShot(true);
	failureDelay.setInterval(1000); // TODO: Make this an advanced setting
	connect(&failureDelay,&QTimer::timeout,this,&ChannelJoinReceiver::Fail);
}

void ChannelJoinReceiver::Process(const QString data)
{
	if (data.contains(IRC_VALIDATION_JOIN))
	{
		if (failureDelay.isActive()) failureDelay.stop();
		emit Print("Stream joined");
		Interpret(data);
		emit Succeeded();
	}
	else
	{
		if (!failureDelay.isActive()) failureDelay.start();
		Interpret(data);
	}
}

void ChannelJoinReceiver::Fail()
{
	emit Print("Failed to join channel for stream");
	emit Failed();
}

ChatMessageReceiver::ChatMessageReceiver(QObject *parent) : MessageReceiver(parent), downloadManager(new QNetworkAccessManager(this))
{
	QFile commandListFile(Filesystem::DataPath().filePath(COMMANDS_LIST_FILENAME));
	if (!commandListFile.open(QIODevice::ReadWrite)) throw std::runtime_error(QString("Failed to open command list file: %1").arg(commandListFile.fileName()).toStdString());

	QJsonParseError jsonError;
	QByteArray data=commandListFile.readAll();
	if (data.isEmpty()) data="[]";
	QJsonDocument json=QJsonDocument::fromJson(data,&jsonError);
	if (json.isNull()) throw std::runtime_error(jsonError.errorString().toStdString());
	for (const QJsonValue &jsonValue : json.array())
	{
		QJsonObject jsonObject=jsonValue.toObject();
		const QString name=jsonObject.value(JSON_KEY_COMMAND_NAME).toString();
		commands[name]={
			name,
			jsonObject.value(JSON_KEY_COMMAND_DESCRIPTION).toString(),
			COMMAND_TYPES.at(jsonObject.value(JSON_KEY_COMMAND_TYPE).toString()),
			jsonObject.contains(JSON_KEY_COMMAND_RANDOM_PATH) ? jsonObject.value(JSON_KEY_COMMAND_RANDOM_PATH).toBool() : false,
			jsonObject.value(JSON_KEY_COMMAND_PATH).toString(),
			jsonObject.contains(JSON_KEY_COMMAND_MESSAGE) ? jsonObject.value(JSON_KEY_COMMAND_MESSAGE).toString() : QString()
		};
		if (jsonObject.contains(JSON_KEY_COMMAND_ALIASES))
		{
			for (const QJsonValue &value : jsonObject.value(JSON_KEY_COMMAND_ALIASES).toArray()) commandAliases.insert_or_assign(value.toString(),commands.at(name));
		}
	}
	for (const std::pair<QString,Command> &command : commands)
	{
		if (!command.second.Protected()) userCommands.push_back(command.second);
	}
}

void ChatMessageReceiver::AttachCommand(const Command &command)
{
	commands[command.Name()]=command;
}

const Command ChatMessageReceiver::RandomCommand() const
{
	return userCommands[Random::Bounded(userCommands)];
}

void ChatMessageReceiver::Process(const QString data)
{
	QStringList messageSegments;
	messageSegments=data.split(" ",StringConvert::Split::Behavior(StringConvert::Split::Behaviors::KEEP_EMPTY_PARTS));
	if (messageSegments.size() < 2)
	{
		Fail("Message is invalid");
		return;
	}

	// extract some metadata that appears before the message
	TagMap tags=ParseTags(messageSegments.takeFirst());
	if (messageSegments=messageSegments.join(" ").split(":",StringConvert::Split::Behavior(StringConvert::Split::Behaviors::SKIP_EMPTY_PARTS)); messageSegments.size() < 2)
	{
		Fail("Invalid message segment count");
		return;
	}
	QString user=ParseHostmask(messageSegments.takeFirst());
	if (user.isEmpty())
	{
		Fail("Invalid message sender");
		return;
	}
	IdentifyViewer(user);

	// determine if this is a command, and if so, process it as such
	if (messageSegments.at(0).at(0) == "!")
	{
		auto [commandName,parameter]=ParseCommand(messageSegments.at(0));
		if (Command *command=FindCommand(commandName); command)
		{
			if (command->Protected() && settingAdministrator != user)
			{
				emit Alert(QString("The command %1 is protected but %2 is not the broadcaster").arg(command->Name(),user));
				return;
			}
			switch (command->Type())
			{
			case CommandType::VIDEO:
				if (command->Random())
				{
					QDir directory(command->Path());
					QStringList videos=directory.entryList(QStringList() << "*.mp4",QDir::Files);
					if (videos.size() < 1)
					{
						Print("No videos found");
						break;
					}
					emit PlayVideo(directory.absoluteFilePath(videos.at(Random::Bounded(0,videos.size()))));
				}
				else
				{
					emit PlayVideo(command->Path());
				}
				break;
			case CommandType::AUDIO:
				emit PlayAudio(user,command->Message(),command->Path());
				break;
			case CommandType::FORWARD:
				ForwardCommand({*command,parameter});
				break;
			};
			return;
		}
	}

	// determine if the message is an action
	// have to do this here, because the ACTION tag
	// throws off the indicies in processing emotes below
	bool action=false;
	if (messageSegments.front() == "\001ACTION")
	{
		messageSegments.pop_front();
		messageSegments.back().remove('\001');
		action=true;
	}
	QString message=messageSegments.join(":");

	// look for emotes if they exist
	if (tags.find("emotes") != tags.end())
	{
		std::vector<Emote> emotes;
		QStringList entries=tags.at("emotes").split("/",StringConvert::Split::Behavior(StringConvert::Split::Behaviors::SKIP_EMPTY_PARTS));
		for (const QString &entry : entries)
		{
			QStringList details=entry.split(":");
			if (details.size() < 2)
			{
				emit Alert("Malformatted emote in message");
				continue;
			}
			for (const QString &instance : details.at(1).split(","))
			{
				QStringList indicies=instance.split("-");
				if (indicies.size() < 2)
				{
					emit Alert("Cannot determine where emote starts or ends");
					continue;
				}
				unsigned int start=StringConvert::PositiveInteger(indicies.at(0));
				unsigned int end=StringConvert::PositiveInteger(indicies.at(1));
				emotes.push_back({details.at(0),QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(QString("%1.png").arg(details.at(0))),start,end});
			}
		}

		QString emotedMessage;
		std::sort(emotes.begin(),emotes.end());
		unsigned int position=0;
		for (const Emote &emote : emotes)
		{
			if (position < emote.start) emotedMessage+=message.midRef(position,emote.start-position);
			emotedMessage+=QString("<img style='vertical-align: middle;' src='%1' />").arg(DownloadEmote(emote));
			position=emote.end+1;
		}
		if (position < message.size()) emotedMessage+=message.midRef(position,message.size()-position);
		message=emotedMessage;
	}

	// print the final message
	if (action)
		emit Print(QString("<div class='user' style='color: %3;'>%1 <span class='message'>%2</span><br></div>").arg(user,message,tags.at("color")));
	else
		emit Print(QString("<div class='user' style='color: %3;'>%1</div><div class='message'>%2<br></div>").arg(user,message,tags.at("color")));

	emit MessageProcessed();
}

ChatMessageReceiver::TagMap ChatMessageReceiver::ParseTags(const QString &tags)
{
	TagMap result;
	for (const QString &pair : tags.split(";",StringConvert::Split::Behavior(StringConvert::Split::Behaviors::SKIP_EMPTY_PARTS)))
	{
		QStringList components=pair.split("=",StringConvert::Split::Behavior(StringConvert::Split::Behaviors::KEEP_EMPTY_PARTS));
		if (components.size() == 2)
			result[components.at(KEY)]=components.at(VALUE);
		else
			emit Alert(QString("Message has an invalid tag: %1").arg(pair));
	}
	if (result.find("color") == result.end())
	{
		emit Alert("Message has no color");
		result["color"]="#ffffff"; // TODO: this should be the foreground color of the chat pane
	}
	return result;
}

QString ChatMessageReceiver::ParseHostmask(const QString &mask)
{
	QStringList hostmaskSegments=mask.split("!",StringConvert::Split::Behavior(StringConvert::Split::Behaviors::SKIP_EMPTY_PARTS));
	if (hostmaskSegments.size() < 1)
	{
		emit Alert("Message hostmask is invalid");
		return QString();
	}
	return hostmaskSegments.front();
}

std::tuple<QString,QString> ChatMessageReceiver::ParseCommand(const QString &message)
{
	QStringList commandSegments=message.trimmed().split(" ");
	QString commandName=commandSegments.takeFirst().mid(1);
	QString parameter=commandSegments.join(" ");
	return std::make_tuple(commandName,parameter);
}

void ChatMessageReceiver::IdentifyViewer(const QString &name)
{
	Viewer viewer(name);
	if (viewers.find(name) == viewers.end()) emit ArrivalConfirmed(name);
	viewers.emplace(name,viewer);
}

Command* ChatMessageReceiver::FindCommand(const QString &name)
{
	if (commands.find(name) != commands.end()) return &commands.at(name);
	if (commandAliases.find(name) != commandAliases.end()) return &commandAliases.at(name).get();
	return nullptr;
}

const QString ChatMessageReceiver::DownloadEmote(const Emote &emote)
{
	const QString emotePath=QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(QString("%1.png").arg(emote.id));
	if (!QFile(emotePath).exists())
	{
		QNetworkRequest request(QString(TWITCH_API_ENDPOINT_EMOTE_URL).arg(emote.id));
		QNetworkReply *downloadReply=downloadManager->get(request);
		connect(downloadReply,&QNetworkReply::finished,[this,downloadReply,id=emote.id,emotePath]() {
			if (downloadReply->error())
			{
				emit Alert(QString("Failed to download %1 emote: %2").arg(id,downloadReply->errorString()));
				return;
			}
			if (!QImage::fromData(downloadReply->readAll()).save(emotePath)) emit Alert(QString("Failed to save emote: %1").arg(emotePath));
			emit Refresh();
		});

	}
	return emotePath;
}

void ChatMessageReceiver::Fail(const QString &reason)
{
	emit Alert(reason);
	emit Failed();
}
