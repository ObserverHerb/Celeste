#pragma once

#include <QTextEdit>

namespace StyleSheet
{
	const QString Colors(const QColor &foreground,const QColor &background);
}

class ScrollingTextEdit : public QTextEdit
{
public:
	ScrollingTextEdit(QWidget *parent=nullptr);
protected:
	void resizeEvent(QResizeEvent *event);
};
