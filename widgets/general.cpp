#include <QScrollBar>
#include "widgets/widgets.h"

StaticTextEdit::StaticTextEdit(QWidget *parent) : QTextEdit(parent) { }

void StaticTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
	emit ContextMenu(event);
}

PinnedTextEdit::PinnedTextEdit(QWidget *parent) : QTextEdit(parent), scrollTransition(QPropertyAnimation(verticalScrollBar(),"sliderPosition"))
{
	connect(&scrollTransition,&QPropertyAnimation::finished,this,&PinnedTextEdit::Tail);
	connect(verticalScrollBar(),&QScrollBar::rangeChanged,this,&PinnedTextEdit::Scroll);
}

void PinnedTextEdit::resizeEvent(QResizeEvent *event)
{
	Tail();
	QTextEdit::resizeEvent(event);
}

void PinnedTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
	emit ContextMenu(event);
}

void PinnedTextEdit::Scroll(int minimum,int maximum)
{
	Q_UNUSED(minimum)
	scrollTransition.setDuration((maximum-verticalScrollBar()->value())*10); // distance remaining * ms/step (10ms/1step)
	scrollTransition.setStartValue(verticalScrollBar()->value());
	scrollTransition.setEndValue(maximum);
	scrollTransition.start();
}

void PinnedTextEdit::Tail()
{
	if (scrollTransition.endValue() != verticalScrollBar()->maximum()) Scroll(scrollTransition.startValue().toInt(),verticalScrollBar()->maximum());
}


void PinnedTextEdit::Append(const QString &text,const QString &id)
{
	Tail();
	QTextCursor cursor=document()->rootFrame()->lastCursorPosition();
	QTextFrameFormat format;
	format.setBorderStyle(QTextFrameFormat::BorderStyle_None);
	frames.try_emplace(id,cursor.insertFrame(format));
	cursor.insertHtml(text);
}

void PinnedTextEdit::Remove(const QString &id)
{
	auto frame=frames.find(id);
	if (frame == frames.end()) return;
	QTextCursor cursor=frame->second->firstCursorPosition();
	// NOTE: I'm not sure why this is deleting all the blocks
	// in the frame without me having to loop through them.
	cursor.select(QTextCursor::BlockUnderCursor);
	cursor.removeSelectedText();
	frames.erase(frame);
}

const int ScrollingTextEdit::PAUSE=5000;

ScrollingTextEdit::ScrollingTextEdit(QWidget *parent) : QTextEdit(parent), scrollTransition(QPropertyAnimation(verticalScrollBar(),"sliderPosition"))
{
	connect(&scrollTransition,&QPropertyAnimation::finished,this,&ScrollingTextEdit::Finished);
}

void ScrollingTextEdit::showEvent(QShowEvent *event)
{
	if (!event->spontaneous())
	{
		if (scrollTransition.state() == QAbstractAnimation::Paused)
		{
			scrollTransition.start();
		}
		else
		{
			scrollTransition.setDuration((verticalScrollBar()->maximum()-verticalScrollBar()->value())*25); // distance remaining * ms/step (10ms/1step)
			scrollTransition.setStartValue(verticalScrollBar()->value());
			scrollTransition.setEndValue(verticalScrollBar()->maximum());
			QTimer::singleShot(PAUSE,&scrollTransition,[this]() {
				scrollTransition.start(); // have to use lambda because start() has a default parameter that breaks usual connection syntax
			});
		}
	}

	QTextEdit::showEvent(event);
}

void ScrollingTextEdit::hideEvent(QHideEvent *event)
{
	if (!event->spontaneous())
	{
		if (scrollTransition.state() == QAbstractAnimation::Running) scrollTransition.pause();
	}

	QTextEdit::hideEvent(event);
}

SingleSelectionListWidget::SingleSelectionListWidget(QWidget *parent): QListWidget(parent)
{
	setSelectionMode(QAbstractItemView::SingleSelection);
}

void SingleSelectionListWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::RightButton)
	{
		clearSelection();
		setCurrentItem(nullptr);
		event->accept();
		return;
	}

	QListWidget::mousePressEvent(event);
}
