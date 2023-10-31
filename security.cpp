#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include "security.h"
#include "entities.h"

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
	settingClientID("ClientID"),
	settingClientSecret("ClientSecret"),
	settingOAuthToken("Token"),
	settingRefreshToken("RefreshToken"),
	settingServerToken("ServerToken"),
	settingCallbackURL("CallbackURL"),
	settingScope("Permissions"),
	settingRewireSession("Session"),
	settingRewireHost(SETTINGS_CATEGORY_REWIRE,"Host","twitch.hlmjr.com"),
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
	if (rewire)
	{
		rewire->disconnectFromHost();
		rewire->deleteLater();
	}

	rewire=new QMqttClient(this);
	rewire->setHostname(settingRewireHost);
	rewire->setPort(settingRewirePort);

	connect(rewire,&QMqttClient::connected,this,&Security::RewireConnected);
	connect(rewire,&QMqttClient::errorChanged,this,&Security::RewireError);

	rewire->connectToHost();
}

void Security::RewireConnected()
{
	if (rewireChannel) rewireChannel->deleteLater();
	if (!settingRewireSession) settingRewireSession.Set(QUuid::createUuid().toString(QUuid::WithoutBraces));
	rewire->setClientId(settingRewireSession);

	rewireChannel=rewire->subscribe({u"twitch/"_s+static_cast<QString>(settingRewireSession)});
	if (!rewireChannel)
	{
		rewire->disconnect();
		emit Disconnected();
		return;
	}

	connect(rewireChannel,&QMqttSubscription::messageReceived,this,&Security::RewireMessage);

	emit Listening();
	ValidateToken();
}

void Security::RewireError(QMqttClient::ClientError error)
{
	if (error != QMqttClient::NoError)
	{
		rewire->disconnect();
		emit Disconnected();
	}
}

void Security::RewireMessage(QMqttMessage message)
{
	const JSON::ParseResult parsedJSON=JSON::Parse(message.payload());
	if (!parsedJSON)
	{
		emit TokenRequestFailed();
		return;
	}
	const QJsonObject object=parsedJSON().object();
	auto jsonFieldAccessToken=object.find(JSON_KEY_ACCESS_TOKEN);
	auto jsonFieldRefreshToken=object.find(JSON_KEY_REFRESH_TOKEN);
	if (jsonFieldAccessToken == object.end() || jsonFieldRefreshToken == object.end())
	{
		emit TokenRequestFailed();
		return;
	}

	settingOAuthToken.Set(jsonFieldAccessToken->toString());
	settingRefreshToken.Set(jsonFieldRefreshToken->toString());

	authorizing=false;
	ObtainAdministratorProfile();
	tokenValidationTimer.start();
}

void Security::ValidateToken()
{
	Network::Request(settingCallbackURL,Network::Method::GET,[this](QNetworkReply *reply) {
		if (!reply->error() && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 400)
		{
			const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
			if (parsedJSON)
			{
				const QJsonObject jsonObject=parsedJSON().object();
				auto jsonFieldAccessToken=jsonObject.find(JSON_KEY_ACCESS_TOKEN);
				auto jsonFieldRefreshToken=jsonObject.find(JSON_KEY_REFRESH_TOKEN);
				if (jsonFieldAccessToken != jsonObject.end() || jsonFieldRefreshToken != jsonObject.end())
				{
					settingOAuthToken.Set(jsonFieldAccessToken->toString());
					settingRefreshToken.Set(jsonFieldRefreshToken->toString());

					Network::Request({TWITCH_API_ENDPOINT_VALIDATE},Network::Method::GET,[this](QNetworkReply *reply) {
						if (!reply->error() && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 401)
						{
							const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
							if (parsedJSON)
							{
								const QJsonObject jsonObject=parsedJSON().object();
								auto jsonFieldExpiry=jsonObject.find(JSON_KEY_EXPIRY);
								auto jsonFieldScopes=jsonObject.find(JSON_KEY_SCOPES);
								if (jsonFieldExpiry != jsonObject.end() || jsonFieldScopes != jsonObject.end())
								{
									std::chrono::hours hoursRemaining=std::chrono::duration_cast<std::chrono::hours>(static_cast<std::chrono::seconds>(jsonFieldExpiry->toInt()));
									QStringList tokenScopes=jsonFieldScopes->toVariant().toStringList();
									QStringList requestedScopes=static_cast<QString>(settingScope).split(" ");
									QSet<QString> scopesNeeded(requestedScopes.begin(),requestedScopes.end());
									scopesNeeded.subtract(QSet<QString>{tokenScopes.begin(),tokenScopes.end()});

									if (hoursRemaining > std::chrono::hours(1) && scopesNeeded.isEmpty())
									{
										ObtainAdministratorProfile();
										return;
									}
								}
							}
						}
						AuthorizeUser();
					},{},{
						{Network::CONTENT_TYPE,Network::CONTENT_TYPE_FORM},
						{NETWORK_HEADER_AUTHORIZATION,"OAuth "_ba+settingOAuthToken}
					});

					return;
				}
			}
		}
		AuthorizeUser();
	},{
		{"session",settingRewireSession}
	},{});
}

void Security::AuthorizeUser()
{
	if (!authorizing)
	{
		authorizing=true;
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
}

void Security::AuthorizeServer()
{
	Network::Request({u"https://id.twitch.tv/oauth2/token"_s},Network::Method::POST,[this](QNetworkReply *reply) {
		// I'm not worried about handling errors here because any problems will trigger a failure in the EventSub
		// subscription again, which will ultimately bring us back through here via Channel::Connect() if the user wishes
		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON) return;

		const QJsonObject jsonObject=parsedJSON().object();
		auto jsonFieldAccessToken=jsonObject.find(JSON_KEY_ACCESS_TOKEN);
		if (jsonFieldAccessToken == jsonObject.end()) return;

		settingServerToken.Set(jsonFieldAccessToken->toString());
	},{
		{QUERY_PARAMETER_CLIENT_ID,settingClientID},
		{QUERY_PARAMETER_CLIENT_SECRET,settingClientSecret},
		{QUERY_PARAMETER_GRANT_TYPE,u"client_credentials"_s}
	},{
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_FORM}
	});
}

void Security::ObtainAdministratorProfile()
{
	Viewer::Remote *profile=new Viewer::Remote(*this,settingAdministrator);
	connect(profile,&Viewer::Remote::Recognized,profile,[this](Viewer::Local profile) {
		administratorID=profile.ID();
		emit Initialize();
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
