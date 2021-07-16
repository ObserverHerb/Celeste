#include "channel.h"
#include "globals.h"

const char *Channel::OPERATION_CHANNEL="channel";
const char *Channel::OPERATION_CONNECTION="connection";
const char *Channel::OPERATION_AUTHORIZATION="authorization";
const char *Channel::OPERATION_SEND="sending data";
const char *Channel::OPERATION_RECEIVE="receiving data";

Channel::Channel(QObject *parent) : QObject(parent),
	settingChannel(SETTINGS_CATEGORY_AUTHORIZATION,"Channel",""),
	settingJoinDelay(SETTINGS_CATEGORY_AUTHORIZATION,"JoinDelay",5),
	ircSocket(nullptr),
	authenticationReceiver(new AuthenticationReceiver(this)),
	channelJoinReceiver(new ChannelJoinReceiver(this)),
	chatMessageReceiver(new ChatMessageReceiver(this))
{
	ircSocket=new IRCSocket(this);
	connect(ircSocket,&IRCSocket::connected,this,&Channel::Connected);
	connect(ircSocket,&IRCSocket::disconnected,this,&Channel::Disconnected);
	connect(ircSocket,&IRCSocket::readyRead,this,&Channel::DataAvailable);
	connect(ircSocket,&IRCSocket::errorOccurred,this,&Channel::SocketError);

	connect(authenticationReceiver,&AuthenticationReceiver::Print,this,&Channel::Print);
	connect(authenticationReceiver,&AuthenticationReceiver::Succeeded,this,&Channel::Authorized);

	connect(channelJoinReceiver,&ChannelJoinReceiver::Print,this,&Channel::Print);
	connect(channelJoinReceiver,&ChannelJoinReceiver::Succeeded,this,QOverload<>::of(&Channel::Joined));

	connect(this,&Channel::Ping,this,&Channel::Pong);
}

Channel::~Channel()
{
	ircSocket->disconnectFromHost();
}

void Channel::DataAvailable()
{
	const QString data=ircSocket->Read();
	Log(Console::GenerateMessage(QCoreApplication::applicationName(),OPERATION_RECEIVE,StringConvert::ByteArray(data)));
	if (data.size() > 4 && data.left(4) == "PING")
	{
		emit Ping();
		return;
	}
	emit Dispatch(data);
}

void Channel::Connect()
{
	QTimer::singleShot(0,[this]() { ircSocket->connectToHost(TWITCH_HOST,TWITCH_PORT); }); // trigger using event loop so the socket operation doesn't freeze the UI
	emit Print(Console::GenerateMessage(QCoreApplication::applicationName(),"connect","Connecting to IRC..."));
}

void Channel::Connected()
{
	emit Print(Console::GenerateMessage(QCoreApplication::applicationName(),OPERATION_CONNECTION,"Connected!"));
	Authorize();
}

void Channel::Disconnected()
{
	if (dispatch) disconnect(dispatch);
	emit Print(Console::GenerateMessage(QCoreApplication::applicationName(),OPERATION_CONNECTION,"Disconnected"));
}

void Channel::Authorize()
{
	if (dispatch) disconnect(dispatch);
	dispatch=connect(this,&Channel::Dispatch,authenticationReceiver,&AuthenticationReceiver::Process);
	if (!settingAdministrator)
	{
		emit Print(Console::GenerateMessage(QCoreApplication::applicationName(),OPERATION_AUTHORIZATION,"Please set the Administrator under the Authorization section in your settings file"));
		return;
	}
	emit Print(Console::GenerateMessage(QCoreApplication::applicationName(),OPERATION_SEND,QString(IRC_COMMAND_USER).arg(static_cast<QString>(settingAdministrator))));
	emit Print(Console::GenerateMessage(QCoreApplication::applicationName(),OPERATION_AUTHORIZATION,"Sending credentials..."));
	ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_PASSWORD).arg(static_cast<QString>(settingOAuthToken))));
	ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_USER).arg(static_cast<QString>(settingAdministrator))));
}

void Channel::Authorized()
{
	Join();
}

void Channel::Join()
{
	if (dispatch) disconnect(dispatch);
	dispatch=connect(this,&Channel::Dispatch,channelJoinReceiver,&ChannelJoinReceiver::Process);
	emit Print(Console::GenerateMessage(QCoreApplication::applicationName(),OPERATION_CHANNEL,QString("Joining stream in %1 seconds...").arg(TimeConvert::Interval(TimeConvert::Seconds(settingJoinDelay)))));
	QTimer::singleShot(TimeConvert::Interval(static_cast<std::chrono::milliseconds>(settingJoinDelay)),this,[this]() {
		ircSocket->write("CAP REQ :twitch.tv/tags :twitch.tv/commands\n");
		ircSocket->write(StringConvert::ByteArray(QString(IRC_COMMAND_JOIN).arg(settingChannel ? static_cast<QString>(settingChannel) : static_cast<QString>(settingAdministrator))));
	});
}

void Channel::Joined()
{
	emit Joined(*chatMessageReceiver);
	Follow();
}

void Channel::Follow()
{
	if (dispatch) disconnect(dispatch);
	dispatch=connect(this,&Channel::Dispatch,chatMessageReceiver,&ChatMessageReceiver::Process);
}

void Channel::SocketError(QAbstractSocket::SocketError error)
{
	emit Print(Console::GenerateMessage(QCoreApplication::applicationName(),OPERATION_CONNECTION,QString("Failed to connect to server (%1)").arg(ircSocket->errorString())));
	Connect();
}

void Channel::Pong()
{
	ircSocket->write(StringConvert::ByteArray(TWITCH_PONG));
}

QByteArray IRCSocket::Read()
{
	return readAll();
}
