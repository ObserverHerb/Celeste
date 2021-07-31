#pragma once

#include <QString>
#include <QNetworkAccessManager>

struct Emote
{
	QString id;
	QString path;
	unsigned int start;
	unsigned int end;
	bool operator<(const Emote &other) { return start < other.start; }
};
