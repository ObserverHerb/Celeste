#pragma once

#include <QSettings>
#include <QColor>
#include <QSize>
#include <memory>
#include "globals.h"

inline const char *SETTINGS_CATEGORY_AUTHORIZATION="Authorization";

class BasicSetting
{
public:
	BasicSetting(const QString &applicationName,const QString &category,const QString &name,const QVariant &value=QVariant()) : name(QString("%1/%2").arg(category,name)), defaultValue(value)
	{
		source=std::make_unique<QSettings>(Platform::Windows() ? QSettings::IniFormat : QSettings::NativeFormat,QSettings::UserScope,ORGANIZATION_NAME,applicationName);
	}
	const QString Name() const { return name; }
	const QVariant Value() const { return source->value(name,defaultValue); }
	void Set(const QVariant &value) { source->setValue(name,value); }
	operator bool() const { return source->contains(name); }
	operator QString() const { return Value().toString(); }
	operator StringConvert::Valid() const { return Value().toString(); }
	operator unsigned int() const { return Value().toUInt(); }
	operator int() const { return Value().toInt(); }
	operator quint16() const { return Value().toUInt(); }
	operator qint64() const { return Value().toLongLong(); }
	operator qreal() const { return Value().toReal(); }
	operator std::chrono::milliseconds() const { return std::chrono::milliseconds(Value().toUInt()); }
	operator std::chrono::seconds() const { return std::chrono::seconds(Value().toUInt()); }
	operator QColor() const { return source->value(name,defaultValue).value<QColor>(); }
	operator QSize() const { return source->value(name,defaultValue).toSize(); }
	void operator=(const QSize &size) { source->setValue(name,size); }
	operator QByteArray() const { return Value().toString().toLocal8Bit(); }
protected:
	QString name;
	QVariant defaultValue;
	std::unique_ptr<QSettings> source;
};

class ApplicationSetting : public BasicSetting
{
public:
	ApplicationSetting(const QString &category,const QString &name,const QVariant &value=QVariant()) : BasicSetting(APPLICATION_NAME,category,name,value) { }
};

class AuthorizationSetting : public BasicSetting
{
public:
	AuthorizationSetting(const QString &name,const QVariant &value=QVariant()) : BasicSetting(SETTINGS_CATEGORY_AUTHORIZATION,APPLICATION_NAME,name,value)
	{
		std::optional<QString> filePath=Filesystem::CreateHiddenFile(source->fileName());
		if (!filePath) throw std::runtime_error("Could not create file for private settings"); // FIXME: don't create settings objects below before main() function
		source=std::make_unique<QSettings>(*filePath,source->format());
	}
};

inline AuthorizationSetting settingAdministrator("Administrator");
inline AuthorizationSetting settingClientID("ClientID");
inline AuthorizationSetting settingOAuthToken("Token");
