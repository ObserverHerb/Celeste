#include <QCoreApplication>
#include "channel.h"
#include "globals.h"

const char *Channel::OPERATION_CHANNEL="channel";
const char *Channel::OPERATION_CONNECTION="connection";
const char *Channel::OPERATION_AUTHORIZATION="authentication";
const char *Channel::OPERATION_SEND="sending data";
const char *Channel::OPERATION_RECEIVE="receiving data";

const char *TWITCH_HOST="irc.chat.twitch.tv";
const unsigned int TWITCH_PORT=6667;
const char *TWITCH_PING="PING :tmi.twitch.tv\n";
const char *TWITCH_PONG="PONG :tmi.twitch.tv\n";

const char *IRC_COMMAND_USER="NICK %1\n";
const char *IRC_COMMAND_PASSWORD="PASS oauth:%1\n";
const char *IRC_COMMAND_JOIN="JOIN #%1\n";
const char *IRC_VALIDATION_AUTHENTICATION="You are in a maze of twisty passages, all alike.";
const char *IRC_VALIDATION_JOIN="End of /NAMES list";

const char *SETTINGS_CATEGORY_CHANNEL="Channel";

Channel::Channel(PrivateSetting &settingAdministrator,PrivateSetting &settingOAuthToken,IRCSocket *socket,QObject *parent) : QObject(parent),
	phase(Phase::CONNECT),
	settingAdministrator(settingAdministrator),
	settingOAuthToken(settingOAuthToken),
	settingChannel(SETTINGS_CATEGORY_CHANNEL,"Name",""),
	settingJoinDelay(SETTINGS_CATEGORY_CHANNEL,"JoinDelay",5),
	ircSocket(socket)
{
	if (!ircSocket) ircSocket=new IRCSocket(this);
	connect(ircSocket,&IRCSocket::connected,this,&Channel::Connected);
	connect(ircSocket,&IRCSocket::disconnected,this,&Channel::Disconnected);
	connect(ircSocket,&IRCSocket::readyRead,this,&Channel::DataAvailable);
	connect(ircSocket,&IRCSocket::errorOccurred,this,&Channel::SocketError);
	connect(this,&Channel::Ping,this,&Channel::Pong);
}

Channel::~Channel()
{
	ircSocket->disconnectFromHost();
}


void Channel::Dump(const QString &data)
{
	QStringList lines=data.split("\n",StringConvert::Split::Behavior(StringConvert::Split::Behaviors::SKIP_EMPTY_PARTS));
	for (QString &line : lines) line.prepend("> ");
	emit Print(lines.join("\n"),OPERATION_RECEIVE);
}

void Channel::DataAvailable()
{
	const QString data=ircSocket->Read();
	emit Dump(data);

	switch (phase)
	{
	case Phase::AUTHENTICATE:
		AuthenticationResponse(data);
		break;
	case Phase::JOIN_CHANNEL:
		JoinResponse(data);
		break;
	case Phase::DISPATCH:
		if (data.size() > 4 && data.left(4) == "PING")
		{
			emit Ping();
			return;
		}
		emit Dispatch(data);
		break;
	}
}

void Channel::Connect()
{
	QMetaObject::invokeMethod(ircSocket,[this]() {
		// trigger using event loop so the socket operation doesn't freeze the UI
		ircSocket->connectToHost(TWITCH_HOST,TWITCH_PORT);
	},Qt::QueuedConnection);
	emit Print("Connecting to IRC...",OPERATION_CONNECTION);
}

void Channel::Connected()
{
	emit Print("Connected!",OPERATION_CONNECTION);
	RequestAuthentication();
}

void Channel::Disconnected()
{
	emit Print("Disconnected",OPERATION_CONNECTION);
}

void Channel::RequestAuthentication()
{
	if (!settingAdministrator)
	{
		emit Print("Please set the Administrator under the Authorization section in your settings file",OPERATION_AUTHORIZATION);
		return;
	}
	phase=Phase::AUTHENTICATE;
	emit Print(QString("Sending credentials: %1").arg(QString(IRC_COMMAND_USER).arg(static_cast<QString>(settingAdministrator))),OPERATION_AUTHORIZATION);
	ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_PASSWORD).arg(static_cast<QString>(settingOAuthToken))));
	ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_USER).arg(static_cast<QString>(settingAdministrator))));
}

void Channel::AuthenticationResponse(const QString &data)
{
	if (data.contains(IRC_VALIDATION_AUTHENTICATION))
	{
		emit Print("Server accepted authentication");
		RequestJoin();
	}
	else
	{
		emit Print("Server did not accept authentication");
	}
}

void Channel::RequestJoin()
{
	phase=Phase::JOIN_CHANNEL;
	emit Print(QString("Joining stream in %1 seconds...").arg(TimeConvert::Interval(TimeConvert::Seconds(settingJoinDelay))),OPERATION_CHANNEL);
	QTimer::singleShot(TimeConvert::Interval(JoinDelay()),this,[this]() {
		ircSocket->write("CAP REQ :twitch.tv/tags :twitch.tv/commands\n");
		ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_JOIN).arg(settingChannel ? static_cast<QString>(settingChannel) : static_cast<QString>(settingAdministrator))));
	});
}

void Channel::JoinResponse(const QString &data)
{
	if (!data.contains(IRC_VALIDATION_JOIN))
	{
		emit Print("Failed to join stream");
		return;
	}
	emit Print("Stream joined");
	emit Joined();
	phase=Phase::DISPATCH;
}

void Channel::SocketError(QAbstractSocket::SocketError error)
{
	emit Print(Console::GenerateMessage(QCoreApplication::applicationName(),OPERATION_CONNECTION,QString("Failed to connect to server (%1)").arg(ircSocket->errorString())));
}

void Channel::Pong()
{
	ircSocket->write(StringConvert::ByteArray(TWITCH_PONG));
}

QByteArray IRCSocket::Read()
{
	return readAll();
}
