#include "globals.h"
#include "widgets.h"

namespace StyleSheet
{
	const QString Colors(const QColor &foreground,const QColor &background)
	{
		return QString("color: rgba(%1,%2,%3,%4); background-color: rgba(%5,%6,%7,%8);").arg(
			StringConvert::Integer(foreground.red()),
			StringConvert::Integer(foreground.green()),
			StringConvert::Integer(foreground.blue()),
			StringConvert::Integer(foreground.alpha()),
			StringConvert::Integer(background.red()),
			StringConvert::Integer(background.green()),
			StringConvert::Integer(background.blue()),
			StringConvert::Integer(background.alpha())
		);
	}
}

ScrollingTextEdit::ScrollingTextEdit(QWidget *parent) : QTextEdit(parent)
{
	connect(this,&ScrollingTextEdit::textChanged,[this]() {
		Tail();
	});
}

void ScrollingTextEdit::resizeEvent(QResizeEvent *event)
{
	Tail();
	QTextEdit::resizeEvent(event);
}

void ScrollingTextEdit::Tail()
{
	QTextCursor cursor=textCursor();
	cursor.movePosition(QTextCursor::End);
	setTextCursor(cursor);
	ensureCursorVisible();
}

void ScrollingTextEdit::Append(const QString &text)
{
	Tail();
	if (!toPlainText().isEmpty()) insertPlainText("\n"); // FIXME: this is really inefficient
	insertHtml(text);
}
