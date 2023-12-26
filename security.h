#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QMqttClient>
#include "settings.h"

inline const char *QUERY_PARAMETER_CODE="code";
inline const char *QUERY_PARAMETER_SCOPE="scope";

inline const char *NETWORK_HEADER_AUTHORIZATION="Authorization";
inline const char *NETWORK_HEADER_CLIENT_ID="Client-Id";

class PrivateSetting : public BasicSetting
{
public:
	PrivateSetting(const QString &name,const QVariant &value=QVariant()) : BasicSetting("Private",qApp->applicationName(),name,value)
	{
		std::optional<QString> filePath=Filesystem::CreateHiddenFile(source->fileName());
		if (!filePath) throw std::runtime_error("Could not create file for private settings");
		source=std::make_shared<QSettings>(*filePath,source->format());
	}
};

class Security final : public QObject
{
	Q_OBJECT
public:
	Security();
	PrivateSetting& Administrator() { return settingAdministrator; }
	PrivateSetting& OAuthToken() { return settingOAuthToken; }
	PrivateSetting& ClientID() { return settingClientID; }
	PrivateSetting& CallbackURL() { return settingCallbackURL; }
	PrivateSetting& Scope() { return settingScope; }
	const QString& AdministratorID() const;
	QByteArray Bearer(const QByteArray &token);
	void Listen();
	static const QStringList SCOPES;
private:
	PrivateSetting settingAdministrator;
	PrivateSetting settingClientID;
	PrivateSetting settingOAuthToken;
	PrivateSetting settingRefreshToken;
	PrivateSetting settingCallbackURL;
	PrivateSetting settingScope;
	PrivateSetting settingRewireSession;
	ApplicationSetting settingRewireHost;
	ApplicationSetting settingRewirePort;
	QString administratorID;
	QMqttClient *rewire;
	QMqttSubscription *rewireChannel;
	bool tokensInitialized;
	QTimer tokenValidationTimer;
	bool authorizing;
signals:
	void TokenRequestFailed();
	void Listening();
	void Disconnected();
	void Initialized();
public slots:
	void AuthorizeUser();
private slots:
	void ValidateToken();
	void ObtainAdministratorProfile();
	void RewireConnected();
	void RewireError(QMqttClient::ClientError error);
	void RewireMessage(QMqttMessage messasge);
	void Initialize();
};
