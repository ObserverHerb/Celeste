#include "widgets.h"

ScrollingTextEdit::ScrollingTextEdit(QWidget *parent) : QTextEdit(parent)
{
	connect(this,&ScrollingTextEdit::textChanged,[this]() {
		ensureCursorVisible();
	});
}

void ScrollingTextEdit::resizeEvent(QResizeEvent *event)
{
	QTextEdit::resizeEvent(event);
	ensureCursorVisible();
}
