#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringBuilder>
#include <QUuid>
#include "globals.h"
#include "subscribers.h"

const char *HEADER_DELIMITER=":";

const char *HEADER_SUBSCRIPTION_TYPE="Twitch-Eventsub-Subscription-Type";

const char *JSON_KEY_CHALLENGE="challenge";
const char *JSON_KEY_EVENT="event";
const char *JSON_KEY_EVENT_REWARD="reward";
const char *JSON_KEY_EVENT_REWARD_TITLE="title";
const char *JSON_KEY_EVENT_FOLLOW="followed at";
const char *JSON_KEY_EVENT_USER_NAME="user_name";
const char *JSON_KEY_EVENT_USER_INPUT="user_input";
const char *JSON_KEY_SUBSCRIPTION="subscription";
const char *JSON_KEY_SUBSCRIPTION_TYPE="type";

const char *SUBSCRIPTION_TYPE_FOLLOW="channel.follow";
const char *SUBSCRIPTION_TYPE_RAID="channel.raid";

const QString EventSubscriber::SUBSYSTEM_NAME="Event Subscriber";
const QString EventSubscriber::LINE_BREAK="\r\n";

EventSubscriber::EventSubscriber(QObject *parent) : QTcpServer(parent),
	settingClientID(SETTINGS_CATEGORY_AUTHORIZATION,"ClientID"),
	settingOAuthToken(SETTINGS_CATEGORY_AUTHORIZATION,"ServerToken"),
	secret(QUuid::createUuid().toString())
{
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_FOLLOW,SubscriptionType::CHANNEL_FOLLOW});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_RAID,SubscriptionType::CHANNEL_RAID});

	//connect(this,&EventSubscriber::acceptError,this,QOverload<QAbstractSocket::SocketError>::of(&EventSubscriber::)) // FIXME: handle error here?
	connect(this,&EventSubscriber::newConnection,this,&EventSubscriber::ConnectionAvailable);
}

EventSubscriber::~EventSubscriber()
{
	close();
}

void EventSubscriber::Listen()
{
	// FIXME: this is going to be a problemon machines with mutiple NICs
	QList<QHostAddress> interfaceAddresses=QNetworkInterface::allAddresses();
	if (interfaceAddresses.size() < 1) throw std::runtime_error("No network interfaces available"); // TODO: unit test this
	int port=4443;
	Print(Console::GenerateMessage(SUBSYSTEM_NAME,QString("Using network address %1 and port %2").arg("any",port)));
	if (!listen(QHostAddress::Any,port))
	{
		Print(Console::GenerateMessage(SUBSYSTEM_NAME,QString("Error while subscribing: %1").arg(errorString())));
		throw std::runtime_error("Failed to listen for incoming connections");
	}
}

void EventSubscriber::Subscribe(const QString &type)
{
	emit Print(Console::GenerateMessage(SUBSYSTEM_NAME,QString("Requesting subscription to %1").arg(type)));
	QNetworkAccessManager *manager=new QNetworkAccessManager(this);
	connect(manager,&QNetworkAccessManager::finished,[this,manager](QNetworkReply *reply) {
		// FIXME: check the validity of this reply!
		emit Print(Console::GenerateMessage(SUBSYSTEM_NAME,reply->readAll()));
		manager->disconnect();
	});
	QUrl query(TWITCH_API_ENDPOINT_EVENTSUB);
	QNetworkRequest request(query);
	request.setRawHeader("Authorization",StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(settingOAuthToken))));
	request.setRawHeader("Client-ID",settingClientID);
	request.setRawHeader("Content-Type","application/json");
	QJsonDocument payload(QJsonObject({
		{"type",type},
		{"version","1"},
		{
			"condition",
			QJsonObject({{"broadcaster_user_id",SUBSCRIPTION_TYPE_FOLLOW}})
		},
		{
			"transport",
			QJsonObject({
				{"method","webhook"},
				{"callback",QString("https://%1").arg("www.sky-meyg.com/eventsub")},
				{"secret",secret}
			})
		}
	}));
	emit Print(Console::GenerateMessage(SUBSYSTEM_NAME,QString("Sending subscription request: %1").arg(QString(payload.toJson(QJsonDocument::Indented)))));
	//manager->post(request,payload.toJson(QJsonDocument::Compact));
}

void EventSubscriber::DataAvailable()
{
	QTcpSocket *socket=SocketFromSignalSender();
	buffer.append(ReadFromSocket(socket));
	QStringList data=buffer.split(LINE_BREAK % LINE_BREAK,StringConvert::Split::Behavior(StringConvert::Split::Behaviors::SKIP_EMPTY_PARTS)); // TODO: can I do a splitref here?
	buffer.clear(); // FIXME: there's a dsync potential here where buffer always has the end of the previous message and the beginning of the next one
	if (data.size() < 2)
	{
		emit Print(Console::GenerateMessage(SUBSYSTEM_NAME,"Incomplete notification received"));
		emit Print(Console::GenerateMessage(SUBSYSTEM_NAME,data.join("\n")));
		return;
	}

	Headers headers=ParseHeaders(data.at(0));
	if (headers.find(HEADER_SUBSCRIPTION_TYPE) != headers.end() && subscriptionTypes.find(headers.at(HEADER_SUBSCRIPTION_TYPE)) != subscriptionTypes.end())
	{
		const QString &request=data.at(1);
		emit Print(Console::GenerateMessage(SUBSYSTEM_NAME,"read",request));
		QByteArray response=ProcessRequest(subscriptionTypes.at(headers.at(HEADER_SUBSCRIPTION_TYPE)),request);
		if (response.size() > 0) WriteToSocket(response,socket); // TODO: std::optional
	}
	else
	{
		emit Print(Console::GenerateMessage(SUBSYSTEM_NAME,"parse","No subscription type in headers"));
	}
}

const EventSubscriber::Headers EventSubscriber::ParseHeaders(const QString &data)
{
	Headers headers;
	for (const QString &line : data.split(LINE_BREAK))
	{
		QStringList pair=line.split(HEADER_DELIMITER,StringConvert::Split::Behavior(StringConvert::Split::Behaviors::SKIP_EMPTY_PARTS));
		if (pair.size() > 1)
		{
			const QString key=pair.takeFirst();
			headers.insert({key,pair.join(HEADER_DELIMITER).trimmed()});
		}
	}
	return headers;
}

const QByteArray EventSubscriber::ProcessRequest(const SubscriptionType type,const QString &request) // TODO: should this return std::optional?
{
	QJsonParseError jsonError;
	QJsonDocument json=QJsonDocument::fromJson(StringConvert::ByteArray(request.trimmed()),&jsonError);
	if (json.isNull())
	{
		Print(Console::GenerateMessage(SUBSYSTEM_NAME,jsonError.errorString()));
		return QByteArray();  // TODO: should this return std::optional?
	}
	else
	{
		Print(Console::GenerateMessage(SUBSYSTEM_NAME,"parser","JSON parsed successfully!"));
	}

	if (json.object().contains(JSON_KEY_CHALLENGE)) return StringConvert::ByteArray(BuildResponse(json.object().value(JSON_KEY_CHALLENGE).toString())); // TODO: different function, this isn't a notification

	switch (type)
	{
	case SubscriptionType::CHANNEL_FOLLOW:
		Print(Console::GenerateMessage(SUBSYSTEM_NAME,"Channel Follow","Follow received"));
		return StringConvert::ByteArray(BuildResponse());
	case SubscriptionType::CHANNEL_RAID:
		Print(Console::GenerateMessage(SUBSYSTEM_NAME,"Channel Raid","You have been raided!"));
		emit Raid();
		return StringConvert::ByteArray(BuildResponse());
	}

	return QByteArray(); // TODO: should this throw some kind of error? Unknown request?
}

const QString EventSubscriber::BuildResponse(const QString &data) const
{
	return QString("HTTP/1.1 200 OK\nAccept Ranges: bytes\nContent-Length: %1\nContent-Type: text/plain");
}

void EventSubscriber::ConnectionAvailable()
{
	QTcpSocket *socket=nextPendingConnection();
	connect(socket,&QTcpSocket::readyRead,this,&EventSubscriber::DataAvailable);
	connect(socket,&QTcpSocket::disconnected,socket,&QTcpSocket::deleteLater);
}

QTcpSocket* EventSubscriber::SocketFromSignalSender() const
{
	return qobject_cast<QTcpSocket*>(sender()); // TODO: capture failures here
}

QByteArray EventSubscriber::ReadFromSocket(QTcpSocket *socket) const
{
	if (!socket) throw std::runtime_error("Tried to read from non-existent socket");
	return socket->readAll();
}

bool EventSubscriber::WriteToSocket(const QByteArray &data,QTcpSocket *socket) const
{
	if (!socket) throw std::runtime_error("Tried to write to non-existent socket");
	return (socket->write(data) > 0);
}
