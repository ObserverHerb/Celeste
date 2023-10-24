#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include "security.h"
#include "entities.h"

const char *QUERY_PARAMETER_CLIENT_ID="client_id";
const char *QUERY_PARAMETER_CLIENT_SECRET="client_secret";
const char *QUERY_PARAMETER_GRANT_TYPE="grant_type";
const char *QUERY_PARAMETER_REDIRECT_URI="redirect_uri";
const char *JSON_KEY_ACCESS_TOKEN="access_token";
const char *JSON_KEY_REFRESH_TOKEN="refresh_token";
const char *JSON_KEY_EXPIRY="expires_in";
const char *TWITCH_API_ENDPOINT_TOKEN="https://id.twitch.tv/oauth2/token";

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

QTimer Security::tokenTimer;

Security::Security() : settingAdministrator("Administrator"),
	settingClientID("ClientID"),
	settingClientSecret("ClientSecret"),
	settingOAuthToken("Token"),
	settingRefreshToken("RefreshToken"),
	settingServerToken("ServerToken"),
	settingCallbackURL("CallbackURL"),
	settingScope("Permissions")
{
	if (!tokenTimer.isActive()) tokenTimer.setInterval(3600000); // 1 hour
	tokenTimer.connect(&tokenTimer,&QTimer::timeout,this,&Security::RefreshToken,Qt::UniqueConnection);
}

void Security::AuthorizeUser()
{
	QUrl request("https://id.twitch.tv/oauth2/authorize");
	request.setQuery(QUrlQuery({
		{QUERY_PARAMETER_CLIENT_ID,settingClientID},
		{QUERY_PARAMETER_REDIRECT_URI,settingCallbackURL},
		{"response_type","code"},
		{QUERY_PARAMETER_SCOPE,settingScope}
	 }));
	QDesktopServices::openUrl(request);
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

void Security::RequestToken(const QString &code,const QString &scopes)
{
	if (code.isEmpty() || scopes.isEmpty()) emit TokenRequestFailed();
	Network::Request({TWITCH_API_ENDPOINT_TOKEN},Network::Method::POST,[this](QNetworkReply *reply) {
		if (reply->error())
		{
			emit TokenRequestFailed();
			return;
		}

		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			emit TokenRequestFailed();
			return;
		}

		const QJsonObject jsonObject=parsedJSON().object();
		auto jsonFieldAccessToken=jsonObject.find(JSON_KEY_ACCESS_TOKEN);
		auto jsonFieldRefreshToken=jsonObject.find(JSON_KEY_REFRESH_TOKEN);
		if (jsonFieldAccessToken == jsonObject.end() || jsonFieldRefreshToken == jsonObject.end())
		{
			emit TokenRequestFailed();
			return;
		}
		settingOAuthToken.Set(jsonFieldAccessToken->toString());
		settingRefreshToken.Set(jsonFieldRefreshToken->toString());
		auto jsonObjectExpiry=jsonObject.find(JSON_KEY_EXPIRY);
		if (jsonObjectExpiry == jsonObject.end()) return;
		std::chrono::milliseconds margin=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(30));
		std::chrono::milliseconds expiry=std::chrono::duration_cast<std::chrono::milliseconds>(static_cast<std::chrono::seconds>(jsonObjectExpiry->toInt()))-margin;
		if (expiry.count() > 0) tokenTimer.setInterval(expiry.count());
	},{
		{QUERY_PARAMETER_CODE,code},
		{QUERY_PARAMETER_CLIENT_ID,settingClientID},
		{QUERY_PARAMETER_CLIENT_SECRET,settingClientSecret},
		{QUERY_PARAMETER_GRANT_TYPE,"authorization_code"},
		{QUERY_PARAMETER_REDIRECT_URI,settingCallbackURL}
	},{
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_FORM}
	});
}

void Security::RefreshToken()
{
	Network::Request({TWITCH_API_ENDPOINT_TOKEN},Network::Method::POST,[this](QNetworkReply *reply) {
		if (reply->error())
		{
			emit TokenRefreshFailed();
			return;
		}

		const JSON::ParseResult parsedJSON=JSON::Parse(reply->readAll());
		if (!parsedJSON)
		{
			emit TokenRefreshFailed();
			return;
		}
		const QJsonObject object=parsedJSON().object();
		auto jsonFieldAccessToken=object.find(JSON_KEY_ACCESS_TOKEN);
		auto jsonFieldRefreshToken=object.find(JSON_KEY_REFRESH_TOKEN);
		if (jsonFieldAccessToken == object.end() || jsonFieldRefreshToken == object.end())
		{
			emit TokenRefreshFailed();
			return;
		}
		settingOAuthToken.Set(jsonFieldAccessToken->toString());
		settingRefreshToken.Set(jsonFieldRefreshToken->toString());
	},{
		{QUERY_PARAMETER_CLIENT_ID,settingClientID},
		{QUERY_PARAMETER_CLIENT_SECRET,settingClientSecret},
		{QUERY_PARAMETER_GRANT_TYPE,"refresh_token"},
		{"refresh_token",settingRefreshToken}
	},{
		{Network::CONTENT_TYPE,Network::CONTENT_TYPE_FORM}
	});
}

void Security::ObtainAdministratorProfile()
{
	Viewer::Remote *profile=new Viewer::Remote(*this,settingAdministrator);
	connect(profile,&Viewer::Remote::Recognized,profile,[this](Viewer::Local profile) {
		administratorID=profile.ID();
		emit AdministratorProfileObtained();
	});

}

void Security::StartClocks()
{
	tokenTimer.start();
}

const QString& Security::AdministratorID() const
{
	if (administratorID.isNull()) throw std::logic_error("Administrator ID used before it was obtained from Twitch");
	return administratorID;
}

QByteArray Security::Bearer(const QByteArray &token)
{
	return "Bearer "_ba.append(token);
}
