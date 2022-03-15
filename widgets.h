#pragma once

#include <QTextEdit>
#include <QTimer>

namespace StyleSheet
{
	const QString Colors(const QColor &foreground,const QColor &background);
}

class PinnedTextEdit : public QTextEdit
{
	Q_OBJECT
public:
	PinnedTextEdit(QWidget *parent);
	void Append(const QString &text);
protected:
	void resizeEvent(QResizeEvent *event);
	void Tail();
};

class ScrollingTextEdit : public QTextEdit
{
	Q_OBJECT
public:
	ScrollingTextEdit(QWidget *parent);
protected:
	void showEvent(QShowEvent *event) override;
	QTimer scrollTimer;
	const static int PAUSE;
signals:
	void Finished();
protected slots:
	void Scrolled(int position);
};