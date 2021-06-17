#include <QDir>
#include <QFileInfo>
#include <QString>
#include "globals.h"
#include "recognizers.h"

/*!
 * \class FileRecognizer
 * \brief Performs operations a file that has been "recognized" from a
 * path to a file or directory
 */

/*!
 * \fn FileRecognizer::FileRecognizer
 * \brief Default constructor
 *
 * \param path A path to a file or directory
 */
FileRecognizer::FileRecognizer(const QString &path)
{
	const QFileInfo pathInfo(path);
	if (pathInfo.isDir())
	{
		for (const QFileInfo &fileInfo : QDir(path).entryInfoList())
		{
			if (fileInfo.isFile()) files.push_back(fileInfo.absoluteFilePath());
		}
	}
	else
	{
		files.push_back(pathInfo.absoluteFilePath());
	}
}

const QString FileRecognizer::File(const int index)
{
	return files.at(index);
}

/*!
 * \fn FileRecognizer::First
 * \brief Return a path to the first file that was recognized
 */
const QString FileRecognizer::First()
{
	return files.front();
}

/*!
 * \fn FileRecognizer::First
 * \brief Return a path to a random file in the list of files
 * that was recognized
 */
const QString FileRecognizer::Random()
{
	return File(RandomIndex());
}

int FileRecognizer::RandomIndex()
{
	return Random::Bounded(files);
}
