#pragma once

#include <QDateTime>
#include "settings.h"
#include "entities.h"

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

class EventSub : public QObject
{
	Q_OBJECT
	using SubscriptionTypes=std::unordered_map<QString,SubscriptionType>;
public:
	EventSub(Viewer::Local broadcaster,PrivateSetting &settingOAuthToken,PrivateSetting &settingClientID,PrivateSetting &settingCallbackURL,QObject *parent=nullptr);
	void Subscribe(const QString &type);
protected:
	Viewer::Local broadcaster;
	PrivateSetting &settingClientID;
	PrivateSetting &settingOAuthToken;
	PrivateSetting &settingCallbackURL;
	const QString secret;
	QString buffer;
	SubscriptionTypes subscriptionTypes; // TODO: find a better name for this
	const QByteArray ProcessRequest(const SubscriptionType type,const QString &data);
	const QString BuildResponse(const QString &data=QString()) const;
signals:
	void Print(const QString &message,const QString &operation=QString(),const QString &subsystem=QString("EventSub")) const;
	void Response(qintptr socketID,const QString &content=QString("Acknowledged: %1").arg(QDateTime::currentDateTime().toString()));
	void Follow();
	void Redemption(const QString &viewer,const QString &rewardTitle,const QString &message);
	void Cheer(const QString &viewer,const unsigned int count,const QString &message);
	void Raid(const QString &raider,const unsigned int viewers);
	void Subscription(const QString &viewer);
public slots:
	void ParseRequest(qintptr socketID,const QUrlQuery &query,const std::unordered_map<QString,QString> &headers,const QString &content);
};
