#include <QFileInfo>
#include <QDir>
#include "globals.h"

namespace Filesystem
{
	const std::optional<QString> CreateHiddenFile(const QString &filePath)
	{
		QFile file;
		const QFileInfo details(filePath);
		const QDir path(details.absolutePath());
		file.setFileName(QString("%1%2.%3").arg(path.absolutePath(),QDir::separator(),details.fileName()));
		if (!Touch(file)) return std::nullopt;
		return file.fileName();
	}
}
