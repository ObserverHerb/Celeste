#pragma once

#include <QString>
#include <unordered_map>

class Viewer
{
public:
	Viewer(const QString &name) : name(name) { }
	const QString Name() const { return name; }
protected:
	QString name;
};
using Viewers=std::unordered_map<QString,Viewer>;
