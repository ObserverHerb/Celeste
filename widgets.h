#pragma once

#include <QTextEdit>
#include <QTimer>
#include <QPropertyAnimation>
#include <QLineEdit>
#include <QListWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QDialogButtonBox>
#include <QSizeGrip>
#include <QDialog>
#include "entities.h"

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
	void contextMenuEvent(QContextMenuEvent *event) override;
signals:
	void ContextMenu(QContextMenuEvent *event);
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

namespace UI
{
	namespace Commands
	{
		enum class Type
		{
			NATIVE=-1,
			VIDEO,
			AUDIO,
			PULSAR
		};

		enum class Filter
		{
			ALL,
			NATIVE,
			DYNAMIC,
			PULSAR
		};

		class Entry : public QWidget
		{
			Q_OBJECT
		public:
			Entry(QWidget *parent);
			Entry(const Command &command,QWidget *parent);
			QString Name() const;
			QString Description() const;
			QStringList Aliases() const;
			void Aliases(const QStringList &names);
			QString Path() const;
			enum class Type Type() const;
			bool Random() const;
			QString Message() const;
			bool Protected() const;
		protected:
			QGridLayout layout;
			QLineEdit name;
			QLineEdit description;
			QListWidget aliases;
			QLineEdit newAlias;
			QPushButton addAlias;
			QLineEdit path;
			QPushButton browse;
			QComboBox type;
			QCheckBox random;
			QCheckBox protect;
			QTextEdit message;
			void Require(QWidget *widget,bool empty);
			void TypeChanged(int index);
			void Native();
			void Pulsar();
			void Browse();
			bool eventFilter(QObject *object,QEvent *event) override;
		signals:
			void Help(const QString &text);
		public slots:
			void ValidateName(const QString &text);
			void ValidateDescription(const QString &text);
			void ValidatePath(const QString &text);
			void ValidateMessage();
		};

		class Dialog : public QDialog		
		{
			Q_OBJECT
		public:
			Dialog(const Command::Lookup &commands,QWidget *parent);
		protected:
			QWidget entriesFrame;
			QTextEdit help;
			QLabel labelFilter;
			QComboBox filter;
			QDialogButtonBox buttons;
			QPushButton discard;
			QPushButton save;
			QPushButton newEntry;
			std::unordered_map<QString,Entry*> entries;
			void PopulateEntries(const Command::Lookup &commands,QLayout *layout);
			void Help(const QString &text);
			void Save();
		signals:
			void Save(const Command::Lookup &commands);
		public slots:
			void FilterChanged(int index);
		};
	}
}