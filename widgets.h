#pragma once

#include <QTextEdit>
#include <QTimer>
#include <QPropertyAnimation>

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
	QPropertyAnimation scrollTransition;
	void resizeEvent(QResizeEvent *event) override;
protected slots:
	void Tail();
	void Scroll(int minimum,int maximum);
};

class ScrollingTextEdit : public QTextEdit
{
	Q_OBJECT
public:
	ScrollingTextEdit(QWidget *parent);
protected:
	QPropertyAnimation scrollTransition;
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	const static int PAUSE;
signals:
	void Finished();
};
