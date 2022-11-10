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
	BasicSetting(const QString &applicationName,const QString &category,const QString &name,const QVariant &value=QVariant()) : name(QString("%1/%2").arg(category,name)), defaultValue(value), source(std::make_shared<QSettings>(Platform::Windows() ? QSettings::IniFormat : QSettings::NativeFormat,QSettings::UserScope,qApp->organizationName(),applicationName)) { }
	const QString Name() const { return name; }
	const QVariant Value() const { return source->value(name,defaultValue); }
	void Set(const QVariant &value) { source->setValue(name,value); }
	void Unset() { source->remove(name); }
	operator bool() const { return source->contains(name) ? Value().userType() == QMetaType::Bool ? true : true : false; }
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
	std::shared_ptr<QSettings> source;
};

class ApplicationSetting : public BasicSetting
{
public:
	ApplicationSetting(const QString &category,const QString &name,const QVariant &value=QVariant()) : BasicSetting(qApp->applicationName(),category,name,value) { }
};

