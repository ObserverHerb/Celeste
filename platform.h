#pragma once

#include <QString>
#include <QFile>
#include <optional>

namespace StringConvert
{
	QByteArray ByteArray(const QString &value);
}

namespace Platform
{
	constexpr bool Windows()
	{
#ifdef Q_OS_WIN
		return true;
#else
		return false;
#endif
	}
}

namespace Filesystem
{
	bool Touch(QFile &file);
	const std::optional<QString> CreateHiddenFile(const QString &filePath);
}
