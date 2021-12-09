#pragma once

#include <QSettings>
#include <QColor>
#include <QSize>
#include <QApplication>
#include <memory>
#include "globals.h"

class BasicSetting
{
public:
	BasicSetting(const QString &applicationName,const QString &category,const QString &name,const QVariant &value=QVariant()) : name(QString("%1/%2").arg(category,name)), defaultValue(value)
	{
		source=std::make_unique<QSettings>(Platform::Windows() ? QSettings::IniFormat : QSettings::NativeFormat,QSettings::UserScope,qApp->organizationName(),applicationName);
	}
	const QString Name() const { return name; }
	const QVariant Value() const { return source->value(name,defaultValue); }
	void Set(const QVariant &value) { source->setValue(name,value); }
	operator bool() const { return source->contains(name); }
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
protected:
	QString name;
	QVariant defaultValue;
	std::unique_ptr<QSettings> source;
};

class ApplicationSetting : public BasicSetting
{
public:
	ApplicationSetting(const QString &category,const QString &name,const QVariant &value=QVariant()) : BasicSetting(qApp->applicationName(),category,name,value) { }
};

class PrivateSetting : public BasicSetting
{
public:
	PrivateSetting(const QString &name,const QVariant &value=QVariant()) : BasicSetting("Private",qApp->applicationName(),name,value)
	{
		std::optional<QString> filePath=Filesystem::CreateHiddenFile(source->fileName());
		if (!filePath) throw std::runtime_error("Could not create file for private settings"); // FIXME: don't create settings objects below before main() function
		source=std::make_unique<QSettings>(*filePath,source->format());
	}
};
