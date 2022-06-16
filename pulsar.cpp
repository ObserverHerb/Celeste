#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLocalSocket>
#include <QFile>
#include "pulsar.h"
#include "globals.h"

const char *TRIGGER_LIST_FILENAME="pulsar.json";
const char *JSON_KEY_TRIGGER="trigger";
const char *JSON_KEY_SOURCES="sources";

bool Pulsar::LoadTriggers()
{
	try
	{
		QFile operationListFile(Filesystem::DataPath().filePath(TRIGGER_LIST_FILENAME));
		if (!operationListFile.open(QIODevice::ReadWrite)) throw std::runtime_error(QString("Failed to open command list file: %1").arg(operationListFile.fileName()).toStdString());

		QJsonParseError jsonError;
		QByteArray data=operationListFile.readAll();
		QJsonDocument json=QJsonDocument::fromJson(data,&jsonError);
		if (json.isNull()) throw std::runtime_error(jsonError.errorString().toStdString());

		const QJsonArray objects=json.array();
		for (const QJsonValue &jsonValue : objects)
		{
			QJsonObject jsonObject=jsonValue.toObject();
			if (jsonObject.isEmpty()) throw std::runtime_error("Malformatted source object");

			if (!jsonObject.contains(JSON_KEY_TRIGGER)) throw std::runtime_error("No trigger specified");
			const QString message=jsonObject.value(JSON_KEY_TRIGGER).toString();

			if (!jsonObject.contains(JSON_KEY_SOURCES)) throw std::runtime_error("No sources specified");
			triggers[message]=jsonObject.value(JSON_KEY_SOURCES).toArray();
		}
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(QString("Error: %1").arg(exception.what()),"load Pulsar triggers");
		return false;
	}

	return true;
}

void Pulsar::Pulse(const QString &trigger)
{
	try
	{
		if (!triggers.contains(trigger)) throw std::runtime_error("Unrecognized trigger forwarded to Pulsar");

		QLocalSocket *socket=new QLocalSocket();
		connect(socket,&QLocalSocket::connected,socket,[this,socket,trigger]() {
			socket->write(QJsonDocument(triggers.at(trigger)).toJson(QJsonDocument::Compact)+'\n');
		});
		connect(socket,&QLocalSocket::errorOccurred,socket,[this,socket]() {
			emit Print(QString("Failed to connect to Pulsar: %1").arg(socket->errorString()));
		});
		connect(socket,&QLocalSocket::bytesWritten,socket,[socket]() {
			socket->deleteLater();
		});
		socket->connectToServer(PULSAR_SOCKET_NAME,QIODevice::WriteOnly);
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(exception.what(),"dispatch pulse");
	}
}
