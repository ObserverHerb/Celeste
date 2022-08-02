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
	"chat:edit","clips:edit",
	"moderation:read",
	"moderator:manage:banned_users",
	"moderator:read:blocked_terms",
	"moderator:manage:blocked_terms",
	"moderator:manage:automod",
	"moderator:read:automod_settings",
	"moderator:manage:automod_settings",
	"moderator:read:chat_settings",
	"moderator:manage:chat_settings",
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

void Security::RequestToken(const QString &code,const QString &scopes)
{
	if (code.isEmpty() || scopes.isEmpty()) emit TokenRequestFailed();
	Network::Request({TWITCH_API_ENDPOINT_TOKEN},Network::Method::POST,[this](QNetworkReply *reply) {
		if (reply->error())
		{
			emit TokenRequestFailed();
			return;
		}
		QJsonDocument json=QJsonDocument::fromJson(reply->readAll());
		if (json.isNull())
		{
			emit TokenRequestFailed();
			return;
		}
		if (!json.object().contains(JSON_KEY_ACCESS_TOKEN) || !json.object().contains(JSON_KEY_REFRESH_TOKEN))
		{
			emit TokenRequestFailed();
			return;
		}

		settingOAuthToken.Set(json.object().value(JSON_KEY_ACCESS_TOKEN).toString());
		settingRefreshToken.Set(json.object().value(JSON_KEY_REFRESH_TOKEN).toString());

		if (json.object().contains(JSON_KEY_EXPIRY))
		{
			std::chrono::milliseconds margin=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(30));
			std::chrono::milliseconds expiry=std::chrono::duration_cast<std::chrono::milliseconds>(static_cast<std::chrono::seconds>(json.object().value("expires_in").toInt()))-margin;
			if (expiry.count() > 0)	tokenTimer.setInterval(expiry.count());
		}
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
			emit TokenRequestFailed();
			return;
		}
		QJsonDocument json=QJsonDocument::fromJson(reply->readAll());
		if (json.isNull())
		{
			emit TokenRequestFailed();
			return;
		}
		if (!json.object().contains(JSON_KEY_REFRESH_TOKEN) || !json.object().contains(JSON_KEY_ACCESS_TOKEN)) return;
		settingOAuthToken.Set(json.object().value(JSON_KEY_ACCESS_TOKEN).toString());
		settingRefreshToken.Set(json.object().value(JSON_KEY_REFRESH_TOKEN).toString());
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
