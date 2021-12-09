#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringBuilder>
#include <QUuid>
#include "globals.h"
#include "eventsub.h"

const char *HEADER_SUBSCRIPTION_TYPE="Twitch-Eventsub-Subscription-Type";

const char *TWITCH_API_ENDPOINT_EVENTSUB="https://api.twitch.tv/helix/eventsub/subscriptions";

const char *JSON_KEY_CHALLENGE="challenge";
const char *JSON_KEY_EVENT="event";
const char *JSON_KEY_EVENT_REWARD="reward";
const char *JSON_KEY_EVENT_REWARD_TITLE="title";
const char *JSON_KEY_EVENT_FOLLOW="followed at";
const char *JSON_KEY_EVENT_USER_NAME="user_name";
const char *JSON_KEY_EVENT_USER_INPUT="user_input";
const char *JSON_KEY_EVENT_VIEWERS="viewers";
const char *JSON_KEY_EVENT_MESSAGE="message";
const char *JSON_KEY_EVENT_CHEER_AMOUNT="bits";
const char *JSON_KEY_SUBSCRIPTION="subscription";
const char *JSON_KEY_SUBSCRIPTION_TYPE="type";

EventSub::EventSub(Viewer::Local broadcaster,PrivateSetting &settingOAuthToken,PrivateSetting &settingClientID,PrivateSetting &settingCallbackURL,QObject *parent) : QObject(parent),
	broadcaster(broadcaster),
	settingClientID(settingClientID),
	settingOAuthToken(settingOAuthToken),
	settingCallbackURL(settingCallbackURL),
	secret(QUuid::createUuid().toString())
{
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_FOLLOW,SubscriptionType::CHANNEL_FOLLOW});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_REDEMPTION,SubscriptionType::CHANNEL_REDEMPTION});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_CHEER,SubscriptionType::CHANNEL_CHEER});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_RAID,SubscriptionType::CHANNEL_RAID});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_SUBSCRIPTION,SubscriptionType::CHANNEL_SUBSCRIPTION});
}

void EventSub::Subscribe(const QString &type)
{
	const char *OPERATION="subscribe";
	emit Print(QString("Requesting subscription to %1").arg(type),OPERATION);
	QNetworkAccessManager *manager=new QNetworkAccessManager(this);
	connect(manager,&QNetworkAccessManager::finished,[this,manager](QNetworkReply *reply) {
		// FIXME: check the validity of this reply!
		emit Print(StringConvert::Dump(reply->readAll()),"twitch");
		manager->disconnect();
	});
	QUrl query(TWITCH_API_ENDPOINT_EVENTSUB);
	QNetworkRequest request(query);
	request.setRawHeader("Authorization",StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(settingOAuthToken))));
	request.setRawHeader("Client-ID",settingClientID);
	request.setRawHeader("Content-Type","application/json");
	QJsonDocument payload(QJsonObject({
		{"type",type},
		{"version","1"},
		{
			"condition",
			QJsonObject({{type == SUBSCRIPTION_TYPE_RAID ? "to_broadcaster_user_id" : "broadcaster_user_id",broadcaster.ID()}})
		},
		{
			"transport",
			QJsonObject({
				{"method","webhook"},
				{"callback",QString("https://%1").arg(static_cast<QString>(settingCallbackURL))},
				{"secret",secret}
			})
		}
	}));
	emit Print(QString("Sending subscription request: %1").arg(StringConvert::Dump(payload.toJson(QJsonDocument::Indented))),OPERATION);
	manager->post(request,payload.toJson(QJsonDocument::Compact));
}

void EventSub::ParseRequest(qintptr socketID,const QUrlQuery &query,const std::unordered_map<QString,QString> &headers,const QString &content)
{
	const char *PARSE_REQUEST="parse request";
	if (headers.find(HEADER_SUBSCRIPTION_TYPE) == headers.end() || subscriptionTypes.find(headers.at(HEADER_SUBSCRIPTION_TYPE)) == subscriptionTypes.end())
	{
		emit Print("Ignoring request with no subscription type",PARSE_REQUEST);
		return;
	}

	QJsonParseError jsonError;
	QJsonDocument json=QJsonDocument::fromJson(StringConvert::ByteArray(content.trimmed()),&jsonError);
	if (json.isNull())
	{
		Print(QString("Invalid JSON: %1").arg(jsonError.errorString()),PARSE_REQUEST);
		return;
	}

	if (json.object().contains(JSON_KEY_CHALLENGE))
	{
		QString response=json.object().value(JSON_KEY_CHALLENGE).toString();
		emit Print(StringConvert::Dump(response),"challenge");
		emit Response(socketID,response);
		return;
	}

	QJsonObject event=json.object().value(JSON_KEY_EVENT).toObject(); // TODO: check event for validity
	switch (subscriptionTypes.at(headers.at(HEADER_SUBSCRIPTION_TYPE)))
	{
	case SubscriptionType::CHANNEL_FOLLOW:
		emit Follow();
		emit Response(socketID);
		return;
	case SubscriptionType::CHANNEL_REDEMPTION:
	{
		emit Redemption(event.value(JSON_KEY_EVENT_USER_NAME).toString(),event.value(JSON_KEY_EVENT_REWARD).toObject().value(JSON_KEY_EVENT_REWARD_TITLE).toString(),event.value(JSON_KEY_EVENT_USER_INPUT).toString());
		emit Response(socketID);
		return;
	}
	case SubscriptionType::CHANNEL_CHEER:
		emit Cheer(event.value(JSON_KEY_EVENT_USER_NAME).toString(),event.value(JSON_KEY_EVENT_CHEER_AMOUNT).toVariant().toUInt(),event.value(JSON_KEY_EVENT_MESSAGE).toString());
		emit Response(socketID);
		return;
	case SubscriptionType::CHANNEL_RAID:
		emit Raid(event.value("from_broadcaster_user_name").toString(),event.value(JSON_KEY_EVENT_VIEWERS).toVariant().toUInt());
		emit Response(socketID);
		return;
	case SubscriptionType::CHANNEL_SUBSCRIPTION:
		emit Subscription(event.value(JSON_KEY_EVENT_USER_NAME).toString());
		emit Response(socketID);
		return;
	}

	emit Response(socketID);
}

