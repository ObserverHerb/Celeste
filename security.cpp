#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include "security.h"
#include "entities.h"
#include "network.h"

const char *QUERY_PARAMETER_CLIENT_ID="client_id";
const char *QUERY_PARAMETER_CLIENT_SECRET="client_secret";
const char *QUERY_PARAMETER_GRANT_TYPE="grant_type";
const char *QUERY_PARAMETER_REDIRECT_URI="redirect_uri";
const char *QUERY_PARAMETER_SESSION_ID="state";
const char *JSON_KEY_ACCESS_TOKEN="access";
const char *JSON_KEY_REFRESH_TOKEN="refresh";
const char *JSON_KEY_EXPIRY="expires_in";
const char *JSON_KEY_SCOPES="scopes";
const char *TWITCH_API_ENDPOINT_VALIDATE="https://id.twitch.tv/oauth2/validate";
const char *SETTINGS_CATEGORY_REWIRE="Rewire";
const char *OPERATION_LISTEN="listen to server";
const char *OPERATION_AUTHENTICATE="authenticate";
const char *ERROR_UKNOWN_REPONSE="Failed to understand response from server";
const char *ERROR_MISSING_TOKEN="Tokens missing in response from server";

const QStringList Security::SCOPES={
	"analytics:read:extensions",
	"analytics:read:games",
	"bits:read",
	"channel:edit:commercial",
	"channel:manage:broadcast",
	"channel:manage:extensions",
	"channel:manage:polls",
	"channel:manage:predictions",
	"channel:manage:redemptions",
	"channel:manage:schedule",
	"channel:manage:videos",
	"channel:moderate",
	"channel:read:editors",
	"channel:read:goals",
	"channel:read:hype_train",
	"channel:read:polls",
	"channel:read:predictions",
	"channel:read:redemptions",
	"channel:read:stream_key",
	"channel:read:subscriptions",
	"chat:edit",
	"clips:edit",
	"moderation:read",
	"moderator:manage:banned_users",
	"moderator:read:blocked_terms",
	"moderator:manage:blocked_terms",
	"moderator:manage:automod",
	"moderator:read:automod_settings",
	"moderator:manage:automod_settings",
	"moderator:read:chat_settings",
	"moderator:manage:chat_settings",
	"moderator:manage:shoutouts",
	"user:edit",
	"user:edit:follows",
	"user:manage:blocked_users",
	"user:read:blocked_users",
	"user:read:broadcast",
	"user:read:email",
	"user:read:follows",
	"user:read:subscriptions",
	"whispers:edit",
	"whispers:read"
};

Security::Security() : settingAdministrator("Administrator"),
	settingClientID("ClientID","vtoskvydu1rb5oyemum3et0nbiazvb"),
	settingOAuthToken("Token"),
	settingRefreshToken("RefreshToken"),
	settingCallbackURL("CallbackURL","https://twitch.engineeringdeck.com/celeste.php"),
	settingScope("Permissions","chat:read bits:read channel:read:redemptions channel:manage:broadcast channel:manage:redemptions channel:read:subscriptions moderator:manage:chat_settings moderator:read:chatters moderator:manage:shoutouts channel:read:hype_train channel:read:ads"),
	settingRewireSession("Session"),
	settingRewireHost(SETTINGS_CATEGORY_REWIRE,"Host","twitch.engineeringdeck.com"),
	settingRewirePort(SETTINGS_CATEGORY_REWIRE,"Port","1883"),
	rewire(nullptr),
	rewireChannel(nullptr),
	tokensInitialized(false),
	authorizing(false)
{
	if (!tokenValidationTimer.isActive()) tokenValidationTimer.setInterval(3600000); // 1 hour
	tokenValidationTimer.connect(&tokenValidationTimer,&QTimer::timeout,this,&Security::AuthorizeUser,Qt::QueuedConnection);
}

void Security::Listen()
{
	// drop existing connection (if there is one) so we don't stack them
	if (rewire)
	{
		rewire->disconnectFromHost();
		rewire->deleteLater();
	}

	// connect to the mqtt broker
	rewire=new QMqttClient(this);
	rewire->setHostname(settingRewireHost);
	rewire->setPort(settingRewirePort);

	connect(rewire,&QMqttClient::connected,this,&Security::RewireConnected);
	connect(rewire,&QMqttClient::errorChanged,this,&Security::RewireError);
	connect(rewire,&QMqttClient::disconnected,this,[this]() {
		emit Print("Disconnected",OPERATION_LISTEN);
	});

	if (!settingRewireSession) // generate a session ID if we don't have one already
	{
		QString sessionID=QUuid::createUuid().toString(QUuid::WithoutBraces);
		settingRewireSession.Set(sessionID);
		emit Print(QString("Created session ID: ")+StringConvert::SafeDump(sessionID),OPERATION_LISTEN);
	}
	rewire->setClientId(settingRewireSession);
	rewire->connectToHost();
}

void Security::RewireConnected()
{
	emit Print("Connected to server",OPERATION_LISTEN);

	// drop existing subscription (if there is one) so we don't stack them
	if (rewireChannel) rewireChannel->deleteLater();

	rewireChannel=rewire->subscribe({u"twitch/"_s+static_cast<QString>(settingRewireSession)});
	if (!rewireChannel)
	{
		emit Print("Failed to subscribe to credentials channel",OPERATION_LISTEN);
		rewire->disconnect();
		emit Disconnected();
		return;
	}

	connect(rewireChannel,&QMqttSubscription::messageReceived,this,&Security::RewireMessage);

	emit Listening();
	ValidateTokenWithRewire(); // FIXME: should listening and validating be tied together in here?
}

void Security::RewireError(QMqttClient::ClientError error)
{
	switch (error)
	{
	case QMqttClient::NoError:
		return;
	case QMqttClient::InvalidProtocolVersion:
		emit Print("The broker does not accept a connection using the specified protocol version.",OPERATION_LISTEN);
		break;
	case QMqttClient::IdRejected:
		emit Print("The client ID is malformed.",OPERATION_LISTEN);
		break;
	case QMqttClient::ServerUnavailable:
		emit Print("The service is unavailable on the broker side.",OPERATION_LISTEN);
		break;
	case QMqttClient::BadUsernameOrPassword:
		emit Print("The username or password is malformed.",OPERATION_LISTEN);
		break;
	case QMqttClient::NotAuthorized:
		emit Print("Not authorized to connect.",OPERATION_LISTEN);
		break;
	case QMqttClient::TransportInvalid:
		emit Print("The underlying transport caused an error.",OPERATION_LISTEN);
		break;
	case QMqttClient::ProtocolViolation:
		emit Print("Protocol violation",OPERATION_LISTEN);
		break;
	case QMqttClient::UnknownError:
		emit Print("An unknown error occurred.",OPERATION_LISTEN);
		break;
	case QMqttClient::Mqtt5SpecificError:
		emit Print("Protocol level 5 error.",OPERATION_LISTEN);
		break;
	}
	rewire->disconnect();
	emit Disconnected();
}

void Security::RewireMessage(QMqttMessage message)
{
	emit Print(QString("Message received: ")+StringConvert::SafeDump(message.payload()),OPERATION_LISTEN);

	const JSON::ParseResult parsedJSON=JSON::Parse(message.payload());
	if (!parsedJSON)
	{
		emit Print(ERROR_UKNOWN_REPONSE,OPERATION_LISTEN);
		emit TokenRequestFailed();
		return;
	}
	const QJsonObject object=parsedJSON().object();
	auto jsonFieldAccessToken=object.find(JSON_KEY_ACCESS_TOKEN);
	auto jsonFieldRefreshToken=object.find(JSON_KEY_REFRESH_TOKEN);
	if (jsonFieldAccessToken == object.end() || jsonFieldRefreshToken == object.end())
	{
		emit Print(ERROR_MISSING_TOKEN,OPERATION_LISTEN);
		emit TokenRequestFailed();
		return;
	}

	settingOAuthToken.Set(jsonFieldAccessToken->toString());
	settingRefreshToken.Set(jsonFieldRefreshToken->toString());

	authorizing=false;
	ObtainAdministratorProfile();
	tokenValidationTimer.start();
}

void Security::ValidateTokenWithRewire()
{
	// ask the server for the most recent auth and refresh tokens
	Network::Request::Send(settingCallbackURL,Network::Method::GET,[this](QNetworkReply *reply) {
		if (reply->error() || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 400) // I report back a 400 if the tokens can't be retrieved (ex. they're missing from the database)
		{
			emit Print("Tokens could not be retrieved from server",OPERATION_AUTHENTICATE);
			AuthorizeUser();
			return;
		}

		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			emit Print(ERROR_UKNOWN_REPONSE,OPERATION_AUTHENTICATE);
			AuthorizeUser();
			return;
		}
		const QJsonObject jsonObject=parsedJSON().object();
		auto jsonFieldAccessToken=jsonObject.find(JSON_KEY_ACCESS_TOKEN);
		auto jsonFieldRefreshToken=jsonObject.find(JSON_KEY_REFRESH_TOKEN);
		if (jsonFieldAccessToken == jsonObject.end() || jsonFieldRefreshToken == jsonObject.end())
		{
			emit Print(ERROR_MISSING_TOKEN,OPERATION_LISTEN);
			emit TokenRequestFailed();
			return;
		}

		settingOAuthToken.Set(jsonFieldAccessToken->toString());
		settingRefreshToken.Set(jsonFieldRefreshToken->toString());
		ValidateTokenWithTwitch();
	},{
		{"session",settingRewireSession}
	},{});
}

void Security::ValidateTokenWithTwitch()
{
	// tokens received from the server are valid, now check with Twitch to see if they think they're valid (required per https://dev.twitch.tv/docs/authentication/validate-tokens)
	Network::Request::Send({TWITCH_API_ENDPOINT_VALIDATE},Network::Method::GET,[this](QNetworkReply *reply) {
		if (reply->error() || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401) // 401 = access token is invalid
		{
			emit Print("Twitch is not recognizing access token",OPERATION_AUTHENTICATE);
			AuthorizeUser();
			return;
		}

		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			emit Print(ERROR_UKNOWN_REPONSE,OPERATION_AUTHENTICATE);
			AuthorizeUser();
			return;
		}

		const QJsonObject jsonObject=parsedJSON().object();
		auto jsonFieldExpiry=jsonObject.find(JSON_KEY_EXPIRY);
		auto jsonFieldScopes=jsonObject.find(JSON_KEY_SCOPES);
		if (jsonFieldExpiry == jsonObject.end() || jsonFieldScopes == jsonObject.end())
		{
			emit Print("Twitch didn't include expiration in reply.",OPERATION_AUTHENTICATE);
			AuthorizeUser();
			return;
		}

		std::chrono::hours hoursRemaining=std::chrono::duration_cast<std::chrono::hours>(static_cast<std::chrono::seconds>(jsonFieldExpiry->toInt()));
		QStringList tokenScopes=jsonFieldScopes->toVariant().toStringList();
		QStringList requestedScopes=static_cast<QString>(settingScope).split(" ");
		QSet<QString> scopesNeeded(requestedScopes.begin(),requestedScopes.end());
		scopesNeeded.subtract(QSet<QString>{tokenScopes.begin(),tokenScopes.end()});

		if (hoursRemaining > std::chrono::hours(1) && scopesNeeded.isEmpty()) // FIXME: I think this is the source of my "spurious reauth" problem.
		{
			ObtainAdministratorProfile();
			return;
		}
		AuthorizeUser(); // FIXME: See above; need a way to tell the rewire server please ask for refresh token NOW. (instead of fully restarting auth process)
	},{},{
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_FORM},
		{NETWORK_HEADER_AUTHORIZATION,"OAuth "_ba+static_cast<QByteArray>(settingOAuthToken)}
	});
}

void Security::AuthorizeUser()
{
	// don't let multiple failures from multiple sources trigger multiple OAuth attempts
	if (authorizing) return;
	authorizing=true;

	// trigger the OAuth process with Twitch
	QUrl request("https://id.twitch.tv/oauth2/authorize");
	request.setQuery(QUrlQuery({
		{QUERY_PARAMETER_CLIENT_ID,settingClientID},
		{QUERY_PARAMETER_REDIRECT_URI,settingCallbackURL},
		{"response_type","code"}, // code must be used instead of token because implicit flow reports token via the URL fragment, which can't be read by server-side code (only client-side code like Javascript)
		{QUERY_PARAMETER_SCOPE,settingScope},
		{QUERY_PARAMETER_SESSION_ID,settingRewireSession}
	}));
	QDesktopServices::openUrl(request);
}

void Security::ObtainAdministratorProfile()
{
	Viewer::Remote *profile=new Viewer::Remote(*this,settingAdministrator);
	connect(profile,&Viewer::Remote::Recognized,profile,[this](Viewer::Local profile) {
		administratorID=profile.ID();
		Initialize();
	});
	connect(profile,&Viewer::Remote::Unrecognized,this,[this]() {
		AuthorizeUser();
	});
}

const QString& Security::AdministratorID() const
{
	if (administratorID.isNull()) throw std::logic_error("Administrator ID used before it was obtained from Twitch");
	return administratorID;
}

void Security::Initialize()
{
	if (!tokensInitialized)
	{
		tokensInitialized=true;
		emit Initialized();
	}
}

QByteArray Security::Bearer(const QByteArray &token)
{
	return "Bearer "_ba.append(token);
}
