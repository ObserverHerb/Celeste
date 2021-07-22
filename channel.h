#pragma once

#include <QTcpSocket>
#include "receivers.h"
#include "settings.h"

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
public:
	Channel(QObject *parent=nullptr);
	~Channel();
	void Connect();
	const QString Name() const { return settingChannel; }
protected:
	Setting settingChannel;
	Setting settingJoinDelay;
	IRCSocket *ircSocket;
	AuthenticationReceiver *authenticationReceiver;
	ChannelJoinReceiver *channelJoinReceiver;
	ChatMessageReceiver *chatMessageReceiver;
	QMetaObject::Connection dispatch;
	static const char *OPERATION_CHANNEL;
	static const char *OPERATION_CONNECTION;
	static const char *OPERATION_AUTHORIZATION;
	static const char *OPERATION_SEND;
	static const char *OPERATION_RECEIVE;
	void Authorize();
signals:
	void Print(const QString text);
	void Log(const QString &message);
	void Dispatch(const QString data);
	void Joined(ChatMessageReceiver &chatMessageReceiver);
	void Ping();
protected slots:
	void DataAvailable();
	void SocketError(QAbstractSocket::SocketError error);
	virtual void Join();
	void Connected();
	void Disconnected();
	void Authorized();
	void Joined();
	void Follow();
	void Pong();
};
