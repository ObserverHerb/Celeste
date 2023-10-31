#pragma once

#include <QDateTime>
#include <QWebSocket>
#include <QJsonObject>
#include <queue>
#include "settings.h"
#include "security.h"
#include "entities.h"

inline const char *SUBSCRIPTION_TYPE_FOLLOW="channel.follow";
inline const char *SUBSCRIPTION_TYPE_REDEMPTION="channel.channel_points_custom_reward_redemption.add";
inline const char *SUBSCRIPTION_TYPE_CHEER="channel.cheer";
inline const char *SUBSCRIPTION_TYPE_RAID="channel.raid";
inline const char *SUBSCRIPTION_TYPE_SUBSCRIPTION="channel.subscribe";
inline const char *SUBSCRIPTION_TYPE_RESUBSCRIPTION="channel.subscription.message";
inline const char *SUBSCRIPTION_TYPE_HYPE_TRAIN_START="channel.hype_train.begin";
inline const char *SUBSCRIPTION_TYPE_HYPE_TRAIN_PROGRESS="channel.hype_train.progress";
inline const char *SUBSCRIPTION_TYPE_HYPE_TRAIN_END="channel.hype_train.end";

enum class MessageType
{
	WELCOME,
	KEEPALIVE,
	NOTIFICATION
};

enum class SubscriptionType
{
	UNKNOWN,
	CHANNEL_FOLLOW,
	CHANNEL_REDEMPTION,
	CHANNEL_CHEER,
	CHANNEL_RAID,
	CHANNEL_SUBSCRIPTION,
	CHANNEL_HYPE_TRAIN
};

class EventSub : public QObject
{
	Q_OBJECT
	using MessageTypes=std::unordered_map<QString,MessageType>;
	using SubscriptionTypes=std::unordered_map<QString,SubscriptionType>;
public:
	EventSub(Security &security,QObject *parent=nullptr);
	void Subscribe();
	void Subscribe(const QString &type);
protected:
	Security &security;
	QString buffer;
	MessageTypes messageTypes;
	SubscriptionTypes subscriptionTypes; // TODO: find a better name for this
	std::queue<QString> defaultTypes;
	QWebSocket socket;
	QString sessionID;
	QTimer keepalive;
	ApplicationSetting settingURL;
	static const char *SETTINGS_CATEGORY_EVENTS; // TODO: can this be removed later when I switch to modules (linking conflicts with definition in bot.cpp)
	void Connect();
	void NextSubscription();
	const QByteArray ProcessRequest(const SubscriptionType type,const QString &data);
	const QString BuildResponse(const QString &data=QString()) const;
signals:
	void Print(const QString &message,const QString &operation=QString(),const QString &subsystem=QString("EventSub")) const;
	void EventSubscriptionFailed(const QString &type);
	void Unauthorized();
	void RateLimitHit();
	void Follow();
	void Redemption(const QString &login,const QString &viewer,const QString &rewardTitle,const QString &message);
	void Cheer(const QString &viewer,const unsigned int count,const QString &message);
	void Raid(const QString &raider,const unsigned int viewers);
	void HypeTrain(int level,double progress);
	void ChannelSubscription(const QString &login,const QString &displayName);
	void EventSubscription(const QString &id,const QString &type,const QDateTime &creationDate,const QString &callbackURL);
	void EventSubscriptionRemoved(const QString &id);
	void Connected();
	void Disconnected();
protected slots:
	void ParseMessage(QString message);
	void ParseWelcome(QJsonObject payload);
	void ParseNotification(QJsonObject payload);
	void Dead();
	void SocketClosed();
public slots:
	void RequestEventSubscriptionList();
	void RemoveEventSubscription(const QString &id);
};
