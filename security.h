#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include "settings.h"

inline const char *QUERY_PARAMETER_CODE="code";
inline const char *QUERY_PARAMETER_SCOPE="scope";

class PrivateSetting : public BasicSetting
{
public:
	PrivateSetting(const QString &name,const QVariant &value=QVariant()) : BasicSetting("Private",qApp->applicationName(),name,value)
	{
		std::optional<QString> filePath=Filesystem::CreateHiddenFile(source->fileName());
		if (!filePath) throw std::runtime_error("Could not create file for private settings");
		source=std::make_unique<QSettings>(*filePath,source->format());
	}
};

class Security final : public QObject
{
	Q_OBJECT
public:
	Security();
	PrivateSetting& Administrator() { return settingAdministrator; }
	PrivateSetting& OAuthToken() { return settingOAuthToken; }
	PrivateSetting& ServerToken() { return settingServerToken; }
	PrivateSetting& ClientID() { return settingClientID; }
	PrivateSetting& ClientSecret() { return settingClientSecret; }
	PrivateSetting& CallbackURL() { return settingCallbackURL; }
	PrivateSetting& Scope() { return settingScope; }
	void AuthorizeUser();
	void RequestToken(const QString &code,const QString &scopes);
	static const QStringList SCOPES;
private:
	PrivateSetting settingAdministrator;
	PrivateSetting settingClientID;
	PrivateSetting settingClientSecret;
	PrivateSetting settingOAuthToken;
	PrivateSetting settingRefreshToken;
	PrivateSetting settingServerToken;
	PrivateSetting settingCallbackURL;
	PrivateSetting settingScope;
	static QTimer tokenTimer;
signals:
	void TokenRequestFailed();
public slots:
	void StartClocks();
private slots:
	void RefreshToken();
};
