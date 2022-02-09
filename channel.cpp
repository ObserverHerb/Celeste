#include <QCoreApplication>
#include "channel.h"
#include "globals.h"

const char *OPERATION_CHANNEL="channel";
const char *OPERATION_CONNECTION="connection";
const char *OPERATION_AUTHENTICATION="authentication";
const char *OPERATION_SEND="sending data";
const char *OPERATION_RECEIVE="receiving data";

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

Channel::Channel(Security &security,IRCSocket *socket,QObject *parent) : QObject(parent),
	phase(Phase::CONNECT),
	security(security),
	settingChannel(SETTINGS_CATEGORY_CHANNEL,"Name",""),
	settingJoinDelay(SETTINGS_CATEGORY_CHANNEL,"JoinDelay",5),
	ircSocket(socket)
{
	if (!ircSocket) ircSocket=new IRCSocket(this);
	connect(ircSocket,&IRCSocket::connected,[this]() {
		emit Print("Connected!",OPERATION_CONNECTION);
		RequestAuthentication();
	});
	connect(ircSocket,&IRCSocket::disconnected,[this]() {
		emit Disconnected();
		emit Print("Disconnected",OPERATION_CONNECTION);
	});
	connect(ircSocket,&IRCSocket::readyRead,this,&Channel::DataAvailable);
	connect(ircSocket,&IRCSocket::errorOccurred,this,&Channel::SocketError);
	connect(this,&Channel::Ping,this,&Channel::Pong);
}

Channel::~Channel()
{
	Disconnect();
}

void Channel::DataAvailable()
{
	const QString data=ircSocket->Read();
	emit Print(StringConvert::Dump(data));

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

void Channel::Disconnect()
{
	ircSocket->disconnectFromHost();
}

void Channel::RequestAuthentication()
{
	if (!security.Administrator())
	{
		emit Print("Please set the Administrator under the Authorization section in your settings file",OPERATION_AUTHENTICATION);
		return;
	}
	phase=Phase::AUTHENTICATE;
	emit Print(QString("Sending credentials: %1").arg(QString(IRC_COMMAND_USER).arg(static_cast<QString>(security.Administrator()))),OPERATION_AUTHENTICATION);
	ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_PASSWORD).arg(static_cast<QString>(security.OAuthToken()))));
	ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_USER).arg(static_cast<QString>(security.Administrator()))));
}

void Channel::AuthenticationResponse(const QString &data)
{
	if (data.contains(IRC_VALIDATION_AUTHENTICATION))
	{
		emit Connected(); // socket must be connected and authentication must succeed before considering bot connected to the channel
		emit Print("Server accepted authentication",OPERATION_AUTHENTICATION);
		RequestJoin();
	}
	else
	{
		emit Print("Server did not accept authentication",OPERATION_AUTHENTICATION);
		emit Denied();
	}
}

void Channel::RequestJoin()
{
	phase=Phase::JOIN_CHANNEL;
	emit Print(QString("Joining stream in %1 seconds...").arg(TimeConvert::Interval(TimeConvert::Seconds(settingJoinDelay))),OPERATION_CHANNEL);
	QTimer::singleShot(TimeConvert::Interval(JoinDelay()),this,[this]() {
		ircSocket->write("CAP REQ :twitch.tv/tags :twitch.tv/commands\n");
		ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_JOIN).arg(settingChannel ? static_cast<QString>(settingChannel).toLower() : static_cast<QString>(security.Administrator()).toLower())));
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
	emit Print(QString("Failed to connect to server (%1)").arg(ircSocket->errorString()),OPERATION_CONNECTION);
}

void Channel::Pong()
{
	ircSocket->write(StringConvert::ByteArray(TWITCH_PONG));
}

QByteArray IRCSocket::Read()
{
	return readAll();
}
