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
		ensureCursorVisible();
	});
}

void ScrollingTextEdit::resizeEvent(QResizeEvent *event)
{
	QTextEdit::resizeEvent(event);
	ensureCursorVisible();
}
