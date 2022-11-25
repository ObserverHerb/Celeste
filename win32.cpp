#include <QFileInfo>
#include <QDir>
#include <windows.h>
#include "window.h"

Win32Window::Win32Window() : Window()
{
	// QtWin::enableBlurBehindWindow(this); // FIXME: How is transparency going to work in Qt6?
}

namespace Filesystem
{
	const std::optional<QString> CreateHiddenFile(const QString &filePath)
	{
		QFile file;
		file.setFileName(filePath);
		if (!Touch(file)) return std::nullopt;
		if (!SetFileAttributesA(StringConvert::ByteArray(filePath).constData(),FILE_ATTRIBUTE_HIDDEN)) return std::nullopt;
		return file.fileName();
	}
}
