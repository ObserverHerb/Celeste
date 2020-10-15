#pragma once

#include <QSettings>
#include <QColor>
#include "globals.h"

class Setting
{
public:
	Setting(const QString &category,const QString &name,const QVariant &value=QVariant()) : name(QString("%1/%2").arg(category,name)), defaultValue(value), source(Platform::Windows() ? QSettings::IniFormat : QSettings::NativeFormat,QSettings::UserScope,ORGANIZATION_NAME,APPLICATION_NAME) { }
	const QString Name() const { return name; }
	const QVariant Value() const { return source.value(name,defaultValue); }
	void Set(const QVariant &value) { source.setValue(name,value); }
	operator bool() const { return source.contains(name); }
	operator QString() const { return Value().toString(); }
	operator unsigned int() const { return Value().toUInt(); }
	operator std::chrono::milliseconds() const { return std::chrono::milliseconds(Value().toUInt()); }
	operator std::chrono::seconds() const { return std::chrono::seconds(Value().toUInt()); }
	operator QColor() const { return source.value(name,defaultValue).value<QColor>(); }
	operator QByteArray() const { return Value().toString().toLocal8Bit(); }
protected:
	const QString name;
	const QVariant defaultValue;
	QSettings source;
};

const QString SETTINGS_CATEGORY_AUTHORIZATION="Authorization";
inline Setting settingAdministrator(SETTINGS_CATEGORY_AUTHORIZATION,"Administrator");
