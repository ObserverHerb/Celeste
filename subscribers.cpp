#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include "globals.h"
#include "subscribers.h"

const char *JSON_KEY_CHALLENGE="challenge";
const char *JSON_KEY_EVENT="event";
const char *JSON_KEY_EVENT_REWARD="reward";
const char *JSON_KEY_EVENT_REWARD_TITLE="title";
const char *JSON_KEY_EVENT_FOLLOW="followed at";
const char *JSON_KEY_EVENT_USER_NAME="user_name";
const char *JSON_KEY_EVENT_USER_INPUT="user_input";
const char *JSON_KEY_SUBSCRIPTION="subscription";
const char *JSON_KEY_SUBSCRIPTION_TYPE="type";

const QString EventSubscriber::SUBSYSTEM_NAME="Event Subscriber";

EventSubscriber::EventSubscriber(QObject *parent) : QTcpServer(parent),
	settingClientID(SETTINGS_CATEGORY_AUTHORIZATION,"ClientID"),
	settingOAuthToken(SETTINGS_CATEGORY_AUTHORIZATION,"ServerToken"),
	secret(QUuid::createUuid().toString())
{
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
	Print(Console::GenerateMessage(SUBSYSTEM_NAME,QString("Requesting subscription to %1").arg(type)));
	QNetworkAccessManager *manager=new QNetworkAccessManager(this);
	connect(manager,&QNetworkAccessManager::finished,[this,manager](QNetworkReply *reply) {
		// FIXME: check the validity of this reply!
		Print(Console::GenerateMessage(SUBSYSTEM_NAME,reply->readAll()));
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
			QJsonObject({{"broadcaster_user_id","channel.follow"}})
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
	Print(Console::GenerateMessage(SUBSYSTEM_NAME,QString("Sending subscription request: %1").arg(QString(payload.toJson(QJsonDocument::Indented)))));
	manager->post(request,payload.toJson(QJsonDocument::Compact));
}

void EventSubscriber::DataAvailable()
{
	QTcpSocket *socket=SocketFromSignalSender();
	buffer.append(ReadFromSocket(socket));
	QStringList data=buffer.split("\r\n\r\n",StringConvert::Split::Behavior(StringConvert::Split::Behaviors::SKIP_EMPTY_PARTS)); // TODO: can I do a splitref here?
	buffer.clear();
	if (data.size() < 2)
	{
		Print(Console::GenerateMessage(SUBSYSTEM_NAME,"Incomplete notification received"));
		Print(Console::GenerateMessage(SUBSYSTEM_NAME,data.join("\n")));
		return;
	}

	const QString &request=data.at(1);
	Print(Console::GenerateMessage(SUBSYSTEM_NAME,"read",request));
	QByteArray response=ProcessRequest(request);
	if (response.size() > 0) WriteToSocket(response,socket); // TODO: std::optional
}

const QByteArray EventSubscriber::ProcessRequest(const QString &request) // TODO: should this return std::optional?
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
		Print(Console::GenerateMessage(SUBSYSTEM_NAME,"== JSON Parsed! =="));
	}

	if (json.object().contains(JSON_KEY_CHALLENGE)) return StringConvert::ByteArray(BuildResponse(json.object().value(JSON_KEY_CHALLENGE).toString()));

	/*if (json.object().contains(JSON_KEY_EVENT) && json.object().value(JSON_KEY_EVENT.toObject().contains(JSON_KEY_EVENT_REWARD))
	{
		QJsonObject event=json.object().value(JSON_KEY_EVENT).toObject();
		Print(Console::GenerateMessage((SUBSYSTEM_NAME,QString("== REWARD: %1|%2|%3").arg(event.value(JSON_KEY_EVENT_USER_NAME).toString(),event.value(JSON_KEY_EVENT_USER_INPUT).toString())))));
		return StringConvert::ByteArray(BuildReponse());
	}

	if (json.object().contains(JSON_KEY_EVENT) && json.object().value(JSON_KEY_EVENT.toObject().contains(JSON_KEY_EVENT_FOLLOW))
	{
		QJsonObject event=json.object().value(JSON_KEY_EVENT).toObject();
		Print(Console::GenerateMessage((SUBSYSTEM_NAME,QString("== FOLLOW: %1").arg(event.value(JSON_KEY_EVENT_USER_NAME).toString()))));
		return StringConvert::ByteArray(BuildReponse());
	}*/

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
