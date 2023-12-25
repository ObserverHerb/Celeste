#pragma once

#include <QTcpSocket>
#include <QTimer>
#include "settings.h"
#include "security.h"
#include "entities.h"

class IRCSocket : public QTcpSocket
{
	Q_OBJECT
public:
	IRCSocket(QObject *parent=nullptr) : QTcpSocket(parent) { }
	QByteArray Read();
	std::optional<char> Pop();
signals:
	void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("network socket"));
};

struct Hostmask
{
	QString nick;
	QString user;
	QString host;
};

class Channel : public QObject
{
	Q_OBJECT
	enum class Phase
	{
		CONNECT,
		AUTHENTICATE,
		CAPABILITIES,
		JOIN_CHANNEL,
		DISPATCH
	};
public:
	Channel(Security &security,QObject *parent=nullptr) : Channel(security,nullptr,parent) { }
	Channel(Security &security,IRCSocket *socket,QObject *parent=nullptr);
	~Channel();
	void Connect();
	void Disconnect();
	ApplicationSetting& Name();
	ApplicationSetting& Protection();
protected:
	Security &security;
	ApplicationSetting settingChannel;
	ApplicationSetting settingProtect;
	IRCSocket *ircSocket;
	void ParseMessage(const QString message);
	void DispatchMessage(QString prefix,QString source,QString command,QStringList parameters,QString finalParamter);
	void SendMessage(QString prefix,QString command,QStringList parameters,QString finalParamter);
	void ParseCapabilities(const QStringList &parameters,const QString &capabilities);
	void DispatchCapabilities(const QString &subCommand,const QStringList &capabilities);
	void ParseNotice(const QString &message);
	void ParseUserNotice(const QString &prefix,const QString &message);
	void Authenticate();
	void RequestCapabilities();
	void RequestJoin();
	void DispatchJoin(const QString &source);
	void DispatchPart(const QString &source);
	std::optional<Hostmask> ParseSource(const QString &source);
signals:
	void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("channel"));
	void Dispatch(const QString &prefix,const QString &source,const QStringList &parameters,const QString &message);
	void Connected();
	void Disconnected();
	void Denied();
	void Joined();
	void Joined(const QString &user);
	void Parted(const QString &user);
	void Ping(const QString &token);
protected slots:
	void DataAvailable();
	void SocketError(QAbstractSocket::SocketError error);
	void Pong(const QString &token);
};
