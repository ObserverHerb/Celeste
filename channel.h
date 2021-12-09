#pragma once

#include <QTcpSocket>
#include <QTimer>
#include "settings.h"
#include "entities.h"

class IRCSocket : public QTcpSocket
{
	Q_OBJECT
public:
	IRCSocket(QObject *parent=nullptr) : QTcpSocket(parent) { }
	virtual QByteArray Read();
};

class Channel : public QObject
{
	Q_OBJECT
	enum class Phase
	{
		CONNECT,
		AUTHENTICATE,
		JOIN_CHANNEL,
		DISPATCH
	};
public:
	Channel(PrivateSetting &settingAdministrator,PrivateSetting &settingOAuthToken,QObject *parent=nullptr) : Channel(settingAdministrator,settingOAuthToken,nullptr,parent) { }
	Channel(PrivateSetting &settingAdministrator,PrivateSetting &settingOAuthToken,IRCSocket *socket,QObject *parent=nullptr);
	~Channel();
	void Connect();
	void Disconnect();
	const QString Name() const { return settingChannel; }
	const std::chrono::milliseconds JoinDelay() const { return static_cast<std::chrono::milliseconds>(settingJoinDelay); }
protected:
	Phase phase;
	PrivateSetting &settingAdministrator;
	PrivateSetting &settingOAuthToken;
	ApplicationSetting settingChannel;
	ApplicationSetting settingJoinDelay;
	IRCSocket *ircSocket;
	void RequestAuthentication();
	void AuthenticationResponse(const QString &data);
	void RequestJoin();
	void JoinResponse(const QString &data);
signals:
	void Print(const QString &message,const QString operation=QString(),const QString subsystem=QString("channel socket"));
	void Dispatch(const QString &data);
	void Connected();
	void Disconnected();
	void Denied();
	void Joined();
	void Ping();
protected slots:
	void DataAvailable();
	void SocketError(QAbstractSocket::SocketError error);
	void Pong();
};
