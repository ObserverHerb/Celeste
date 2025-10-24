#pragma once

#include <QSettings>
#include <QColor>
#include <QSize>
#include <QApplication>
#include <QUrl>
#include <memory>
#include "globals.h"

class BasicSetting
{
public:
	BasicSetting(const QString &applicationName,const QString &category,const QString &name,const QVariant &defaultValue=QVariant()) : name(QString("%1/%2").arg(category,name)), defaultValue(defaultValue), source(std::make_shared<QSettings>(Platform::Windows() ? QSettings::IniFormat : QSettings::NativeFormat,QSettings::UserScope,qApp->organizationName(),applicationName))
	{
		// if the setting is the word "true" or "false", convert the value to an actual boolean
		// this is because everything is read in as a QString (https://stackoverflow.com/questions/32654233/qsettings-with-different-types)
		// this is a problem because I'm relying on type information to differentiate between whether a value is false or nonexistent
		// this hack will have unintended side-effects that may become a problem later
		QString text=Value().toString();
		if (text == "true")	Set(true);
		if (text == "false") Set(false);
	}
	const QString Name() const { return name; }
	void Save() { source->sync(); }
	const QVariant Value() const { return source->value(name,defaultValue); }
	void Set(const QVariant &value) { source->setValue(name,value); }
	void Unset() { source->remove(name); }
	operator bool() const
	{
		const QVariant candidate=Value();
		if (!candidate.isValid()) return false; // setting does not appear in file, no default value
		if (Value().userType() == QMetaType::Bool) return candidate.toBool(); // setting appears in file and it a boolean
		return true; // setting appears in file
	}
	operator QString() const { return Value().toString(); }
	operator unsigned int() const { return Value().toUInt(); }
	operator int() const { return Value().toInt(); }
	operator quint16() const { return Value().toUInt(); }
	operator qint64() const { return Value().toLongLong(); }
	operator qreal() const { return Value().toReal(); }
	operator std::chrono::milliseconds() const { return std::chrono::milliseconds(Value().toUInt()); }
	operator std::chrono::seconds() const { return std::chrono::seconds(Value().toUInt()); }
	operator QColor() const { return source->value(name,defaultValue).value<QColor>(); }
	operator QSize() const { return source->value(name,defaultValue).toSize(); }
	operator QByteArray() const { return Value().toString().toLocal8Bit(); }
	operator QUrl() const { return Value().toUrl(); }
protected:
	QString name;
	QVariant defaultValue;
	std::shared_ptr<QSettings> source;
};

class ApplicationSetting : public BasicSetting
{
public:
	ApplicationSetting(const QString &category,const QString &name,const QVariant &value=QVariant()) : BasicSetting(qApp->applicationName(),category,name,value) { }
};

namespace Settings
{
	struct Channel
	{
		ApplicationSetting name;
		ApplicationSetting protect;
	};

	struct Bot
	{
		ApplicationSetting inactivityCooldown;
		ApplicationSetting helpCooldown;
		ApplicationSetting textWallThreshold;
		ApplicationSetting textWallSound;
		ApplicationSetting roasts;
		ApplicationSetting portraitVideo;
		ApplicationSetting arrivalSound;
		ApplicationSetting cheerVideo;
		ApplicationSetting subscriptionSound;
		ApplicationSetting raidSound;
		ApplicationSetting raidInterruptDuration;
		ApplicationSetting raidInterruptDelayThreshold;
		ApplicationSetting deniedCommandVideo;
		ApplicationSetting commandCooldown;
		ApplicationSetting uptimeHistory;
		ApplicationSetting commandNameAgenda;
		ApplicationSetting commandNameStreamCategory;
		ApplicationSetting commandNameStreamTitle;
		ApplicationSetting commandNameCommands;
		ApplicationSetting commandNameEmote;
		ApplicationSetting commandNameFollowage;
		ApplicationSetting commandNameHTML;
		ApplicationSetting commandNameLimit;
		ApplicationSetting commandNamePanic;
		ApplicationSetting commandNameShoutout;
		ApplicationSetting commandNameSong;
		ApplicationSetting commandNameTimezone;
		ApplicationSetting commandNameUptime;
		ApplicationSetting commandNameTotalTime;
		ApplicationSetting commandNameVibe;
		ApplicationSetting commandNameVibeVolume;
	};

	struct Pulsar
	{
		ApplicationSetting enabled;
		ApplicationSetting reconnectDelay;
	};
}
