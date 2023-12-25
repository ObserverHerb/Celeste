#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringBuilder>
#include <QUuid>
#include "globals.h"
#include "eventsub.h"

const char *HEADER_SUBSCRIPTION_TYPE="Twitch-Eventsub-Subscription-Type";

const char *TWITCH_API_ENDPOINT_EVENTSUB="https://api.twitch.tv/helix/eventsub/subscriptions";
const char *TWITCH_API_ENDPOINT_EVENTSUB_SUBSCRIPTIONS="https://api.twitch.tv/helix/eventsub/subscriptions";

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
const char *JSON_KEY_EVENT_HYPE_TRAIN_LEVEL="level";
const char *JSON_KEY_EVENT_HYPE_TRAIN_PROGRESS="progress";
const char *JSON_KEY_EVENT_HYPE_TRAIN_TOTAL="goal";
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
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_HYPE_TRAIN_START,SubscriptionType::CHANNEL_HYPE_TRAIN});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_HYPE_TRAIN_PROGRESS,SubscriptionType::CHANNEL_HYPE_TRAIN});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_HYPE_TRAIN_END,SubscriptionType::CHANNEL_HYPE_TRAIN});
}

void EventSub::Subscribe()
{
	defaultTypes.push(SUBSCRIPTION_TYPE_REDEMPTION);
	defaultTypes.push(SUBSCRIPTION_TYPE_RAID);
	defaultTypes.push(SUBSCRIPTION_TYPE_SUBSCRIPTION);
	defaultTypes.push(SUBSCRIPTION_TYPE_RESUBSCRIPTION);
	defaultTypes.push(SUBSCRIPTION_TYPE_CHEER);
	defaultTypes.push(SUBSCRIPTION_TYPE_HYPE_TRAIN_START);
	defaultTypes.push(SUBSCRIPTION_TYPE_HYPE_TRAIN_PROGRESS);
	defaultTypes.push(SUBSCRIPTION_TYPE_HYPE_TRAIN_END);
	Subscribe(defaultTypes.front());
}

void EventSub::Subscribe(const QString &type)
{
	static const char *TWITCH_API_OPERATION_SUBSCRIBE="subscribe to event";

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
		emit EventSubscriptionFailed(type);
	},{},{
		{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.ServerToken())},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
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
	static const char *OPERATION_PARSE_REQUEST="parse request";
	auto headerSubscriptionType=headers.find(HEADER_SUBSCRIPTION_TYPE);
	if (headerSubscriptionType == headers.end())
	{
		emit Print("Ignoring request with no subscription type",OPERATION_PARSE_REQUEST);
		return;
	}

	auto subscriptionType=subscriptionTypes.find(headerSubscriptionType->second);
	if (subscriptionType == subscriptionTypes.end()) return;

	const JSON::ParseResult parsedJSON=JSON::Parse(StringConvert::ByteArray(content.trimmed()));
	if (!parsedJSON)
	{
		Print(QString("Invalid JSON: %1").arg(parsedJSON.error),OPERATION_PARSE_REQUEST);
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
	JSON::SignalPayload *payload=new JSON::SignalPayload(event->toObject());
	if (std::optional<QString> prompt=ExtractPrompt(subscriptionType->second,eventObject); prompt) payload->context=*prompt;
	const QString name=eventObject.value(JSON_KEY_EVENT_USER_NAME).toString();
	const QString login=eventObject.value(JSON_KEY_EVENT_USER_LOGIN).toString();
	connect(payload,&JSON::SignalPayload::Deliver,this,[subscriptionType=subscriptionType->second,name,login,payload,this](const QJsonObject &eventObject) {
		switch (subscriptionType)
		{
		case SubscriptionType::CHANNEL_FOLLOW:
			emit Follow();
			break;
		case SubscriptionType::CHANNEL_REDEMPTION:
			emit Redemption(login,name,eventObject.value(JSON_KEY_EVENT_REWARD).toObject().value(JSON_KEY_EVENT_REWARD_TITLE).toString(),payload->context.toString());
			break;
		case SubscriptionType::CHANNEL_CHEER:
			emit Cheer(name,eventObject.value(JSON_KEY_EVENT_CHEER_AMOUNT).toVariant().toUInt(),eventObject.value(JSON_KEY_EVENT_MESSAGE).toString().section(" ",1));
			break;
		case SubscriptionType::CHANNEL_SUBSCRIPTION:
			emit ChannelSubscription(eventObject.value(JSON_KEY_EVENT_USER_LOGIN).toString(),eventObject.value(JSON_KEY_EVENT_USER_NAME).toString());
			break;
		case SubscriptionType::CHANNEL_HYPE_TRAIN:
			double goal=eventObject.value(JSON_KEY_EVENT_HYPE_TRAIN_TOTAL).toDouble();
			if (goal > 0)
				emit HypeTrain(eventObject.value(JSON_KEY_EVENT_HYPE_TRAIN_LEVEL).toInt(),eventObject.value(JSON_KEY_EVENT_HYPE_TRAIN_PROGRESS).toDouble()/goal);
			break;
		}
	});
	emit ParseCommand(payload,name,login);

	emit Response(socketID);
}

void EventSub::RequestEventSubscriptionList()
{
	static const char *TWITCH_API_OPERATION_SUBSCRIPTION_LIST="list subscriptions";

	Network::Request({TWITCH_API_ENDPOINT_EVENTSUB_SUBSCRIPTIONS},Network::Method::GET,[this](QNetworkReply *reply) {
		switch (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
		{
		case 400:
			emit Print(u"The subscription request was malformatted"_s,TWITCH_API_OPERATION_SUBSCRIPTION_LIST);
			break;
		case 401:
			emit Print(u"Invalid OAuth token or authorization header was malformatted"_s,TWITCH_API_OPERATION_SUBSCRIPTION_LIST);
			return;
		}

		const QByteArray data=reply->readAll();
		emit Print(StringConvert::Dump(data),TWITCH_API_OPERATION_SUBSCRIPTION_LIST);

		const JSON::ParseResult parsedJSON=JSON::Parse(StringConvert::ByteArray(data.trimmed()));
		if (!parsedJSON)
		{
			Print(QString("Invalid JSON: %1").arg(parsedJSON.error),TWITCH_API_OPERATION_SUBSCRIPTION_LIST);
			return;
		}

		QJsonObject jsonObject=parsedJSON().object();
		if (auto subscriptionList=jsonObject.find(JSON::Keys::DATA); subscriptionList != jsonObject.end())
		{
			for (const QJsonValue &eventSubscription : subscriptionList->toArray())
			{
				const QJsonObject entry=eventSubscription.toObject();
				const QString id=entry.value("id").toString();
				const QString type=entry.value("type").toString();
				const QDateTime date=entry.value("created_at").toVariant().toDateTime();
				const QString callbackURL=entry.value("transport").toObject().value("callback").toString();
				if (id.isEmpty() || type.isEmpty() || callbackURL.isEmpty() || !date.isValid()) continue;
				emit EventSubscription(id,type,date,callbackURL);
			}
		}
	},{},{
		{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.ServerToken())},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()}
	});
}

void EventSub::RemoveEventSubscription(const QString &id)
{
	static const char *TWITCH_API_OPERATION_SUBSCRIPTION_DELETE="delete subscription";

	Network::Request({TWITCH_API_ENDPOINT_EVENTSUB_SUBSCRIPTIONS},Network::Method::DELETE,[this,id](QNetworkReply *reply) {
		switch (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
		{
		case 204:
			emit Print(u"Removed event "_s.append(id),TWITCH_API_OPERATION_SUBSCRIPTION_DELETE);
			emit EventSubscriptionRemoved(id);
			break;
		case 400:
			emit Print(u"The subscription request was malformatted"_s,TWITCH_API_OPERATION_SUBSCRIPTION_DELETE);
			break;
		case 401:
			emit Print(u"Invalid OAuth token or authorization header was malformatted"_s,TWITCH_API_OPERATION_SUBSCRIPTION_DELETE);
			break;
		case 404:
			emit Print(u"Invalid subscription ID"_s,TWITCH_API_OPERATION_SUBSCRIPTION_DELETE);
			break;
		}
	},{
		{u"id"_s,id}
	},{
		{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.ServerToken())},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()}
	});
}

std::optional<QString> EventSub::ExtractPrompt(SubscriptionType type,const QJsonObject &event) const
{
	switch (type)
	{
	case SubscriptionType::CHANNEL_CHEER:
		return "!uptime "+event.value(JSON_KEY_EVENT_MESSAGE).toString().section(" ",1);
	default:
		return std::nullopt;
	}
}
