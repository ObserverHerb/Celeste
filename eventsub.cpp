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
const char *JSON_KEY_EVENT_USER_LOGIN="user_login";
const char *JSON_KEY_EVENT_USER_INPUT="user_input";
const char *JSON_KEY_EVENT_VIEWERS="viewers";
const char *JSON_KEY_EVENT_MESSAGE="message";
const char *JSON_KEY_EVENT_CHEER_AMOUNT="bits";
const char *JSON_KEY_SUBSCRIPTION="subscription";
const char *JSON_KEY_SUBSCRIPTION_TYPE="type";

EventSub::EventSub(Security &security,QObject *parent) : QObject(parent),
	security(security),
	secret(QUuid::createUuid().toString())
{
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_FOLLOW,SubscriptionType::CHANNEL_FOLLOW});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_REDEMPTION,SubscriptionType::CHANNEL_REDEMPTION});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_CHEER,SubscriptionType::CHANNEL_CHEER});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_RAID,SubscriptionType::CHANNEL_RAID});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_SUBSCRIPTION,SubscriptionType::CHANNEL_SUBSCRIPTION});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_RESUBSCRIPTION,SubscriptionType::CHANNEL_SUBSCRIPTION});
}

void EventSub::Subscribe(const QString &type)
{
	const char *OPERATION="subscribe";
	QUrl callbackURL(security.CallbackURL());
	callbackURL.setScheme("https");
	emit Print(QString("Requesting subscription to %1").arg(type),OPERATION);
	Network::Request({TWITCH_API_ENDPOINT_EVENTSUB},Network::Method::POST,[this](QNetworkReply *reply) {
		// FIXME: check the validity of this reply!
		emit Print(StringConvert::Dump(reply->readAll()),"twitch");
	},{},{
		{"Authorization",StringConvert::ByteArray(QString("Bearer %1").arg(static_cast<QString>(security.ServerToken())))},
		{"Client-ID",security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON}
	},QJsonDocument(QJsonObject({
		{"type",type},
		{"version","1"},
		{
			"condition",
			QJsonObject({{type == SUBSCRIPTION_TYPE_RAID ? "to_broadcaster_user_id" : "broadcaster_user_id",security.AdministratorID()}})
		},
		{
			"transport",
			QJsonObject({
				{"method","webhook"},
				{"callback",callbackURL.toString()},
				{"secret",secret}
			})
		}
	})).toJson(QJsonDocument::Compact));
}

void EventSub::ParseRequest(qintptr socketID,const QUrlQuery &query,const std::unordered_map<QString,QString> &headers,const QString &content)
{
	const char *PARSE_REQUEST="parse request";
	auto headerSubscriptionType=headers.find(HEADER_SUBSCRIPTION_TYPE);
	if (headerSubscriptionType == headers.end())
	{
		emit Print("Ignoring request with no subscription type",PARSE_REQUEST);
		return;
	}

	auto subscriptionType=subscriptionTypes.find(headerSubscriptionType->second);
	if (subscriptionType == subscriptionTypes.end()) return;

	const JSON::ParseResult parsedJSON=JSON::Parse(StringConvert::ByteArray(content.trimmed()));
	if (!parsedJSON)
	{
		Print(QString("Invalid JSON: %1").arg(parsedJSON.error),PARSE_REQUEST);
		return;
	}

	QJsonObject jsonObject=parsedJSON().object();
	if (auto challenge=jsonObject.find(JSON_KEY_CHALLENGE); challenge != jsonObject.end())
	{
		QString response=challenge->toString();
		emit Print(StringConvert::Dump(response),"challenge");
		emit Response(socketID,response);
		return;
	}

	auto event=jsonObject.find(JSON_KEY_EVENT);
	if (event == jsonObject.end())
	{
		Print("Event field missing from request");
		return;
	}

	QJsonObject eventObject=event->toObject();
	switch (subscriptionType->second)
	{
	case SubscriptionType::CHANNEL_FOLLOW:
		emit Follow();
		break;
	case SubscriptionType::CHANNEL_REDEMPTION:
		emit Redemption(eventObject.value(JSON_KEY_EVENT_USER_LOGIN).toString(),eventObject.value(JSON_KEY_EVENT_USER_NAME).toString(),eventObject.value(JSON_KEY_EVENT_REWARD).toObject().value(JSON_KEY_EVENT_REWARD_TITLE).toString(),eventObject.value(JSON_KEY_EVENT_USER_INPUT).toString());
		break;
	case SubscriptionType::CHANNEL_CHEER:
		emit Cheer(eventObject.value(JSON_KEY_EVENT_USER_NAME).toString(),eventObject.value(JSON_KEY_EVENT_CHEER_AMOUNT).toVariant().toUInt(),eventObject.value(JSON_KEY_EVENT_MESSAGE).toString().section(" ",1));
		break;
	case SubscriptionType::CHANNEL_RAID:
		emit Raid(eventObject.value("from_broadcaster_user_name").toString(),eventObject.value(JSON_KEY_EVENT_VIEWERS).toVariant().toUInt());
		break;
	case SubscriptionType::CHANNEL_SUBSCRIPTION:
		emit Subscription(eventObject.value(JSON_KEY_EVENT_USER_NAME).toString());
		break;
	}

	emit Response(socketID);
}

