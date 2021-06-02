#pragma once

#include <QTcpServer>
#include "settings.h"

inline const char *SUBSCRIPTION_TYPE_FOLLOW="channel.follow";
inline const char *SUBSCRIPTION_TYPE_REDEPTION="channel.challen_points_custom_reward_redemption.add";

class EventSubscriber : public QTcpServer
{
	Q_OBJECT
public:
	EventSubscriber(QObject *parent=nullptr);
	~EventSubscriber();
	void Listen();
	void Subscribe(const QString &type);
	void DataAvailable();
protected:
	static const QString SUBSYSTEM_NAME;
	Setting settingClientID;
	Setting settingOAuthToken;
	const QString secret;
	QString buffer;
	const QByteArray ProcessRequest(const QString &request);
	const QString BuildResponse(const QString &data=QString()) const;
	virtual QTcpSocket* SocketFromSignalSender() const;
	virtual QByteArray ReadFromSocket(QTcpSocket *socket=nullptr) const;
	virtual bool WriteToSocket(const QByteArray &data,QTcpSocket *socket=nullptr) const;
signals:
	void Print(const QString &message) const;
protected slots:
	void ConnectionAvailable();
};
