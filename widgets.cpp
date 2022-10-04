#include <QScrollBar>
#include <QShowEvent>
#include <QEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
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
	scrollTransition.setDuration((maximum-verticalScrollBar()->value())*10); // distance remaining * ms/step (10ms/1step)
	scrollTransition.setStartValue(verticalScrollBar()->value());
	scrollTransition.setEndValue(maximum);
	scrollTransition.start();
}

void PinnedTextEdit::Tail()
{
	if (scrollTransition.endValue() != verticalScrollBar()->maximum()) Scroll(scrollTransition.startValue().toInt(),verticalScrollBar()->maximum());
}


void PinnedTextEdit::Append(const QString &text)
{
	Tail();
	if (!toPlainText().isEmpty()) insertPlainText("\n"); // FIXME: this is really inefficient
	insertHtml(text);
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
			QTimer::singleShot(PAUSE,&scrollTransition,SLOT(&QPropertyAnimation::start())); // FIXME: this circumvents show/hide process
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

namespace UI
{
	namespace Commands
	{
		Entry::Entry(QWidget *parent) : layout(this),
			name(this),
			description(this),
			aliases(this),
			newAlias(this),
			addAlias("Add &Alias"),
			path(this),
			browse("Browse",this),
			type(this),
			random("Random",this),
			message(this),
			protect("Protect",this)
		{
			setLayout(&layout);

			QFrame *frame=new QFrame(this);
			QGridLayout *frameLayout=new QGridLayout(frame);
			frame->setLayout(frameLayout);
			frame->setFrameShape(QFrame::Box);
			layout.addWidget(frame);

			name.setPlaceholderText("Command Name");
			frameLayout->addWidget(&name,0,0,1,3);
			newAlias.setPlaceholderText("Alias");
			frameLayout->addWidget(&newAlias,1,0,1,1);
			frameLayout->addWidget(&addAlias,1,1,1,1);
			frameLayout->addWidget(&protect,1,2,1,1);
			frameLayout->addWidget(&aliases,2,0,2,3);
			description.setPlaceholderText("Command Description");
			frameLayout->addWidget(&description,0,3,1,3);
			frameLayout->addWidget(&path,1,3,1,2);
			frameLayout->addWidget(&browse,1,5,1,1);
			type.addItems({
				"Video",
				"Announce",
				"Pulsar"
			});
			type.setPlaceholderText("Native");
			frameLayout->addWidget(&type,2,3,1,2);
			frameLayout->addWidget(&random,2,5,1,1);
			frameLayout->addWidget(&message,3,3,1,3);

			connect(&name,&QLineEdit::textChanged,this,&Entry::ValidateName);
			connect(&description,&QLineEdit::textChanged,this,&Entry::ValidateDescription);
			connect(&path,&QLineEdit::textChanged,this,&Entry::ValidatePath);
			connect(&message,&QTextEdit::textChanged,this,&Entry::ValidateMessage);
			connect(&type,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&Entry::TypeChanged);
			connect(&browse,&QPushButton::clicked,this,&Entry::Browse);

			name.installEventFilter(this);
			description.installEventFilter(this);
			aliases.installEventFilter(this);
			path.installEventFilter(this);
			browse.installEventFilter(this);
			type.installEventFilter(this);
		}

		Entry::Entry(const Command &command,QWidget *parent) : Entry(parent)
		{
			name.setText(command.Name());
			description.setText(command.Description());
			protect.setChecked(command.Protected());
			path.setText(command.Path());
			random.setChecked(command.Random());
			message.setText(command.Message());

			int defaultIndex=static_cast<int>(Type());
			switch (command.Type())
			{
			case CommandType::NATIVE:
				type.setCurrentIndex(static_cast<int>(Type::NATIVE));
				break;
			case CommandType::VIDEO:
			{
				type.setCurrentIndex(static_cast<int>(Type::VIDEO));
				break;
			}
			case CommandType::AUDIO:
				type.setCurrentIndex(static_cast<int>(Type::AUDIO));
				break;
			case CommandType::PULSAR:
				type.setCurrentIndex(static_cast<int>(Type::PULSAR));
				break;
			}
			if (static_cast<int>(Type()) == defaultIndex) TypeChanged(type.currentIndex());
		}

		QString Entry::Name() const
		{
			return name.text();
		}

		QString Entry::Description() const
		{
			return description.text();
		}

		QStringList Entry::Aliases() const
		{
			QStringList list;
			for (int index=0; index < aliases.count(); index++) list.append(aliases.item(index)->text());
			return list;
		}

		void Entry::Aliases(const QStringList &names)
		{
			for (const QString &name : names) aliases.addItem(name);
		}

		QString Entry::Path() const
		{
			return path.text();
		}

		enum class Type Entry::Type() const
		{
			return static_cast<enum class Type>(type.currentIndex());
		}

		bool Entry::Random() const
		{
			return random.isChecked();
		}

		QString Entry::Message() const
		{
			return message.toPlainText();
		}

		bool Entry::Protected() const
		{
			return protect.isChecked();
		}

		void Entry::ValidateName(const QString &text)
		{
			Require(&name,text.isEmpty());
		}

		void Entry::ValidateDescription(const QString &text)
		{
			Require(&description,text.isEmpty());
		}

		void Entry::ValidatePath(const QString &text)
		{
			Require(&path,text.isEmpty());
		}

		void Entry::ValidateMessage()
		{
			if (Type() == Type::AUDIO) Require(&message,Message().isEmpty());
		}

		void Entry::Require(QWidget *widget,bool empty)
		{
			if (empty)
				widget->setStyleSheet("background-color: LavenderBlush;");
			else
				widget->setStyleSheet("background-color: none;");
		}

		void Entry::TypeChanged(int index)
		{
			if (static_cast<enum class Type>(index) == Type::PULSAR)
			{
				Pulsar();
				return;
			}

			if (static_cast<enum class Type>(index) == Type::NATIVE)
			{
				Native();
				return;
			}

			name.setEnabled(true);
			description.setEnabled(true);
			aliases.setEnabled(true);
			protect.setEnabled(true);
			path.setEnabled(true);
			browse.setEnabled(true);
			type.setEnabled(true);
			random.setEnabled(true);
			message.setEnabled(static_cast<enum class Type>(index) == Type::VIDEO ? false : true);
		}

		void Entry::Native()
		{
			name.setEnabled(false);
			description.setEnabled(false);
			aliases.setEnabled(true);
			protect.setEnabled(false);
			path.setEnabled(false);
			browse.setEnabled(false);
			type.setEnabled(false);
			random.setEnabled(false);
			message.setEnabled(false);
		}

		void Entry::Pulsar()
		{
			name.setEnabled(true);
			description.setEnabled(true);
			aliases.setEnabled(false);
			protect.setEnabled(true);
			path.setEnabled(false);
			browse.setEnabled(false);
			random.setEnabled(false);
			message.setEnabled(false);
		}

		void Entry::Browse()
		{
			QFileDialog::getOpenFileName(this,"Open File","/home","Videos (*.mp4)");
		}

		bool Entry::eventFilter(QObject *object,QEvent *event)
		{
			if (event->type() == QEvent::HoverEnter)
			{
				if (object == &name) emit Help("Name of the command. This is the text that must appear after an exclamation mark (!).");
				if (object == &description) emit Help("Short description of the command that will appear in in list of commands and showcase rotation.");
				if (object == &aliases) emit Help("List of alternate command names.");
				if (object == &path || object == &browse) emit Help("Location of the media that will be played for command types that use media, such as Video and Announce");
				if (object == &type) emit Help("Determines what kind of action is taken in response to a command.\n\nValid values are:\nAnnounce - Displays text while playing an audio file\nVideo - Displays a video");
			}

			return false;
		}

		Dialog::Dialog(const Command::Lookup &commands,QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			entriesFrame(this),
			help(this),
			labelFilter("Filter:",this),
			filter(this),
			buttons(this),
			discard("&Discard",this),
			save("&Save",this),
			newEntry("&New",this)
		{
			setStyleSheet("QFrame { background-color: palette(window); } QListWidget:enabled { background-color: palette(base); }");

			setModal(true);
			setWindowTitle("Commands Editor");

			QVBoxLayout *mainLayout=new QVBoxLayout(this);
			setLayout(mainLayout);

			QWidget *upperContent=new QWidget(this);
			QHBoxLayout *upperLayout=new QHBoxLayout(this);
			upperContent->setLayout(upperLayout);
			mainLayout->addWidget(upperContent);
			
			QPalette palette=qApp->palette();
			palette.setColor(QPalette::Window,qApp->palette().color(QPalette::Active,QPalette::Base));
			QScrollArea *scroll=new QScrollArea(this);
			scroll->setWidgetResizable(true);
			entriesFrame.setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed));
			entriesFrame.setPalette(palette);
			scroll->setWidget(&entriesFrame);
			scroll->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding));
			upperLayout->addWidget(scroll);

			QVBoxLayout *scrollLayout=new QVBoxLayout(&entriesFrame);
			entriesFrame.setLayout(scrollLayout);
			palette.setColor(QPalette::Window,qApp->palette().color(QPalette::Inactive,QPalette::AlternateBase));
			PopulateEntries(commands,scrollLayout);

			QWidget *rightPane=new QWidget(this);
			QGridLayout *rightLayout=new QGridLayout(this);
			rightPane->setLayout(rightLayout);
			help.setEnabled(false);
			help.setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding));
			rightLayout->addWidget(&help,0,0,1,2);
			labelFilter.setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
			rightLayout->addWidget(&labelFilter,1,0);
			filter.addItems({"All","Native","Dynamic","Pulsar"});
			filter.setCurrentIndex(0);
			connect(&filter,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&Dialog::FilterChanged);
			rightLayout->addWidget(&filter,1,1);
			upperLayout->addWidget(rightPane);

			QWidget *lowerContent=new QWidget(this);
			QHBoxLayout *lowerLayout=new QHBoxLayout(this);
			lowerContent->setLayout(lowerLayout);
			mainLayout->addWidget(lowerContent);

			connect(&save,&QPushButton::clicked,this,QOverload<>::of(&Dialog::Save));
			buttons.addButton(&save,QDialogButtonBox::AcceptRole);
			connect(&discard,&QPushButton::clicked,this,&Dialog::reject);
			buttons.addButton(&discard,QDialogButtonBox::RejectRole);
			buttons.addButton(&newEntry,QDialogButtonBox::ActionRole);
			lowerLayout->addWidget(&buttons);

			setSizeGripEnabled(true);
		}

		void Dialog::PopulateEntries(const Command::Lookup &commands,QLayout *layout)
		{
			std::unordered_map<QString,QStringList> aliases;
			for (const Command::Entry &pair : commands)
			{
				const Command &command=pair.second;

				if (command.Parent())
				{
					aliases[command.Parent()->Name()].push_back(command.Name());
					continue;
				}

				Entry *entry=new Entry(command,this);
				connect(entry,&Entry::Help,this,&Dialog::Help);
				entries[entry->Name()]=entry;
				layout->addWidget(entry);
			}
			for (const std::pair<QString,QStringList> &pair : aliases) entries.at(pair.first)->Aliases(pair.second);
		}

		void Dialog::Help(const QString &text)
		{
			help.setText(text);
		}

		void Dialog::FilterChanged(int index)
		{
			switch (static_cast<Filter>(index))
			{
			case Filter::ALL:
				for (std::pair<const QString,Entry*> &pair : entries) pair.second->show();
				break;
			case Filter::DYNAMIC:
				for (std::pair<const QString,Entry*> &pair : entries)
				{
					Entry *entry=pair.second;
					Type type=entry->Type();
					if (type == Type::AUDIO || type == Type::VIDEO)
						entry->show();
					else
						entry->hide();
				}
				break;
			case Filter::NATIVE:
				for (std::pair<const QString,Entry*> &pair : entries)
				{
					Entry *entry=pair.second;
					if (entry->Type() == Type::NATIVE)
						entry->show();
					else
						entry->hide();
				}
				break;
			case Filter::PULSAR:
				for (std::pair<const QString,Entry*> &pair : entries)
				{
					Entry *entry=pair.second;
					if (entry->Type() == Type::PULSAR)
						entry->show();
					else
						entry->hide();
				}
				break;
			}
		}

		void Dialog::Save()
		{
			Command::Lookup commands;

			for (const std::pair<QString,Entry*> &pair : entries)
			{
				Entry *entry=pair.second;
				CommandType type;
				switch (entry->Type())
				{
				case Type::AUDIO:
					type=CommandType::AUDIO;
					break;
				case Type::VIDEO:
					type=CommandType::VIDEO;
					break;
				case Type::PULSAR:
					type=CommandType::PULSAR;
					break;
				case Type::NATIVE:
					type=CommandType::NATIVE;
					break;
				}

				Command command={
					entry->Name(),
					entry->Description(),
					type,
					entry->Random(),
					entry->Path(),
					entry->Message(),
					entry->Protected()
				};
				commands[command.Name()]=command;

				for (const QString &alias : entry->Aliases())
				{
					commands[alias]={
						alias,
						&commands.at(command.Name())
					};
				}
			}

			emit Save(commands);
		}
	}
}