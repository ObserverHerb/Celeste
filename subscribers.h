#pragma once

#include <QTcpServer>
#include "settings.h"

enum class SubscriptionType
{
	CHANNEL_FOLLOW,
	CHANNEL_REDEMPTION,
	CHANNEL_CHEER,
	CHANNEL_RAID
};

class EventSubscriber : public QTcpServer
{
	Q_OBJECT
	using Headers=std::unordered_map<QString,QString>;
	using SubscriptionTypes=std::unordered_map<QString,SubscriptionType>;
public:
	EventSubscriber(QObject *parent=nullptr);
	~EventSubscriber();
	void Listen();
	void Subscribe(const QString &type);
	void DataAvailable();
protected:
	static const QString SUBSYSTEM_NAME;
	static const QString LINE_BREAK;
	Setting settingClientID;
	Setting settingOAuthToken;
	const QString secret;
	QString buffer;
	SubscriptionTypes subscriptionTypes; // TODO: find a better name for this
	const Headers ParseHeaders(const QString &data);
	const QByteArray ProcessRequest(const SubscriptionType type,const QString &data);
	const QString BuildResponse(const QString &data=QString()) const;
	virtual QTcpSocket* SocketFromSignalSender() const;
	virtual QByteArray ReadFromSocket(QTcpSocket *socket=nullptr) const;
	virtual bool WriteToSocket(const QByteArray &data,QTcpSocket *socket=nullptr) const;
signals:
	void Print(const QString &message) const;
	void Follow();
	void Redemption(const QString &viewer,const QString &rewardTitle,const QString &message);
	void Cheer(const QString &viewer,const unsigned int count,const QString &message);
	void Raid(const QString &raider,const unsigned int viewers);
protected slots:
	void ConnectionAvailable();
};
