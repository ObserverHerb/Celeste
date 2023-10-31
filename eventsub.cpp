#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStringBuilder>
#include <QUuid>
#include "globals.h"
#include "eventsub.h"

const char *TWITCH_API_ENDPOINT_EVENTSUB="https://api.twitch.tv/helix/eventsub/subscriptions";
const char *TWITCH_API_ENDPOINT_EVENTSUB_SUBSCRIPTIONS="https://api.twitch.tv/helix/eventsub/subscriptions";

const char *JSON_KEY_METADATA="metadata";
const char *JSON_KEY_METADATA_TYPE="message_type";
const char *JSON_KEY_PAYLOAD="payload";
const char *JSON_KEY_PAYLOAD_SESSION="session";
const char *JSON_KEY_PAYLOAD_SESSION_ID="id";
const char *JSON_KEY_PAYLOAD_SESSION_KEEPALIVE_TIMEOUT="keepalive_timeout_seconds";
const char *JSON_KEY_PAYLOAD_SUBSCRIPTION="subscription";
const char *JSON_KEY_PAYLOAD_SUBSCRIPTION_TYPE="type";
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

const char *MESSAGE_TYPE_WELCOME="session_welcome";
const char *MESSAGE_TYPE_KEEPALIVE="session_keepalive";
const char *MESSAGE_TYPE_NOTIFICATION="notification";

const char *EventSub::SETTINGS_CATEGORY_EVENTS="Events";

enum class TwitchCloseCode
{
	INTERNAL_SERVER_ERROR=4000,
	CLIENT_SENT_INBOUND_TRAFFIC=4001,
	CLIENT_FAILED_PING_PONG=4002,
	CONNECTION_UNUSED=4003,
	RECONNECT_GRACE_TIME_EXPIRED=4004,
	NETWORK_TIMEOUT=4005,
	NETWORK_ERROR=4006,
	INVALID_RECONNECT=4007
};

EventSub::EventSub(Security &security,QObject *parent) : QObject(parent),
	security(security),
	settingURL(SETTINGS_CATEGORY_EVENTS,"WebsocketURL","wss://eventsub.wss.twitch.tv/ws")
{
	messageTypes.insert({MESSAGE_TYPE_WELCOME,MessageType::WELCOME});
	messageTypes.insert({MESSAGE_TYPE_NOTIFICATION,MessageType::NOTIFICATION});
	messageTypes.insert({MESSAGE_TYPE_KEEPALIVE,MessageType::KEEPALIVE});

	subscriptionTypes.insert({SUBSCRIPTION_TYPE_FOLLOW,SubscriptionType::CHANNEL_FOLLOW});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_REDEMPTION,SubscriptionType::CHANNEL_REDEMPTION});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_CHEER,SubscriptionType::CHANNEL_CHEER});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_RAID,SubscriptionType::CHANNEL_RAID});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_SUBSCRIPTION,SubscriptionType::CHANNEL_SUBSCRIPTION});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_RESUBSCRIPTION,SubscriptionType::CHANNEL_SUBSCRIPTION});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_HYPE_TRAIN_START,SubscriptionType::CHANNEL_HYPE_TRAIN});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_HYPE_TRAIN_PROGRESS,SubscriptionType::CHANNEL_HYPE_TRAIN});
	subscriptionTypes.insert({SUBSCRIPTION_TYPE_HYPE_TRAIN_END,SubscriptionType::CHANNEL_HYPE_TRAIN});

	connect(&keepalive,&QTimer::timeout,this,&EventSub::Dead);

	connect(&socket,&QWebSocket::disconnected,this,&EventSub::SocketClosed);
	connect(&socket,&QWebSocket::textMessageReceived,this,&EventSub::ParseMessage);
	Connect();
}

void EventSub::Connect()
{
	socket.open(settingURL);
}

void EventSub::SocketClosed()
{
	static const char *TWITCH_API_OPERATION_SOCKET_CLOSED="socket closed";

	switch (socket.closeCode())
	{
	case QWebSocketProtocol::CloseCodeNormal:
		break;
	case static_cast<int>(TwitchCloseCode::INTERNAL_SERVER_ERROR):
		emit Print("Internal server error on Twitch's end",TWITCH_API_OPERATION_SOCKET_CLOSED);
		break;
	case static_cast<int>(TwitchCloseCode::CLIENT_SENT_INBOUND_TRAFFIC):
		emit Print("Sending outgoing messages to the server is prohibited with the exception of pong messages.",TWITCH_API_OPERATION_SOCKET_CLOSED);
		break;
	case static_cast<int>(TwitchCloseCode::CLIENT_FAILED_PING_PONG):
		emit Print("You must respond to ping messages with a pong message.",TWITCH_API_OPERATION_SOCKET_CLOSED);
		break;
	case static_cast<int>(TwitchCloseCode::CONNECTION_UNUSED):
		emit Print("You must create a subscription within 10 seconds.",TWITCH_API_OPERATION_SOCKET_CLOSED);
		break;
	case static_cast<int>(TwitchCloseCode::RECONNECT_GRACE_TIME_EXPIRED):
		emit Print("When you receive a session_reconnect message, you have 30 seconds to reconnect to the server and close the old connection.",TWITCH_API_OPERATION_SOCKET_CLOSED);
		break;
	case static_cast<int>(TwitchCloseCode::NETWORK_TIMEOUT):
		emit Print("Transient network timeout",TWITCH_API_OPERATION_SOCKET_CLOSED);
		Connect();
		return;
	case static_cast<int>(TwitchCloseCode::NETWORK_ERROR):
		emit Print("Transient network error",TWITCH_API_OPERATION_SOCKET_CLOSED);
		break;
	case static_cast<int>(TwitchCloseCode::INVALID_RECONNECT):
		emit Print("The reconnect URL is invalid",TWITCH_API_OPERATION_SOCKET_CLOSED);
		break;
	}

	emit Disconnected();
}

void EventSub::Dead()
{
	socket.close(static_cast<QWebSocketProtocol::CloseCode>(TwitchCloseCode::NETWORK_TIMEOUT));
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
		{NETWORK_HEADER_AUTHORIZATION,security.Bearer(security.OAuthToken())},
		{NETWORK_HEADER_CLIENT_ID,security.ClientID()},
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_JSON}
	},QJsonDocument(QJsonObject({
		{u"type"_s,type},
		{u"version"_s,"1"}, // NOTE: Twitch has already started using this, so going to need to know max versions for subscriptions eventually
		{
			u"condition"_s,
			QJsonObject({{type == SUBSCRIPTION_TYPE_RAID ? u"to_broadcaster_user_id"_s : u"broadcaster_user_id"_s,security.AdministratorID()}})
		},
		{
			u"transport"_s,
			QJsonObject({
				{u"method"_s,"websocket"},
				{u"session_id"_s,sessionID}
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

void EventSub::ParseMessage(QString message)
{
	static const char *OPERATION_PARSE_MESSAGE="parse message";

	const JSON::ParseResult parsedJSON=JSON::Parse(StringConvert::ByteArray(message.trimmed()));

	if (!parsedJSON)
	{
		Print(u"Invalid JSON: %1"_s.arg(parsedJSON.error),OPERATION_PARSE_MESSAGE);
		return;
	}

	QJsonObject messageObject=parsedJSON().object();
	auto metadata=messageObject.find(JSON_KEY_METADATA);
	auto payload=messageObject.find(JSON_KEY_PAYLOAD);
	if (metadata == messageObject.end() || payload == messageObject.end())
	{
		Print(u"Malformatted message"_s,OPERATION_PARSE_MESSAGE);
		return;
	}

	QJsonObject metadataObject=metadata->toObject();
	auto type=metadataObject.find(JSON_KEY_METADATA_TYPE);
	if (type == metadataObject.end())
	{
		Print(u"Missing message type"_s,OPERATION_PARSE_MESSAGE);
		return;
	}

	auto messageType=messageTypes.find(type->toString());
	if (messageType == messageTypes.end())
	{
		Print(u"Unknown message type (%1)"_s.arg(type->toString()),OPERATION_PARSE_MESSAGE);
		return;
	}
	switch (messageType->second)
	{
	case MessageType::WELCOME:
		ParseWelcome(payload->toObject());
		break;
	case MessageType::KEEPALIVE:
		keepalive.start();
		break;
	case MessageType::NOTIFICATION:
		ParseNotification(payload->toObject());
		break;
	default:
		throw std::logic_error("Websocket message type recognized but unimplemented");
	}
}

void EventSub::ParseWelcome(QJsonObject payload)
{
	static const char *OPERATION_PARSE_WELCOME="parse welcome";

	auto session=payload.find(JSON_KEY_PAYLOAD_SESSION);
	if (session == payload.end())
	{
		emit Print("Ignoring welcome message with no session data",OPERATION_PARSE_WELCOME);
		return;
	}

	QJsonObject sessionObject=session->toObject();
	auto candidateID=sessionObject.find(JSON_KEY_PAYLOAD_SESSION_ID);
	if (candidateID == sessionObject.end())
	{
		emit Print("Unable to create EventSub connection because Twitch did not provide a session ID",OPERATION_PARSE_WELCOME);
		return;
	}

	sessionID=candidateID->toString();
	emit Connected();

	auto keepaliveCandidate=sessionObject.find(JSON_KEY_PAYLOAD_SESSION_KEEPALIVE_TIMEOUT);
	if (keepaliveCandidate != sessionObject.end())
	{
		keepalive.setInterval(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(keepaliveCandidate->toInt()*2)));
		keepalive.start();
	}
}

void EventSub::ParseNotification(QJsonObject payload)
{
	static const char *OPERATION_PARSE_NOTIFICATION="parse notification";

	auto subscription=payload.find(JSON_KEY_PAYLOAD_SUBSCRIPTION);
	if (subscription == payload.end())
	{
		emit Print("Ignoring notification payload with no subscription data",OPERATION_PARSE_NOTIFICATION);
		return;
	}

	QJsonObject subscriptionObject=subscription->toObject();
	SubscriptionType subscriptionType=SubscriptionType::UNKNOWN;
	if (auto subscriptionTypeCandidate=subscriptionObject.find(JSON_KEY_PAYLOAD_SUBSCRIPTION_TYPE); subscriptionTypeCandidate != subscriptionObject.end())
	{
		QString key=subscriptionTypeCandidate->toString();
		if (auto subscriptionTypeCandidate=subscriptionTypes.find(key); subscriptionTypeCandidate != subscriptionTypes.end())
		{
			subscriptionType=subscriptionTypeCandidate->second;
		}
	}
	if (subscriptionType == SubscriptionType::UNKNOWN) return;

	auto event=payload.find(JSON_KEY_EVENT);
	if (event == payload.end())
	{
		Print("Event field missing from notification");
		return;
	}

	QJsonObject eventObject=event->toObject();
	switch (subscriptionType)
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
		emit ChannelSubscription(eventObject.value(JSON_KEY_EVENT_USER_LOGIN).toString(),eventObject.value(JSON_KEY_EVENT_USER_NAME).toString());
		break;
	case SubscriptionType::CHANNEL_HYPE_TRAIN:
		if (double goal=eventObject.value(JSON_KEY_EVENT_HYPE_TRAIN_TOTAL).toDouble(); goal > 0) emit HypeTrain(eventObject.value(JSON_KEY_EVENT_HYPE_TRAIN_LEVEL).toInt(),eventObject.value(JSON_KEY_EVENT_HYPE_TRAIN_PROGRESS).toDouble()/goal);
		break;
	default:
		throw std::logic_error("Subscription type recognized but unimplemented");
	}
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
