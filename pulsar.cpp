#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QMessageBox>
#include "pulsar.h"
#include "globals.h"

const char *TRIGGER_LIST_FILENAME="pulsar.json";
const char *JSON_KEY_TRIGGER="trigger";
const char *JSON_KEY_COMMAND="command";
const char *JSON_KEY_SOURCES="sources";
const char *JSON_KEY_DIMENSIONS="size";
const char *JSON_KEY_DIMENSIONS_X="width";
const char *JSON_KEY_DIMENSIONS_Y="height";
const char *SETTINGS_CATEGORY_PULSAR="Pulsar";

Pulsar::Pulsar() : socket(new QLocalSocket(this)),
	settingEnabled(SETTINGS_CATEGORY_PULSAR,"Enabled",true),
	settingReconnectDelay(SETTINGS_CATEGORY_PULSAR,"ReconnectDelay",5000)
{
	reconnectDelay.setInterval(static_cast<int>(settingReconnectDelay));
	connect(&reconnectDelay,&QTimer::timeout,this,&Pulsar::Reconnect);

	connect(socket,&QLocalSocket::readyRead,this,&Pulsar::Data);
	connect(socket,&QLocalSocket::errorOccurred,this,&Pulsar::Error);
}

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

		// check if this is an event rather than a trigger and stage accordingly
		if (auto jsonFieldScene=jsonObjectTrigger.find(JSON_KEY_SCENE); jsonFieldScene != jsonObjectTrigger.end())
		{
			if (auto jsonFieldDimensions=jsonObjectTrigger.find(JSON_KEY_DIMENSIONS); jsonFieldDimensions != jsonObjectTrigger.end())
			{
				const QJsonObject jsonObjectDimensions=jsonFieldDimensions->toObject();
				auto jsonFieldDimensionsX=jsonObjectDimensions.find(JSON_KEY_DIMENSIONS_X);
				auto jsonFieldDimensionsY=jsonObjectDimensions.find(JSON_KEY_DIMENSIONS_Y);

				if (jsonFieldDimensionsX == jsonObjectDimensions.end() || jsonFieldDimensionsY == jsonObjectDimensions.end())
				{
					emit Print("Invalid dimensions in scene entry",OPERATION);
					continue;
				}

				dimensions.try_emplace(jsonFieldScene->toString(),jsonFieldDimensionsX->toInt(),jsonFieldDimensionsY->toInt());
				continue;
			}
		}

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

void Pulsar::Error(QLocalSocket::LocalSocketError error)
{
	// connection problem
	if (error == QLocalSocket::ConnectionRefusedError ||
		error == QLocalSocket::PeerClosedError ||
		error == QLocalSocket::ServerNotFoundError ||
		error == QLocalSocket::SocketTimeoutError ||
		error == QLocalSocket::ConnectionError)
	{
		const QString failureMessage=QString("Failed to connect to Pulsar: %1").arg(socket->errorString());
		emit Print(failureMessage,"connect");
		QMessageBox failedDialog;
		failedDialog.setWindowTitle("Connection to Pulsar Failed");
		failedDialog.setText(failureMessage+"\n\nThis can happen if the bot is started before OBS Studio or if the Pulsar plugin has not been installed. Attempting to connect to Pulsar can be disabled in options.\n\nWould you like to try connecting again?");
		failedDialog.setIcon(QMessageBox::Warning);
		failedDialog.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
		failedDialog.setDefaultButton(QMessageBox::Yes);
		if (failedDialog.exec() == QMessageBox::Yes) reconnectDelay.start();
		return;
	}

	// other problem
	emit Print(QString("Failed to communicate with Pulsar: %1").arg(socket->errorString()),"write");
}

void Pulsar::Connect()
{
	if (settingEnabled) socket->connectToServer(PULSAR_SOCKET_NAME,QIODevice::ReadWrite);
}

void Pulsar::Reconnect()
{
	reconnectDelay.stop();
	Connect();
}

void Pulsar::Data()
{
	static const char *OPERATION_READ="read";

	try
	{
		QByteArray data=socket->readAll().trimmed();
		emit Print(QString("Data received: ")+data,OPERATION_READ);
		const JSON::ParseResult parsedJSON=JSON::Parse(data);
		if (!parsedJSON) throw std::runtime_error("Failed to parse incoming data");
		const QJsonObject object=parsedJSON().object();

		if (auto scene=object.find(JSON_KEY_SCENE); scene != object.end())
		{
			if (auto candidate=dimensions.find(scene->toString()); candidate != dimensions.end())
				emit Dimensions(candidate->second);
			else
				emit Dimensions({});
		}
	}

	catch (const std::runtime_error &exception)
	{
		emit Print(exception.what(),OPERATION_READ);
	}
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

	socket->write(QJsonDocument(payload->second).toJson(QJsonDocument::Compact)+'\n');
}

ApplicationSetting& Pulsar::Enabled()
{
	return settingEnabled;
}
