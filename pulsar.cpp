#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLocalSocket>
#include <QFile>
#include "pulsar.h"
#include "globals.h"

const char *TRIGGER_LIST_FILENAME="pulsar.json";
const char *JSON_KEY_TRIGGER="trigger";
const char *JSON_KEY_COMMAND="command";
const char *JSON_KEY_SOURCES="sources";

bool Pulsar::LoadTriggers()
{
	static const char *OPERATION="load Pulsar triggers";

	QFile operationListFile(Filesystem::DataPath().filePath(TRIGGER_LIST_FILENAME));
	if (!operationListFile.open(QIODevice::ReadWrite))
	{
		emit Print(QString("Failed to open command list file: %1").arg(operationListFile.fileName()),OPERATION);
		return false;
	}

	const JSON::ParseResult parsedJSON=JSON::Parse(operationListFile.readAll());
	if (!parsedJSON)
	{
		emit Print("Failed to parse JSON",OPERATION);
		return false;
	}

	const QJsonArray objects=parsedJSON().array();
	for (const QJsonValue &jsonValue : objects)
	{
		QJsonObject jsonObjectTrigger=jsonValue.toObject();

		auto jsonFieldTrigger=jsonObjectTrigger.find(JSON_KEY_TRIGGER);
		if (jsonFieldTrigger == jsonObjectTrigger.end())
		{
			emit Print("No trigger specified",OPERATION);
			return false;
		}

		auto jsonFieldSources=jsonObjectTrigger.find(JSON_KEY_SOURCES);
		if (jsonFieldSources == jsonObjectTrigger.end())
		{
			emit Print("No sources specified",OPERATION);
			return false;
		}

		const QString name=jsonFieldTrigger->toString();
		if (triggers.try_emplace(name,jsonFieldSources->toArray()).second)
		{
			auto jsonFieldCommand=jsonObjectTrigger.find(JSON_KEY_COMMAND);
			if (jsonFieldCommand != jsonObjectTrigger.end()) commandCrossReference.try_emplace(name,jsonFieldCommand->toString());
		}
	}

	return true;
}

void Pulsar::Pulse(const QString &trigger,const QString &command)
{
	auto candidate=commandCrossReference.find(trigger);
	if (candidate != commandCrossReference.end() && candidate->second != command)
	{
		emit Print(uR"(Trigger "%1" is not attached to command "%2")"_s.arg(trigger,command));
		return;
	}

	Pulse(trigger);
}

void Pulsar::Pulse(const QString &trigger)
{
	static const char *OPERATION="dispatch pulse";

	auto payload=triggers.find(trigger);
	if (payload == triggers.end())
	{
		emit Print("Unrecognized trigger forwarded to Pulsar",OPERATION);
		return;
	}

	QLocalSocket *socket=new QLocalSocket();
	connect(socket,&QLocalSocket::connected,socket,[socket,trigger,payload]() {
		socket->write(QJsonDocument(payload->second).toJson(QJsonDocument::Compact)+'\n');
	});
	connect(socket,&QLocalSocket::errorOccurred,socket,[this,socket]() {
		emit Print(QString("Failed to connect to Pulsar: %1").arg(socket->errorString()),OPERATION);
	});
	connect(socket,&QLocalSocket::bytesWritten,socket,[socket]() {
		socket->deleteLater();
	});
	socket->connectToServer(PULSAR_SOCKET_NAME,QIODevice::WriteOnly);
}
