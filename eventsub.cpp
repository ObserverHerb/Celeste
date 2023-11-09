#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringBuilder>
#include <QUuid>
#include "globals.h"
#include "eventsub.h"

const char *HEADER_SUBSCRIPTION_TYPE="Twitch-Eventsub-Subscription-Type";

const char *TWITCH_API_ENDPOINT_EVENTSUB="https://api.twitch.tv/helix/eventsub/subscriptions";
const char *TWITCH_API_OPERATION_SUBSCRIBE="subscribe to event";

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
}

void EventSub::Subscribe()
{
	defaultTypes.push(SUBSCRIPTION_TYPE_REDEMPTION);
	defaultTypes.push(SUBSCRIPTION_TYPE_RAID);
	defaultTypes.push(SUBSCRIPTION_TYPE_SUBSCRIPTION);
	defaultTypes.push(SUBSCRIPTION_TYPE_CHEER);
	Subscribe(defaultTypes.front());
}

void EventSub::Subscribe(const QString &type)
{
	QUrl callbackURL(security.CallbackURL());
	callbackURL.setScheme("https");
	emit Print(u"Requesting subscription to %1"_s.arg(type),TWITCH_API_OPERATION_SUBSCRIBE);
	Network::Request({TWITCH_API_ENDPOINT_EVENTSUB},Network::Method::POST,[this,type](QNetworkReply *reply) {
		emit Print(StringConvert::Dump(reply->readAll()),TWITCH_API_OPERATION_SUBSCRIBE);
		switch (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
		{
		case 202:
			emit Print(u"Successfully subscribed to %1"_s.arg(type),TWITCH_API_OPERATION_SUBSCRIBE);
			NextSubscription();
			return;
		case 400:
			emit Print(u"The subscription request was malformatted"_s,TWITCH_API_OPERATION_SUBSCRIBE);
			break;
		case 401:
			emit Print(u"Invalid OAuth token or authorization header was malformatted"_s,TWITCH_API_OPERATION_SUBSCRIBE);
			emit Unauthorized();
			return;
		case 403:
			emit Print(u"Access token is missing the required scopes"_s,TWITCH_API_OPERATION_SUBSCRIBE);
			break;
		case 409:
			emit Print(u"Subscription already exists"_s,TWITCH_API_OPERATION_SUBSCRIBE);
			NextSubscription();
			return;
		case 429:
			emit Print(u"Too many subscription requests"_s,TWITCH_API_OPERATION_SUBSCRIBE);
			emit RateLimitHit();
			return;
		}
		emit SubscriptionFailed(type);
	},{},{
		{"Authorization","Bearer "_ba.append(static_cast<QByteArray>(security.ServerToken()))},
		{"Client-ID",security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON}
	},QJsonDocument(QJsonObject({
		{u"type"_s,type},
		{u"version"_s,"1"},
		{
			u"condition"_s,
			QJsonObject({{type == SUBSCRIPTION_TYPE_RAID ? u"to_broadcaster_user_id"_s : u"broadcaster_user_id"_s,security.AdministratorID()}})
		},
		{
			u"transport"_s,
			QJsonObject({
				{u"method"_s,"webhook"},
				{u"callback"_s,callbackURL.toString()},
				{u"secret"_s,secret}
			})
		}
	})).toJson(QJsonDocument::Compact));
}

void EventSub::NextSubscription()
{
	if (!defaultTypes.empty())
	{
		defaultTypes.pop();
		if (!defaultTypes.empty()) Subscribe(defaultTypes.front());
	}
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

