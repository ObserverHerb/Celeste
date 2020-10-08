#pragma once

#include <QSettings>
#include "globals.h"

class Setting
{
public:
	Setting(const QString name,const QVariant value) : name(name), defaultValue(value) { }
	const QString Name() const { return name; }
	const QVariant Value() const { return defaultValue; }
protected:
	const QString name;
	const QVariant defaultValue;
};

class SettingCategory
{
public:
	SettingCategory(const QString name) : name(name) { }
	const QString Path(const QString &settingName) const { return QString("%1/%2").arg(name,settingName); }
protected:
	const QString name;
};

class Settings : public QSettings
{
public:
	Settings(QObject *parent=nullptr) : QSettings(ORGANIZATION_NAME,APPLICATION_NAME,parent) { }
	QVariant Value(const Setting &setting) const { return value(setting.Name(),setting.Value()); }
};

