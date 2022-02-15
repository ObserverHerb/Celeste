#include <QFileInfo>
#include <QDir>
#include "globals.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif

namespace Filesystem
{
	const std::optional<QString> CreateHiddenFile(const QString &filePath)
	{
		QFile file;
#ifdef Q_OS_WIN
		file.setFileName(filePath);
		if (!Touch(file)) return std::nullopt;
		if (!SetFileAttributesA(StringConvert::ByteArray(filePath).constData(),FILE_ATTRIBUTE_HIDDEN)) return std::nullopt;
#else
		const QFileInfo details(filePath);
		const QDir path(details.absolutePath());
		file.setFileName(QString("%1%2.%3").arg(path.absolutePath(),QDir::separator(),details.fileName()));
		if (!Touch(file)) return std::nullopt;
#endif
		return file.fileName();
	}
}
