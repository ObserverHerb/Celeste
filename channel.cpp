#include <QCoreApplication>
#include "channel.h"
#include "globals.h"

const char *OPERATION_CHANNEL="channel";
const char *OPERATION_CONNECTION="connection";
const char *OPERATION_AUTHENTICATION="authentication";
const char *OPERATION_SEND="sending data";
const char *OPERATION_RECEIVE="receiving data";
const char *OPERATION_CAPABILITIES="recognize capabilities";
const char *OPERATION_NOTICES="recognize notice";

const char *TWITCH_HOST="irc.chat.twitch.tv";
const unsigned int TWITCH_PORT=6667;

const char *IRC_COMMAND_USER="NICK";
const char *IRC_COMMAND_JOIN="JOIN";

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
	ERR_UNKNOWNCOMMAND=421,
	CAP=1000,
	JOIN,
	PART,
	CLEARMSG,
	CLEARCHAT,
	PRIVMSG,
	NOTICE,
	USERNOTICE,
	PING
};

const std::unordered_map<QString,IRCCommand> nonNumericIRCCommands={
	{"CAP",IRCCommand::CAP},
	{IRC_COMMAND_JOIN,IRCCommand::JOIN},
	{"PART",IRCCommand::PART},
	{"CLEARMSG",IRCCommand::CLEARMSG},
	{"CLEARCHAT",IRCCommand::CLEARCHAT},
	{"PRIVMSG",IRCCommand::PRIVMSG},
	{"NOTICE",IRCCommand::NOTICE},
	{"USERNOTICE",IRCCommand::USERNOTICE},
	{"PING",IRCCommand::PING}
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

enum class Notice
{
	MALFORMATTED_AUTH,
	DENIED,
};

const std::unordered_map<QString,Notice> notices={
	{"Login authentication failed",Notice::DENIED},
	{"Improperly formatted auth",Notice::MALFORMATTED_AUTH}
};

Channel::Channel(Security &security,IRCSocket *socket,QObject *parent) : QObject(parent),
	security(security),
	settings{
		.name{SETTINGS_CATEGORY_CHANNEL,"Name",security.Administrator().Value()},
		.protect{SETTINGS_CATEGORY_CHANNEL,"Protect",false}
	},
	ircSocket(socket)
{
	if (!ircSocket) ircSocket=new IRCSocket(this);
	connect(ircSocket,&IRCSocket::connected,this,[this]() {
		emit Print("Connected!",OPERATION_CONNECTION);
		Authenticate();
	});
	connect(ircSocket,&IRCSocket::disconnected,this,[this]() {
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
	static QByteArray cache;

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

void Channel::ParseMessage(const QString message)
{
	static const char* OPERATION_PARSE_MESSAGE="message parsing";
	emit Print(message,OPERATION_PARSE_MESSAGE);
	QStringView window(message);

	// grab prefix if one exists
	std::optional<QStringView> prefix=StringView::Take(window,'@',' ');
	std::optional<QStringView> source=StringView::Take(window,':',' ');
	if (!source) emit Print("Source is missing from message",OPERATION_PARSE_MESSAGE); // make a note, but per the spec, source is optional
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
	DispatchMessage(prefix ? prefix->toString() : QString(), source ? source->toString() : QString(), command->toString(), parameters ? parameters->toString().split(' ',Qt::SkipEmptyParts) : QStringList(), finalParameter ? finalParameter->toString() : QString());
}

void Channel::DispatchMessage(QString prefix,QString source,QString command,QStringList parameters,QString finalParameter)
{
	static const char *OPERATION_DISPATCH="dispatch message";

	bool numeric=false;
	int code=command.toInt(&numeric);
	if (!numeric)
	{
		auto nonNumericIRCCommand=nonNumericIRCCommands.find(command);
		if (nonNumericIRCCommand != nonNumericIRCCommands.end())
			code=static_cast<int>(nonNumericIRCCommand->second);
		else
			code=-1;
	}
	switch (code) // I'd rather static_cast the code, but if Twitch sends a command I haven't implemented, code will be outside the enum's range
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
	{
		emit Print(QString("User list received:\n%1").arg(finalParameter.replace(' ','\n')));
		const QStringList rows=finalParameter.split(' ');
		for (const QString &row : rows)
		{
			const QStringList users=row.split('\n'); // Twitch doesn't follow the spec here and returns mutliple names deliminted by \n between each space
			for (const QString &user : users) emit Joined(user);
		}
		break;
	}
	case static_cast<int>(IRCCommand::RPL_ENDOFNAMES):
		break;
	case static_cast<int>(IRCCommand::RPL_ENDOFMOTD):
		emit Connected();
		emit Print("Server accepted authentication; requesting capabilities...",OPERATION_DISPATCH);
		RequestCapabilities();
		break;
	case static_cast<int>(IRCCommand::ERR_UNKNOWNCOMMAND):
		emit Print("Server didn't recognize command",OPERATION_DISPATCH);
		break;
	case static_cast<int>(IRCCommand::CAP):
		ParseCapabilities(parameters,finalParameter);
		break;
	case static_cast<int>(IRCCommand::JOIN):
		DispatchJoin(source);
		break;
	case static_cast<int>(IRCCommand::PART):
		DispatchPart(source);
		break;
	case static_cast<int>(IRCCommand::CLEARMSG):
		emit Deleted(prefix);
		break;
	case static_cast<int>(IRCCommand::CLEARCHAT):
		emit Deleted(prefix);
		break;
	case static_cast<int>(IRCCommand::PRIVMSG):
		emit Dispatch(prefix,source,parameters,finalParameter);
		break;
	case static_cast<int>(IRCCommand::NOTICE):
		ParseNotice(finalParameter);
		break;
	case static_cast<int>(IRCCommand::USERNOTICE):
		ParseUserNotice(prefix,finalParameter);
		break;
	case static_cast<int>(IRCCommand::PING):
		emit Ping(finalParameter);
		break;
	default:
		emit Print(QString("Unrecognized command '%1' received from server").arg(command),OPERATION_DISPATCH);
	}
}

void Channel::SendMessage(QString prefix,QString command,QStringList parameters,QString finalParameter)
{
	// "Clients MUST NOT include a source when sending a message." (https://modern.ircdocs.horse/#client-messages)
	QString message=QString("%1 %2").arg(prefix,command);
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
		emit Print("Capabilities message is malformatted",OPERATION_CAPABILITIES);
		return;
	}

	DispatchCapabilities(parameters.at(1),capabilities.split(' ')); // parameters.at(0) is client identifier, which I don't need right now
}

void Channel::DispatchCapabilities(const QString &subCommand,const QStringList &capabilities)
{
	int code=-1;
	if (auto capabilitiesSubcommand=capabilitiesSubcommands.find(subCommand); capabilitiesSubcommand != capabilitiesSubcommands.end()) code=static_cast<int>(capabilitiesSubcommand->second);
	switch (code)
	{
	case static_cast<int>(CapabilitiesSubcommand::ACK):
		RequestJoin();
		break;
	case static_cast<int>(CapabilitiesSubcommand::NAK):
		emit Print(QString("Capability was rejected by server: %1").arg(capabilities.join(' ')),OPERATION_CAPABILITIES);
		break;
	default:
		emit Print("Unrecognized capabilities subcommand",OPERATION_CAPABILITIES);
		break;
	}
}

void Channel::ParseNotice(const QString &message)
{
	auto notice=notices.find(message);
	if (notice == notices.end())
	{
		emit Print("Unrecognized notice received",OPERATION_NOTICES);
		return;
	}

	switch (notice->second)
	{
	case Notice::DENIED:
		emit Print("Server denied login",OPERATION_NOTICES);
		emit Denied();
		break;
	case Notice::MALFORMATTED_AUTH:
		emit Print("Authentication information was sent incorrectly",OPERATION_NOTICES);
		break;
	}
}

void Channel::ParseUserNotice(const QString &prefix,const QString &message)
{
	emit Print(QString("%1 - %2").arg(prefix,message),QStringLiteral("USERNOTICE"));
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

	emit Print(QString("Sending credentials: %1").arg(QString("%1 %2\n").arg(IRC_COMMAND_USER,static_cast<QString>(security.Administrator()))),OPERATION_AUTHENTICATION);
	SendMessage(QString(),"PASS",{QString("oauth:%1").arg(static_cast<QString>(security.OAuthToken()))},QString());
	SendMessage(QString(),"NICK",{security.Administrator()},QString());
}

void Channel::RequestCapabilities()
{
	SendMessage(QString(),"CAP",{"REQ"},"twitch.tv/membership twitch.tv/tags twitch.tv/commands");
}

void Channel::RequestJoin()
{
	SendMessage(QString(),IRC_COMMAND_JOIN,{QString("#%1").arg(settings.name ? static_cast<QString>(settings.name).toLower() : static_cast<QString>(security.Administrator()).toLower())},QString());
}

void Channel::DispatchJoin(const QString &source)
{
	std::optional<Hostmask> hostmask=ParseSource(source);
	if (hostmask)
	{
		if (hostmask->nick == static_cast<QString>(security.Administrator()))
			emit Joined();
		else
			emit Joined(hostmask->nick);
	}
}

void Channel::DispatchPart(const QString &source)
{
	std::optional<Hostmask> hostmask=ParseSource(source);
	if (hostmask) emit Parted(hostmask->nick);
}

std::optional<Hostmask> Channel::ParseSource(const QString &source)
{
	QStringView window(source);
	std::optional<QStringView> nick=StringView::Take(window,'!');
	if (!nick) return std::nullopt;
	std::optional<QStringView> user=StringView::Take(window,'@');
	if (!user) return std::nullopt;
	if (window.isEmpty()) return std::nullopt;
	return Hostmask{
		.nick=nick->toString(),
		.user=user->toString(),
		.host=window.toString()
	};
}

void Channel::SocketError(QAbstractSocket::SocketError error)
{
	Q_UNUSED(error)
	emit Print(QString("Failed to connect to server (%1)").arg(ircSocket->errorString()),OPERATION_CONNECTION);
}

void Channel::Pong(const QString &token)
{
	SendMessage(QString(),"PONG",{},token);
}

Settings::Channel& Channel::Settings()
{
	return settings;
}

bool Channel::Protection()
{
	return settings.protect;
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
		emit Print("Failed to read data from socket",OPERATION_RECEIVE);
		return std::nullopt;
	}
	return {character};
}
