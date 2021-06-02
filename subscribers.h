#pragma once

#include <QTcpServer>
#include "settings.h"

inline const char *SUBSCRIPTION_TYPE_FOLLOW="channel.follow";
inline const char *SUBSCRIPTION_TYPE_REDEMPTION="channel.channel_points_custom_reward_redemption.add";
inline const char *SUBSCRIPTION_TYPE_CHEER="channel.cheer";
inline const char *SUBSCRIPTION_TYPE_RAID="channel.raid";
inline const char *SUBSCRIPTION_TYPE_SUBSCRIPTION="channel.subscribe";

enum class SubscriptionType
{
	CHANNEL_FOLLOW,
	CHANNEL_REDEMPTION,
	CHANNEL_CHEER,
	CHANNEL_RAID,
	CHANNEL_SUBSCRIPTION
};

class EventSubscriber : public QTcpServer
{
	Q_OBJECT
	using Headers=std::unordered_map<QString,QString>;
	using SubscriptionTypes=std::unordered_map<QString,SubscriptionType>;
public:
	EventSubscriber(const QString &channelOwnerID,QObject *parent=nullptr);
	~EventSubscriber();
	void Listen();
	void Subscribe(const QString &type);
	void DataAvailable();
protected:
	static const QString SUBSYSTEM_NAME;
	static const QString LINE_BREAK;
	static const char *SETTINGS_CATEGORY_EVENTSUB;
	const QString channelOwnerID;
	Setting settingClientID;
	Setting settingOAuthToken;
	Setting settingListenPort;
	Setting settingCallbackURL;
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
	void Subscription(const QString &viewer);
protected slots:
	void ConnectionAvailable();
};
