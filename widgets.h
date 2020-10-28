#pragma once

#include <QTextEdit>

class ScrollingTextEdit : public QTextEdit
{
public:
	ScrollingTextEdit(QWidget *parent=nullptr);
protected:
	void resizeEvent(QResizeEvent *event);
};
