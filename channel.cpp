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
const char16_t *TWITCH_PING=u"PING :tmi.twitch.tv";
const char *TWITCH_PONG="PONG :tmi.twitch.tv\n";

const char *IRC_COMMAND_USER="NICK %1\n";
const char *IRC_COMMAND_PASSWORD="PASS oauth:%1\n";
const char *IRC_COMMAND_JOIN="JOIN #%1\n";
const char16_t *IRC_VALIDATION_AUTHENTICATION=u"You are in a maze of twisty passages, all alike.";
const char16_t *IRC_VALIDATION_JOIN=u"End of /NAMES list";

const char *SETTINGS_CATEGORY_CHANNEL="Channel";

enum class IRCCommand
{
	RPL_WELCOME=1,
	RPL_YOURHOST=2,
	RPL_CREATED=3,
	RPL_MYINFO=4,
	RPL_NAMREPLY=353,
	RPL_ENDOFNAMES=366,
	RPL_MOTDSTART=375,
	RPL_MOTD=372,
	RPL_ENDOFMOTD=376,
	CAP=1000,
	JOIN,
	PRIVMSG
};

const std::unordered_map<QString,IRCCommand> nonNumericIRCCommands={
	{"CAP",IRCCommand::CAP},
	{"JOIN",IRCCommand::JOIN},
	{"PRIVMSG",IRCCommand::PRIVMSG}
};

enum class CapabilitiesSubcommand
{
	ACK,
	NAK
};

const std::unordered_map<QString,CapabilitiesSubcommand> capabilitiesSubcommands={
	{"ACK",CapabilitiesSubcommand::ACK},
	{"NAK",CapabilitiesSubcommand::NAK}
};

Channel::Channel(Security &security,IRCSocket *socket,QObject *parent) : QObject(parent),
	security(security),
	settingChannel(SETTINGS_CATEGORY_CHANNEL,"Name",""),
	ircSocket(socket)
{
	if (!ircSocket) ircSocket=new IRCSocket(this);
	connect(ircSocket,&IRCSocket::connected,[this]() {
		emit Print("Connected!",OPERATION_CONNECTION);
		Authenticate();
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
	static QString cache;

	while (!ircSocket->atEnd())
	{
		std::optional<char> character='\0';
		while (character && *character != '\n')
		{
			character=ircSocket->Pop();
			if (character) cache.append(*character);
		}

		if (cache.isEmpty() || cache.back() != '\n') return;
		ParseMessage(cache);
		cache.clear();
	}
}

void Channel::ParseMessage(const QString& message)
{
	static const char* OPERATION_PARSE_MESSAGE="message parsing";

	QStringView window(message);
	std::optional<QStringView> prefix=StringView::Take(window,':');
	std::optional<QStringView> source=StringView::Take(window,' ');
	if (!source)
	{
		emit Print("Source is missing from message",OPERATION_PARSE_MESSAGE);
		return;
	}
	std::optional<QStringView> command=StringView::Take(window,' ');
	if (!command)
	{
		emit Print("Command is missing from message",OPERATION_PARSE_MESSAGE);
		return;
	}
	std::optional<QStringView> parameters;
	if (!window.isEmpty()) parameters=StringView::Take(window,':');
	std::optional<QStringView> finalParameter;
	if (!window.isEmpty()) finalParameter=StringView::Take(window,'\n');
	DispatchMessage(prefix ? prefix->toString() : QString(), source->toString(), command->toString(), parameters ? parameters->toString().split(' ') : QStringList(), finalParameter ? finalParameter->toString() : QString());
}

void Channel::DispatchMessage(QString prefix,QString source,QString command,QStringList parameters,QString finalParameter)
{
	const char *OPERATION_DISPATCH="dispatch message";

	bool numeric=false;
	int code=command.toInt(&numeric);
	if (!numeric) code=nonNumericIRCCommands.contains(command) ? static_cast<int>(nonNumericIRCCommands.at(command)) : -1;
	switch (code)
	{
	case static_cast<int>(IRCCommand::RPL_WELCOME):
		break;
	case static_cast<int>(IRCCommand::RPL_YOURHOST):
		break;
	case static_cast<int>(IRCCommand::RPL_CREATED):
		break;
	case static_cast<int>(IRCCommand::RPL_MYINFO):
		break;
	case static_cast<int>(IRCCommand::RPL_NAMREPLY):
		emit Print(QString("User list received:\n%1").arg(finalParameter.replace(' ','\n')));
		break;
	case static_cast<int>(IRCCommand::RPL_ENDOFNAMES):
		break;
	case static_cast<int>(IRCCommand::RPL_ENDOFMOTD):
		emit Connected();
		emit Print("Server accepted authentication; requesting capabilities...",OPERATION_DISPATCH);
		RequestCapabilities();
		break;
	case static_cast<int>(IRCCommand::CAP):
		ParseCapabilities(parameters,finalParameter);
		break;
	case static_cast<int>(IRCCommand::JOIN):
		emit Joined();
		break;
	case static_cast<int>(IRCCommand::PRIVMSG):
		emit Dispatch(prefix,source,parameters,finalParameter);
		break;
	default:
		emit Print("Unrecognized command",OPERATION_DISPATCH);
	}
}

void Channel::SendMessage(QString prefix,QString command,QStringList parameters,QString finalParameter)
{
	QString message=QString("%1:? %2").arg(prefix,command);
	if (!parameters.isEmpty()) message.append(QString(" %1").arg(parameters.join(' ')));
	if (!finalParameter.isEmpty()) message.append(QString(" :%1").arg(finalParameter));
	message.append("\r\n");
	ircSocket->write(StringConvert::ByteArray(message));
}

void Channel::ParseCapabilities(const QStringList &parameters,const QString &capabilities)
{
	// must contain at least client identifier name (or *) and subcommand
	if (parameters.size() < 2)
	{
		emit Print("Capabilities message is malformatted");
		return;
	}

	DispatchCapabilities(parameters.at(0),parameters.at(1),capabilities.split(' '));
}

void Channel::DispatchCapabilities(const QString &clientIdentifier,const QString &subCommand,const QStringList &capabilities)
{
	int code=capabilitiesSubcommands.contains(subCommand) ? static_cast<int>(capabilitiesSubcommands.at(subCommand)) : -1;
	switch (code)
	{
	case static_cast<int>(CapabilitiesSubcommand::ACK):
		RequestJoin();
		break;
	case static_cast<int>(CapabilitiesSubcommand::NAK):
		emit Print(QString("Capability was rejected by server: %1").arg(capabilities.join(' ')));
		break;
	default:
		emit Print("Unrecognized capabilities subcommand");
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

void Channel::Authenticate()
{
	if (!security.Administrator())
	{
		emit Print("Please set the Administrator under the Authorization section in your settings file",OPERATION_AUTHENTICATION);
		return;
	}

	emit Print(QString("Sending credentials: %1").arg(QString(IRC_COMMAND_USER).arg(static_cast<QString>(security.Administrator()))),OPERATION_AUTHENTICATION);
	SendMessage(QString(),"PASS",{QString("oauth:%1").arg(static_cast<QString>(security.OAuthToken()))},QString());
	SendMessage(QString(),"NICK",{security.Administrator()},QString());
}

void Channel::RequestCapabilities()
{
	SendMessage(QString(),"CAP",{"REQ"},"twitch.tv/tags :twitch.tv/commands"); // TODO: implement source
}

void Channel::RequestJoin()
{
	SendMessage(QString(),"JOIN",{QString("#%1").arg(settingChannel ? static_cast<QString>(settingChannel).toLower() : static_cast<QString>(security.Administrator()).toLower())},QString());
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

std::optional<char> IRCSocket::Pop()
{
	char character='\0';
	if (!getChar(&character))
	{
		emit Print("Failed to read data from socket");
		return std::nullopt;
	}
	return {character};
}
