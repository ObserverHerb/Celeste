#include "server.h"
#include "globals.h"

const char *SETTINGS_CATEGORY_SERVER="Server";

const char *HEADER_DELIMITER=":";
const char *LINE_BREAK="\r\n";

Server::Server(QObject *parent) : QTcpServer(parent),
	settingListenPort(SETTINGS_CATEGORY_SERVER,"Port",4443)
{
	//connect(this,&EventSubscriber::acceptError,this,QOverload<QAbstractSocket::SocketError>::of(&EventSubscriber::)) // FIXME: handle error here?
	connect(this,&Server::newConnection,this,&Server::ConnectionAvailable);
}

Server::~Server()
{
	while (!sockets.empty()) sockets.begin()->second->deleteLater();
}

bool Server::Listen()
{
	const char *OPERATION="listen";
	emit Print(QString("Using network address %1 and port %2").arg("any",StringConvert::Integer(static_cast<int>(settingListenPort))),OPERATION);
	if (!listen(QHostAddress::Any,settingListenPort))
	{
		emit Print(QString("Error while listening on address/port: %1").arg(errorString()),OPERATION);
		return false;
	}
	return true;
}

void Server::DataAvailable(qintptr socketID)
{
	const char *OPERATION="receive";

	QString buffer;
	if (std::optional<QByteArray> incomingData=SocketRead(socketID); !incomingData)
		return;
	else
		buffer=*incomingData;
	StringConvert::Dump(buffer);

	QStringList sections=buffer.split(QString("%1%1").arg(LINE_BREAK),StringConvert::Split::Behavior(StringConvert::Split::Behaviors::SKIP_EMPTY_PARTS));
	if (sections.size() < 1)
	{
		Print("Invalid HTTP request",OPERATION);
		return;
	}

	QStringList headers=sections.at(0).split(LINE_BREAK);
	QUrlQuery query(QUrl(headers.takeFirst().section(" ",1,1)).query());
	emit Dispatch(socketID,query,ParseHeaders(headers),sections.size() < 2 ? QString() : sections.at(1));
}


void Server::ConnectionAvailable()
{
	QTcpSocket *socket=nextPendingConnection();
	qintptr socketID=socket->socketDescriptor();
	sockets.insert({socketID,socket});
	connect(socket,&QTcpSocket::readyRead,[this,socketID]() {
		emit DataAvailable(socketID);
	});
	connect(socket,&QTcpSocket::disconnected,[this,socketID]() {
		sockets.at(socketID)->deleteLater();
		sockets.erase(socketID);
	});
}

std::optional<QByteArray> Server::SocketRead(qintptr socketID) const
{
	if (sockets.find(socketID) == sockets.end() || !sockets.at(socketID))
	{
		Print("Tried to read from non-existent socket");
		return std::nullopt;
	}
	return sockets.at(socketID)->readAll();
}

const std::unordered_map<QString,QString> Server::ParseHeaders(const QStringList &lines)
{
	std::unordered_map<QString,QString> headers;
	for (const QString &line : lines)
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

void Server::SocketWrite(qintptr socketID,const QString &data) const
{
	const char *OPERATION="send";
	if (sockets.find(socketID) == sockets.end() || !sockets.at(socketID))
	{
		Print("Tried to write to non-existent socket",OPERATION);
		return;
	}
	const QString response=QString("HTTP/1.1 200 OK%1Content-Type: text/plain%1%1%2").arg(LINE_BREAK,data);
	if (int written=sockets.at(socketID)->write(StringConvert::ByteArray(response)); written < data.size())
		Print(QString("Partial write: %1 or %2").arg(written,data.size()),OPERATION);
	else
		Print(StringConvert::Dump(response),OPERATION);
	sockets.at(socketID)->flush();
	sockets.at(socketID)->disconnectFromHost();
}
