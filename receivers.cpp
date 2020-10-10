#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>
#include <stdexcept>
#include "globals.h"
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

ChatMessageReceiver::ChatMessageReceiver(QObject *parent) : MessageReceiver(parent)
{
	QDir commandListPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
	if (!commandListPath.mkpath(commandListPath.absolutePath())) throw std::runtime_error(QString("Failed to create command list path: %1").arg(commandListPath.absolutePath()).toStdString());
	QFile commandListFile(commandListPath.filePath(COMMANDS_LIST_FILENAME));
	if (!commandListFile.open(QIODevice::ReadWrite)) throw std::runtime_error(QString("Failed to open command list file: %1").arg(commandListFile.fileName()).toStdString());

	QJsonParseError jsonError;
	QByteArray data=commandListFile.readAll();
	if (data.isEmpty()) data="[]";
	QJsonDocument json=QJsonDocument::fromJson(data,&jsonError);
	if (json.isNull()) throw std::runtime_error(jsonError.errorString().toStdString());
	for (const QJsonValue jsonValue : json.array())
	{
		QJsonObject jsonObject=jsonValue.toObject();
		QString name=jsonObject.value(JSON_KEY_COMMAND_NAME).toString();
		CommandType type=COMMAND_TYPES.at(jsonObject.value(JSON_KEY_COMMAND_TYPE).toString());
		bool randomPath=jsonObject.contains(JSON_KEY_COMMAND_RANDOM_PATH) ? jsonObject.value(JSON_KEY_COMMAND_RANDOM_PATH).toBool() : false;
		QString path=jsonObject.value(JSON_KEY_COMMAND_PATH).toString();
		commands[name]={type,randomPath,path};
	}
}

void ChatMessageReceiver::Process(const QString data)
{
	QStringList messageSegments=data.split(":",Qt::SkipEmptyParts);
	if (messageSegments.size() < 2)
	{
		emit Failed();
		return;
	}
	QString hostmask=QString(messageSegments.front());
	messageSegments.pop_front();
	QStringList hostmaskSegments=hostmask.split("!",Qt::SkipEmptyParts);
	if (hostmaskSegments.size() < 1)
	{
		emit Failed();
		return;
	}
	QString user=hostmaskSegments.front();
	if (messageSegments.at(0).at(0) == "!")
	{
		QStringList commandSegments=messageSegments.at(0).trimmed().split(" ");
		QString commandName=commandSegments.at(0).mid(1);
		commandSegments.pop_front();
		QString parameter=commandSegments.join(" ");
		if (commands.find(commandName) != commands.end())
		{
			Command command=commands.at(commandName);
			switch (command.type)
			{
			case CommandType::VIDEO:
				if (command.random)
				{
					QDir directory(command.path);
					QStringList videos=directory.entryList(QStringList() << "*.mp4",QDir::Files);
					emit PlayVideo(directory.absoluteFilePath(videos[QRandomGenerator::global()->bounded(0,videos.size())]));
				}
				else
				{
					emit PlayVideo(command.path);
				}
				break;
			};
		}
		/*if (commands.front() == "!lurk")
		{
			emit PlayVideo("short.mp4");
		}
		if (commands.front() == "!gandalf")
		{
			emit PlayVideo("gandalf.mp4");
		}
		if (commands.front() == "!progress")
		{
			emit PlayVideo("progress.mp4");
		}
		if (commands.front() == "!tts")
		{
			commands.pop_front();
			emit Speak(commands.join(" "));
		}
		if (commands.front() == "!voices")
		{
			emit ShowVoices();
		}*/
	}
	emit Print(QString("<div class='user'>%1</div><div class='message'>%2</div><br>").arg(user,messageSegments.join(":")));
}
