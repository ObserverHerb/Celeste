#include <QScrollBar>
#include <QShowEvent>
#include <QEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QFontDialog>
#include <QVideoWidget>
#include <QScreen>
#include <QMessageBox>
#include <QInputDialog>
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

namespace UI
{
	void Valid(QWidget *widget,bool valid)
	{
		if (valid)
			widget->setStyleSheet("background-color: none;");
		else
			widget->setStyleSheet("background-color: LavenderBlush;");
	}

	void Require(QWidget *widget,bool empty)
	{
		Valid(widget,!empty);
	}

	QString OpenVideo(QWidget *parent,QString initialPath)
	{
		return QDir::toNativeSeparators(QFileDialog::getOpenFileName(parent,Text::DIALOG_TITLE_FILE,initialPath.isEmpty() ? Filesystem::HomePath().absolutePath() : initialPath,QString("Videos (*.%1)").arg(Text::FILE_TYPE_VIDEO)));
	}

	QString OpenAudio(QWidget *parent,QString initialPath)
	{
		return QDir::toNativeSeparators(QFileDialog::getOpenFileName(parent,Text::DIALOG_TITLE_FILE,initialPath.isEmpty() ? Filesystem::HomePath().absolutePath() : initialPath,QString("Audios (*.%1)").arg(Text::FILE_TYPE_AUDIO)));
	}

	Help::Help(QWidget *parent) : QTextEdit(parent)
	{
		setEnabled(false);
		setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding));
		setStyleSheet(QStringLiteral("border: none; color: palette(window-text);"));
	}

	Color::Color(QWidget *parent,const QString &color) : QLabel(parent)
	{
		Set(color);
		setText(QStringLiteral("preview"));
	}

	void Color::Set(const QString &color)
	{
		setStyleSheet(QString("border: 1px solid black; color: %1; background-color: %1;").arg(color));
	}

	namespace Security
	{
		Scopes::Scopes(QWidget *parent) : QDialog(parent),
			layout(this),
			list(this),
			scopes({"chat:read"})
		{
			setLayout(&layout);
			setSizeGripEnabled(true);

			list.setSelectionMode(QAbstractItemView::ExtendedSelection);
			list.addItems(::Security::SCOPES);
			layout.addWidget(&list);

			QDialogButtonBox *buttons=new QDialogButtonBox(this);
			QPushButton *okay=buttons->addButton(QDialogButtonBox::Ok);
			okay->setDefault(true);
			layout.addWidget(buttons);

			connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
			connect(this,&QDialog::accepted,this,&Scopes::Save);
		}

		QStringList Scopes::operator()()
		{
			return scopes;
		}

		void Scopes::Save()
		{
			for (QListWidgetItem *item : list.selectedItems()) scopes.append(item->text());
		}
	}

	namespace Commands
	{
		Aliases::Aliases(QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			layout(this),
			list(this),
			name(this),
			add(Text::BUTTON_ADD,this),
			remove(Text::BUTTON_REMOVE,this)
		{
			setModal(true);
			setWindowTitle("Command Aliases");

			setLayout(&layout);

			name.setPlaceholderText("Alias");

			layout.addWidget(&list,0,0,1,3);
			layout.addWidget(&name,1,0);
			layout.addWidget(&add,1,1);
			layout.addWidget(&remove,1,2);

			connect(&add,&QPushButton::clicked,this,&Aliases::Add);
			connect(&remove,&QPushButton::clicked,this,&Aliases::Remove);

			setSizeGripEnabled(true);
		}

		void Aliases::Populate(const QStringList &names)
		{
			list.clear();
			for (const QString &name : names) list.addItem(name);
		}

		QStringList Aliases::operator()() const
		{
			QStringList result;
			for (int index=0; index < list.count(); index++) result.append(list.item(index)->text());
			return result;
		}

		void Aliases::Add()
		{
			const QString candidate=name.text();
			if (!list.findItems(candidate,Qt::MatchExactly).isEmpty())
			{
				QMessageBox duplicateDialog;
				duplicateDialog.setWindowTitle("Duplicate Alias");
				duplicateDialog.setText("Alias already exists in the list");
				duplicateDialog.setIcon(QMessageBox::Warning);
				duplicateDialog.setStandardButtons(QMessageBox::Ok);
				duplicateDialog.setDefaultButton(QMessageBox::Ok);
				duplicateDialog.exec();
				return;
			}

			list.addItem(candidate);
			name.clear();
		}

		void Aliases::Remove()
		{
			QListWidgetItem *item=list.currentItem();
			if (!item) return;
			item=list.takeItem(list.row(item));
			if (item) delete item;
		}

		void Aliases::hideEvent(QHideEvent *event)
		{
			emit Finished();
			QDialog::hideEvent(event);
		}

		Entry::Entry(QWidget *parent) : QWidget(parent),
			commandProtect(-1),
			commandRandom(-1),
			commandDuplicates(-1),
			commandType(UI::Commands::Type::INVALID),
			layout(this),
			details(nullptr),
			detailsLayout(nullptr),
			header(this),
			name(nullptr),
			description(nullptr),
			openAliases(nullptr),
			path(nullptr),
			browse(nullptr),
			type(nullptr),
			random(nullptr),
			duplicates(nullptr),
			protect(nullptr),
			message(nullptr),
			aliases(nullptr)
		{
			setLayout(&layout);

			QFrame *frame=new QFrame(this);
			QGridLayout *frameLayout=new QGridLayout(frame);
			frame->setLayout(frameLayout);
			frame->setFrameShape(QFrame::Box);
			layout.addWidget(frame);

			header.setStyleSheet(QString("font-size: %1pt; font-weight: bold; text-align: left;").arg(header.font().pointSizeF()*1.25));
			header.setCursor(Qt::PointingHandCursor);
			header.setFlat(true);
			frameLayout->addWidget(&header);

			details=new QFrame(this);
			details->setLayout(&detailsLayout);
			details->setFrameShape(QFrame::NoFrame);
			details->setVisible(false);
			frameLayout->addWidget(details);

			connect(&header,&QPushButton::clicked,this,&Entry::ToggleFold);
		}

		Entry::Entry(const Command &command,QWidget *parent) : Entry(parent)
		{
			commandName=command.Name();
			commandDescription=command.Description();
			commandProtect=command.Protected() ? 1 : 0;
			commandPath=command.Path();
			commandRandom=command.Random() ? 1 : 0;
			commandDuplicates=command.Duplicates() ? 1 : 0;
			commandMessage=command.Message();

			switch (command.Type())
			{
			case CommandType::BLANK:
				throw std::logic_error(u"Warning: Type for command !%1 was blank."_s.arg(commandName).toStdString());
			case CommandType::NATIVE:
				commandType=Type::NATIVE;
				break;
			case CommandType::VIDEO:
				commandType=Type::VIDEO;
				throw std::logic_error(u"Warning: Type for command !%1 was blank."_s.arg(commandName).toStdString());
				break;
			case CommandType::AUDIO:
				commandType=Type::AUDIO;
				break;
			case CommandType::PULSAR:
				commandType=Type::PULSAR;
				break;
			}

			UpdateHeader(Name());
		}

		QString Entry::Name() const
		{
			if (commandName.isNull())
				return name->text();
			else
				return commandName;
		}

		QString Entry::Description() const
		{
			if (commandDescription.isNull())
				return description->text();
			else
				return commandDescription;
		}

		QStringList Entry::Aliases() const
		{
			if (aliases)
				return (*aliases)();
			else
				return commandAliases;
		}

		void Entry::Aliases(const QStringList &names)
		{
			if (aliases)
				aliases->Populate(names);
			else
				commandAliases=names;
			UpdateHeader();
		}

		QString Entry::Path() const
		{
			if (commandPath.isNull())
				return path->text();
			else
				return commandPath;
		}

		QStringList Entry::Filters() const
		{
			return Command::FileListFilters(Type());
		}

		CommandType Entry::Type() const
		{
			UI::Commands::Type candidate;
			if (commandType == UI::Commands::Type::INVALID)
				candidate=static_cast<enum Type>(type->currentIndex());
			else
				candidate=commandType;

			switch (candidate)
			{
			case Type::AUDIO:
				return CommandType::AUDIO;
			case Type::VIDEO:
				return CommandType::VIDEO;
			case Type::PULSAR:
				return CommandType::PULSAR;
			case Type::NATIVE:
				return CommandType::NATIVE;
			case Type::INVALID:
				throw std::logic_error(u"Fatal: An unrecognized type was selected for command !%1."_s.arg(commandName).toStdString());
			}
		}

		bool Entry::Random() const
		{
			if (commandRandom < 0)
				return random->isChecked();
			else
				return commandRandom > 0;
		}

		bool Entry::Duplicates() const
		{
			if (commandDuplicates < 0)
				return duplicates->isChecked();
			else
				return commandDuplicates > 0;
		}

		QString Entry::Message() const
		{
			if (commandMessage.isNull())
				return message->toPlainText();
			else
				return commandMessage;
		}

		bool Entry::Protected() const
		{
			if (commandProtect < 0)
				return protect->isChecked();
			else
				return commandProtect > 0;
		}

		void Entry::UpdateHeader()
		{
			UpdateHeader(Name());
		}

		void Entry::UpdateHeader(const QString &commandName)
		{
			QString text=commandName;
			const QStringList commandAliases=Aliases();
			if (!commandAliases.isEmpty()) text.append(QString(" (%1)").arg(commandAliases.join(", ")));
			header.setText(text);
		}

		void Entry::ToggleFold()
		{
			if (details->isVisible())
			{
				details->setVisible(false);
			}
			else
			{
				if (!name)
				{
					name=new QLineEdit(details);
					name->setPlaceholderText(u"Command Name"_s);
					name->setText(commandName);
					commandName.clear();
					connect(name,&QLineEdit::textChanged,this,&Entry::ValidateName);
					connect(name,&QLineEdit::textChanged,this,QOverload<const QString&>::of(&Entry::UpdateHeader));
					name->installEventFilter(this);
					detailsLayout.addWidget(name,0,0);
				}

				if (!openAliases)
				{
					aliases=new UI::Commands::Aliases(this);
					aliases->Populate(commandAliases);
					commandAliases.clear();
					connect(aliases,&Aliases::Finished,this,QOverload<>::of(&Entry::UpdateHeader));
					aliases->installEventFilter(this);
					openAliases=new QPushButton(u"Aliases"_s,details);
					connect(openAliases,&QPushButton::clicked,aliases,&QDialog::show);
					detailsLayout.addWidget(openAliases,0,1);
				}

				if (!protect)
				{
					protect=new QCheckBox(u"Protect"_s,details);
					protect->setChecked(commandProtect > 0 ? true : false);
					commandProtect=-1;
					detailsLayout.addWidget(protect,0,2);
				}

				if (!description)
				{
					description=new QLineEdit(details);
					description->setPlaceholderText(u"Command Description"_s);
					description->setText(commandDescription);
					commandDescription.clear();
					connect(description,&QLineEdit::textChanged,this,&Entry::ValidateDescription);
					description->installEventFilter(this);
					detailsLayout.addWidget(description,1,0,1,3);
				}

				if (!path)
				{
					path=new QLineEdit(details);
					path->setText(commandPath);
					commandPath.clear();
					connect(path,&QLineEdit::textChanged,this,QOverload<const QString&>::of(&Entry::ValidatePath));
					path->installEventFilter(this);
					detailsLayout.addWidget(path,2,0,1,2);
				}

				if (!browse)
				{
					browse=new QPushButton(Text::BROWSE,details);
					connect(browse,&QPushButton::clicked,this,&Entry::Browse);
					browse->installEventFilter(this);
					detailsLayout.addWidget(browse,2,2,1,1);
				}

				if (!type)
				{
					type=new QComboBox(details);
					type->addItems({
						u"Video"_s,
						u"Announce"_s,
						u"Pulsar"_s
					});
					type->setPlaceholderText(u"Native"_s);
					type->setCurrentIndex(static_cast<int>(commandType));
					commandType=UI::Commands::Type::INVALID;
					connect(type,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&Entry::TypeChanged);
					type->installEventFilter(this);
					detailsLayout.addWidget(type,3,0,1,1);
				}

				if (!random)
				{
					random=new QCheckBox(u"Random"_s,details);
					random->setChecked(commandRandom > 0 ? true : false);
					commandRandom=-1;
					connect(random,&QCheckBox::stateChanged,this,&Entry::RandomChanged);
					detailsLayout.addWidget(random,3,1,1,1);
				}

				if (!duplicates)
				{
					duplicates=new QCheckBox(u"Duplicates"_s,details);
					duplicates->setChecked(commandDuplicates > 0 ? true : false);
					commandDuplicates=-1;
					detailsLayout.addWidget(duplicates,3,2,1,1);
				}

				if (!message)
				{
					message=new QTextEdit(details);
					message->setText(commandMessage);
					commandMessage.clear();
					connect(message,&QTextEdit::textChanged,this,&Entry::ValidateMessage);
					detailsLayout.addWidget(message,4,0,1,3);
				}
			}
			details->setVisible(true);
		}

		void Entry::ValidateName(const QString &text)
		{
			Require(name,text.isEmpty());
		}

		void Entry::ValidateDescription(const QString &text)
		{
			Require(description,text.isEmpty());
		}

		void Entry::ValidatePath(const QString &text)
		{
			ValidatePath(text,Random(),static_cast<UI::Commands::Type>(type->currentIndex()));
		}

		void Entry::ValidatePath(const QString &text,bool random,const enum Type type)
		{
			QFileInfo candidate(text);

			if (!candidate.exists())
			{
				Valid(path,false);
				return;
			}

			if (random)
			{
				Valid(path,candidate.isDir());
				return;
			}
			else
			{
				if (candidate.isDir())
				{
					Valid(path,false);
					return;
				}

				const QString extension=candidate.suffix();
				switch (type)
				{
				case Type::VIDEO:
					Valid(path,extension == Text::FILE_TYPE_VIDEO);
					break;
				case Type::AUDIO:
					Valid(path,extension == Text::FILE_TYPE_AUDIO);
					break;
				default:
					Valid(path,false);
					break;
				}
			}
		}

		void Entry::ValidateMessage()
		{
			if (static_cast<UI::Commands::Type>(type->currentIndex()) == UI::Commands::Type::AUDIO) Require(message,Message().isEmpty());
		}

		void Entry::RandomChanged(const int state)
		{
			bool checked=state == Qt::Checked;
			duplicates->setEnabled(checked);
			ValidatePath(Path(),checked,static_cast<UI::Commands::Type>(type->currentIndex()));
		}

		void Entry::TypeChanged(int index)
		{
			enum Type currentType=static_cast<enum Type>(index);

			if (currentType == Type::PULSAR)
			{
				Pulsar();
				return;
			}

			if (currentType == Type::NATIVE)
			{
				Native();
				return;
			}

			name->setEnabled(true);
			description->setEnabled(true);
			aliases->setEnabled(true);
			protect->setEnabled(true);
			path->setEnabled(true);
			browse->setEnabled(true);
			type->setEnabled(true);
			random->setEnabled(true);
			message->setEnabled(currentType == Type::VIDEO ? false : true);

			ValidatePath(Path(),Random(),currentType);
		}

		void Entry::Native()
		{
			name->setEnabled(false);
			description->setEnabled(false);
			aliases->setEnabled(true);
			protect->setEnabled(false);
			path->setEnabled(false);
			browse->setEnabled(false);
			type->setEnabled(false);
			random->setEnabled(false);
			message->setEnabled(false);
		}

		void Entry::Pulsar()
		{
			name->setEnabled(true);
			description->setEnabled(true);
			aliases->setEnabled(false);
			protect->setEnabled(true);
			path->setEnabled(false);
			browse->setEnabled(false);
			random->setEnabled(false);
			message->setEnabled(false);
		}

		void Entry::Browse()
		{
			QString candidate;
			if (Random())
			{
				candidate=QFileDialog::getExistingDirectory(this, Text::DIALOG_TITLE_DIRECTORY,Filesystem::HomePath().absolutePath(),QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
			}
			else
			{
				if (static_cast<UI::Commands::Type>(type->currentIndex()) == UI::Commands::Type::VIDEO)
					candidate=OpenVideo(this);
				else
					candidate=OpenAudio(this);
			}
			if (!candidate.isEmpty()) path->setText(candidate);
		}

		bool Entry::eventFilter(QObject *object,QEvent *event)
		{
			if (event->type() == QEvent::HoverEnter)
			{
				if (object == name) emit Help("Name of the command. This is the text that must appear after an exclamation mark (!).");
				if (object == description) emit Help("Short description of the command that will appear in in list of commands and showcase rotation.");
				if (object == aliases) emit Help("List of alternate command names.");
				if (object == path || object == browse) emit Help("Location of the media that will be played for command types that use media, such as Video and Announce");
				if (object == type) emit Help("Determines what kind of action is taken in response to a command.\n\nValid values are:\nAnnounce - Displays text while playing an audio file\nVideo - Displays a video");
			}

			return false;
		}

		Dialog::Dialog(const Command::Lookup &commands,QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			entriesFrame(this),
			scrollLayout(&entriesFrame),
			help(this),
			labelFilter("Filter:",this),
			filter(this),
			buttons(this),
			discard(Text::BUTTON_DISCARD,this),
			save(Text::BUTTON_SAVE,this),
			newEntry("&New",this),
			statusBar(this)
		{
			setStyleSheet("QFrame { background-color: palette(window); } QScrollArea, QWidget#commands { background-color: palette(base); } QListWidget:enabled, QTextEdit:enabled { background-color: palette(base); }");

			setModal(true);
			setWindowTitle("Commands Editor");

			QVBoxLayout *mainLayout=new QVBoxLayout(this);
			setLayout(mainLayout);

			QWidget *upperContent=new QWidget(this);
			QHBoxLayout *upperLayout=new QHBoxLayout(upperContent);
			upperContent->setLayout(upperLayout);
			mainLayout->addWidget(upperContent);
			
			QScrollArea *scroll=new QScrollArea(this);
			scroll->setWidgetResizable(true);
			entriesFrame.setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed));
			entriesFrame.setObjectName("commands");
			scroll->setWidget(&entriesFrame);
			scroll->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding));
			upperLayout->addWidget(scroll);

			entriesFrame.setLayout(&scrollLayout);
			PopulateEntries(commands);

			QWidget *rightPane=new QWidget(this);
			QGridLayout *rightLayout=new QGridLayout(rightPane);
			rightPane->setLayout(rightLayout);
			rightLayout->addWidget(&help,0,0,1,2);
			labelFilter.setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed));
			rightLayout->addWidget(&labelFilter,1,0);
			filter.addItems({"All","Native","Dynamic","Pulsar"});
			filter.setCurrentIndex(0);
			connect(&filter,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&Dialog::FilterChanged);
			rightLayout->addWidget(&filter,1,1);
			upperLayout->addWidget(rightPane);

			QWidget *lowerContent=new QWidget(this);
			QHBoxLayout *lowerLayout=new QHBoxLayout(lowerContent);
			lowerContent->setLayout(lowerLayout);
			mainLayout->addWidget(lowerContent);

			buttons.addButton(&save,QDialogButtonBox::AcceptRole);
			buttons.addButton(&discard,QDialogButtonBox::RejectRole);
			buttons.addButton(&newEntry,QDialogButtonBox::ActionRole);
			connect(&buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
			connect(&buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
			connect(this,&QDialog::accepted,this,QOverload<>::of(&Dialog::Save));
			connect(&newEntry,&QPushButton::clicked,this,&UI::Commands::Dialog::Add);
			lowerLayout->addWidget(&buttons);

			setSizeGripEnabled(true);
		}

		void Dialog::PopulateEntries(const Command::Lookup &commands)
		{
			entriesFrame.setUpdatesEnabled(false);
			std::unordered_map<QString,QStringList> aliases;
			for (const Command::Entry &pair : commands)
			{
				const Command &command=pair.second;

				if (command.Parent())
				{
					aliases[command.Parent()->Name()].push_back(command.Name());
					continue;
				}

				Entry *entry=new Entry(command,&entriesFrame);
				connect(entry,&Entry::Help,&help,&QTextEdit::setText);
				entries[entry->Name()]=entry;
				scrollLayout.addWidget(entry);
			}
			for (const std::pair<const QString,QStringList> &pair : aliases) entries.at(pair.first)->Aliases(pair.second);
			entriesFrame.setUpdatesEnabled(true);
		}

		void Dialog::Add()
		{
			QString name=QInputDialog::getText(this,"New Command","Please provide a name for the new command.");
			if (name.isEmpty()) return;
			Entry *entry=new Entry({
				name,
				{},
				CommandType::VIDEO, // has a type, so no need to catch possible exception here
				false,
				true,
				{},
				Command::FileListFilters(CommandType::VIDEO),
				{},
				false
			},this);
			entries[entry->Name()]=entry;
			scrollLayout.addWidget(entry);
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
					CommandType type=entry->Type();
					if (type == CommandType::AUDIO || type == CommandType::VIDEO)
						entry->show();
					else
						entry->hide();
				}
				break;
			case Filter::NATIVE:
				for (std::pair<const QString,Entry*> &pair : entries)
				{
					Entry *entry=pair.second;
					if (entry->Type() == CommandType::NATIVE)
						entry->show();
					else
						entry->hide();
				}
				break;
			case Filter::PULSAR:
				for (std::pair<const QString,Entry*> &pair : entries)
				{
					Entry *entry=pair.second;
					if (entry->Type() == CommandType::PULSAR)
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

			for (const std::pair<const QString,Entry*> &pair : entries)
			{
				Entry *entry=pair.second;

				QString name=entry->Name();
				commands.try_emplace(name,Command{
					entry->Name(),
					entry->Description(),
					entry->Type(),
					entry->Random(),
					entry->Duplicates(),
					entry->Path(),
					entry->Filters(),
					entry->Message(),
					entry->Protected()
				});

				for (const QString &alias : entry->Aliases())
				{
					commands.try_emplace(alias,Command{
						alias,
						&commands.at(name)
					});
				}
			}

			emit Save(commands);
		}
	}

	namespace Options
	{
		namespace Categories
		{
			Category::Category(QWidget *parent,const QString &name) : QFrame(parent),
				layout(this),
				header(this),
				details(nullptr),
				detailsLayout(nullptr)
			{
				setLayout(&layout);
				setFrameShape(QFrame::Box);

				header.setStyleSheet(QString("font-size: %1pt; font-weight: bold; text-align: left;").arg(header.font().pointSizeF()*1.25));
				header.setCursor(Qt::PointingHandCursor);
				header.setFlat(true);
				header.setText(name);
				layout.addWidget(&header);

				details=new QFrame(this);
				details->setLayout(&detailsLayout);
				layout.addWidget(details);

				connect(&header,&QPushButton::clicked,this,&Category::ToggleDetails);
			}

			QLabel* Category::Label(const QString &text)
			{
				QLabel *label=new QLabel(text+":",this);
				label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
				return label;
			}

			void Category::Rows(std::vector<std::vector<QWidget*>> widgets)
			{
				// find longest vector
				int maxColumns=2;
				for (const std::vector<QWidget*> &row : widgets)
				{
					if (row.size() > maxColumns) maxColumns=row.size();
				}

				for (int rowIndex=0; rowIndex < widgets.size(); rowIndex++)
				{
					int columns=widgets[rowIndex].size();
					if (columns < 2) continue;

					QWidget *widget=widgets[rowIndex][0];
					detailsLayout.addWidget(widget,rowIndex,0,1,1);
					widget->installEventFilter(this); // NOTE: this will not fire for labels because they do not fire an enterEvent for mouse hovers

					widget=widgets[rowIndex][1];
					int columnSpan=maxColumns-columns+1; // +1 because spans are not zero-indexed
					detailsLayout.addWidget(widget,rowIndex,1,1,columnSpan);
					widget->installEventFilter(this);

					if (columns > 2)
					{
						for (int columnIndex=2; columnIndex < columns; columnIndex++)
						{
							widget=widgets[rowIndex][columnIndex];
							detailsLayout.addWidget(widget,rowIndex,columnIndex+(columnSpan-1),1,1); // -1 on columnSpan to move back to zero-indexed
							widget->installEventFilter(this);
						}
					}
				}
			}

			void Category::ToggleDetails()
			{
				details->setVisible(!details->isVisible());
			}

			void Category::PickColor(QLineEdit &control)
			{
				QColor color=QColorDialog::getColor(control.text(),this,QStringLiteral("Choose a Color"),QColorDialog::ShowAlphaChannel);
				if (color.isValid()) control.setText(color.name(QColor::HexArgb));
			}

			Channel::Channel(QWidget *parent,Settings settings) : Category(parent,QStringLiteral("Channel")),
				name(this),
				protection(this),
				settings(settings)
			{
				connect(&name,&QLineEdit::textChanged,this,&Channel::ValidateName);

				name.setText(settings.name);
				protection.setChecked(settings.protection);

				Rows({
					{Label(QStringLiteral("Name")),&name},
					{Label(QStringLiteral("Protection")),&protection}
				});
			}

			bool Channel::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &name)
					{
						emit Help(QStringLiteral("Name of the channel Celeste will join on launch"));
						return false;
					}

					if (object == &protection)
					{
						emit Help(QStringLiteral("When the bot is closed, enable protections such as turning on emote-only chat? This is intended to prevent situations such as offline hate raids."));
						return false;
					}
				}

				if (event->type() == QEvent::HoverLeave) emit Help("");
				return false;
			}

			void Channel::ValidateName(const QString &text)
			{
				Require(&name,text.isEmpty());
			}

			void Channel::Save()
			{
				settings.name.Set(name.text());
				settings.protection.Set(protection.isChecked());

				// only need to do this on one of the settings for all of the categories, because it
				// is all the same QSettings object under the hood, so it's saving all of the settings
				settings.name.Save();
			}

			Window::Window(QWidget *parent,Settings settings) : Category(parent,QStringLiteral("Main Window")),
				backgroundColor(this),
				selectBackgroundColor(Text::CHOOSE,this),
				width(this),
				height(this),
				settings(settings)
			{
				connect(&selectBackgroundColor,&QPushButton::clicked,this,[this]() { PickColor(backgroundColor); });

				backgroundColor.setText(settings.backgroundColor);
				QRect desktop=QGuiApplication::primaryScreen()->availableVirtualGeometry();
				width.setRange(1,desktop.width());
				width.setValue(static_cast<QSize>(settings.dimensions).width());
				height.setRange(1,desktop.height());
				height.setValue(static_cast<QSize>(settings.dimensions).height());

				Rows({
					{Label(QStringLiteral("Background Color")),&backgroundColor,&selectBackgroundColor},
					{Label(QStringLiteral("Width")),&width},
					{Label(QStringLiteral("Height")),&height}
				});
			}

			bool Window::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &backgroundColor || object == &selectBackgroundColor)
					{
						emit Help(QStringLiteral("This is the background color of the main window. Note that this is <em>not</em> the background color of individual panes (such as the chat pane)."));
						return false;
					}

					if (object == &width)
					{
						emit Help(QStringLiteral("The width (in pixels) of the application window's contents (the part seen by OBS)"));
						return false;
					}

					if (object == &height)
					{
						emit Help(QStringLiteral("The height (in pixels) of the application window's contents (the part seen by OBS)"));
						return false;
					}
				}

				if (event->type() == QEvent::HoverLeave) emit Help("");
				return false;
			}

			void Window::Save()
			{
				settings.backgroundColor.Set(backgroundColor.text());
				settings.dimensions.Set(QSize{width.value(),height.value()});
			}

			Status::Status(QWidget *parent,Settings settings) : Category(parent,QStringLiteral("Status")),
				font(this),
				fontSize(this),
				selectFont(Text::CHOOSE,this),
				foregroundColor(this),
				previewForegroundColor(this,settings.foregroundColor),
				selectForegroundColor(Text::CHOOSE,this),
				backgroundColor(this),
				previewBackgroundColor(this,settings.backgroundColor),
				selectBackgroundColor(Text::CHOOSE,this),
				settings(settings)
			{
				connect(&font,&QLineEdit::textChanged,this,QOverload<const QString&>::of(&Status::ValidateFont));
				connect(&fontSize,QOverload<const int>::of(&QSpinBox::valueChanged),this,QOverload<const int>::of(&Status::ValidateFont));
				connect(&selectFont,&QPushButton::clicked,this,&Status::PickFont);
				connect(&selectForegroundColor,&QPushButton::clicked,this,&Status::PickForegroundColor);
				connect(&selectBackgroundColor,&QPushButton::clicked,this,&Status::PickBackgroundColor);

				font.setText(settings.font);
				fontSize.setRange(1,std::numeric_limits<short>::max());
				fontSize.setValue(settings.fontSize);
				foregroundColor.setText(settings.foregroundColor);
				backgroundColor.setText(settings.backgroundColor);

				Rows({
					{Label(QStringLiteral("Font")),&font,Label(QStringLiteral("Size")),&fontSize,&selectFont},
					{Label(QStringLiteral("Text Color")),&foregroundColor,&previewForegroundColor,&selectForegroundColor},
					{Label(QStringLiteral("Background Color")),&backgroundColor,&previewBackgroundColor,&selectBackgroundColor},
				});
			}

			void Status::PickFont()
			{
				bool ok=false;
				QFont candidate(font.text(),fontSize.value());
				candidate=QFontDialog::getFont(&ok,candidate,this,Text::DIALOG_TITLE_FONT);
				if (!ok) return;
				font.setText(candidate.family());
				fontSize.setValue(candidate.pointSize());
			}

			void Status::PickForegroundColor()
			{
				PickColor(foregroundColor);
				previewForegroundColor.Set(foregroundColor.text());
			}

			void Status::PickBackgroundColor()
			{
				PickColor(backgroundColor);
				previewBackgroundColor.Set(backgroundColor.text());
			}

			void Status::ValidateFont(const QString &family,const int pointSize)
			{
				QFont candidate(family,pointSize);
				Valid(&font,candidate.exactMatch());
			}

			void Status::ValidateFont(const QString &family)
			{
				ValidateFont(family,fontSize.value());
			}

			void Status::ValidateFont(const int pointSize)
			{
				ValidateFont(font.text(),pointSize);
			}

			bool Status::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &font || object == &fontSize || object == &selectFont) emit Help(QStringLiteral("The font that will be used in the initialization screen when the bot is first launched and connecting to Twitch"));
					if (object == &foregroundColor || object == &selectForegroundColor) emit Help(QStringLiteral("The color of text in the initialization screen that is shown when the bot is first launched and connecting to Twitch"));
					if (object == &backgroundColor || object == &selectBackgroundColor) emit Help(QStringLiteral("The color of the background in the initialization screen that is shown when the bot is first launched and connecting to Twitch"));
				}

				if (event->type() == QEvent::HoverLeave) emit Help("");
				return false;
			}

			void Status::Save()
			{
				settings.font.Set(font.text());
				settings.fontSize.Set(fontSize.value());
				settings.foregroundColor.Set(foregroundColor.text());
				settings.backgroundColor.Set(backgroundColor.text());
			}

			Chat::Chat(QWidget *parent,Settings settings) : Category(parent,QStringLiteral("Chat")),
				font(this),
				fontSize(this),
				selectFont(Text::CHOOSE,this),
				foregroundColor(this),
				previewForegroundColor(this,settings.foregroundColor),
				selectForegroundColor(Text::CHOOSE,this),
				backgroundColor(this),
				previewBackgroundColor(this,settings.backgroundColor),
				selectBackgroundColor(Text::CHOOSE,this),
				statusInterval(this),
				settings(settings)
			{
				connect(&font,&QLineEdit::textChanged,this,QOverload<const QString&>::of(&Chat::ValidateFont));
				connect(&fontSize,QOverload<const int>::of(&QSpinBox::valueChanged),this,QOverload<const int>::of(&Chat::ValidateFont));
				connect(&selectFont,&QPushButton::clicked,this,&Chat::PickFont);
				connect(&selectForegroundColor,&QPushButton::clicked,this,&Chat::PickForegroundColor);
				connect(&selectBackgroundColor,&QPushButton::clicked,this,&Chat::PickBackgroundColor);

				font.setText(settings.font);
				fontSize.setRange(1,std::numeric_limits<short>::max());
				fontSize.setValue(settings.fontSize);
				foregroundColor.setText(settings.foregroundColor);
				backgroundColor.setText(settings.backgroundColor);
				statusInterval.setRange(TimeConvert::Milliseconds(TimeConvert::OneSecond()).count(),std::numeric_limits<int>::max());

				Rows({
					{Label(QStringLiteral("Font")),&font,Label(QStringLiteral("Size")),&fontSize,&selectFont},
					{Label(QStringLiteral("Text Color")),&foregroundColor,&previewForegroundColor,&selectForegroundColor},
					{Label(QStringLiteral("Background Color")),&backgroundColor,&previewBackgroundColor,&selectBackgroundColor},
					{Label(QStringLiteral("Status Duration")),&statusInterval}
				});
			}

			void Chat::PickFont()
			{
				bool ok=false;
				QFont candidate(font.text(),fontSize.value());
				candidate=QFontDialog::getFont(&ok,candidate,this,Text::DIALOG_TITLE_FONT);
				if (!ok) return;
				font.setText(candidate.family());
				fontSize.setValue(candidate.pointSize());
			}

			void Chat::PickForegroundColor()
			{
				PickColor(foregroundColor);
				previewForegroundColor.Set(foregroundColor.text());
			}

			void Chat::PickBackgroundColor()
			{
				PickColor(backgroundColor);
				previewBackgroundColor.Set(backgroundColor.text());
			}

			void Chat::ValidateFont(const QString &family,const int pointSize)
			{
				QFont candidate(family,pointSize);
				Valid(&font,candidate.exactMatch());
			}

			void Chat::ValidateFont(const QString &family)
			{
				ValidateFont(family,fontSize.value());
			}

			void Chat::ValidateFont(const int pointSize)
			{
				ValidateFont(font.text(),pointSize);
			}

			bool Chat::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &font || object == &fontSize || object == &selectFont) emit Help(QStringLiteral("The font that will be used to display chat messages"));
					if (object == &foregroundColor || object == &selectForegroundColor) emit Help(QStringLiteral("The color of chat message text"));
					if (object == &backgroundColor || object == &selectBackgroundColor) emit Help(QStringLiteral("The color of the background behind chat messages"));
					if (object == &statusInterval) emit Help(QStringLiteral("How long (in milliseconds) updates and error messages should display at the bottom of the chat pane"));
				}

				if (event->type() == QEvent::HoverLeave) emit Help("");
				return false;
			}

			void Chat::Save()
			{
				settings.font.Set(font.text());
				settings.fontSize.Set(fontSize.value());
				settings.foregroundColor.Set(foregroundColor.text());
				settings.backgroundColor.Set(backgroundColor.text());
				settings.statusInterval.Set(statusInterval.value());
			}

			Pane::Pane(QWidget *parent,Settings settings) : Category(parent,QStringLiteral("Panes")),
				font(this),
				fontSize(this),
				selectFont(Text::CHOOSE,this),
				foregroundColor(this),
				previewForegroundColor(this,settings.foregroundColor),
				selectForegroundColor(Text::CHOOSE,this),
				backgroundColor(this),
				previewBackgroundColor(this,settings.backgroundColor),
				selectBackgroundColor(Text::CHOOSE,this),
				accentColor(this),
				previewAccentColor(this,settings.accentColor),
				selectAccentColor(Text::CHOOSE,this),
				duration(this),
				settings(settings)
			{
				connect(&font,&QLineEdit::textChanged,this,QOverload<const QString&>::of(&Pane::ValidateFont));
				connect(&fontSize,QOverload<const int>::of(&QSpinBox::valueChanged),this,QOverload<const int>::of(&Pane::ValidateFont));
				connect(&selectFont,&QPushButton::clicked,this,&Pane::PickFont);
				connect(&selectForegroundColor,&QPushButton::clicked,this,&Pane::PickForegroundColor);
				connect(&selectBackgroundColor,&QPushButton::clicked,this,&Pane::PickBackgroundColor);
				connect(&selectAccentColor,&QPushButton::clicked,this,&Pane::PickAccentColor);

				font.setText(settings.font);
				fontSize.setRange(1,std::numeric_limits<short>::max());
				fontSize.setValue(settings.fontSize);
				foregroundColor.setText(settings.foregroundColor);
				backgroundColor.setText(settings.backgroundColor);
				accentColor.setText(settings.accentColor);
				duration.setRange(TimeConvert::Milliseconds(TimeConvert::OneSecond()).count(),std::numeric_limits<int>::max());
				duration.setValue(settings.duration);

				Rows({
					{Label(QStringLiteral("Font")),&font,Label(QStringLiteral("Size")),&fontSize,&selectFont},
					{Label(QStringLiteral("Text Color")),&foregroundColor,&previewForegroundColor,&selectForegroundColor},
					{Label(QStringLiteral("Background Color")),&backgroundColor,&previewBackgroundColor,&selectBackgroundColor},
					{Label(QStringLiteral("Accent Color")),&accentColor,&previewAccentColor,&selectAccentColor},
					{Label(QStringLiteral("Duration")),&duration}
				});
			}

			void Pane::PickFont()
			{
				bool ok=false;
				QFont candidate(font.text(),fontSize.value());
				candidate=QFontDialog::getFont(&ok,candidate,this,Text::DIALOG_TITLE_FONT);
				if (!ok) return;
				font.setText(candidate.family());
				fontSize.setValue(candidate.pointSize());
			}

			void Pane::PickForegroundColor()
			{
				PickColor(foregroundColor);
				previewForegroundColor.Set(foregroundColor.text());
			}

			void Pane::PickBackgroundColor()
			{
				PickColor(backgroundColor);
				previewBackgroundColor.Set(backgroundColor.text());
			}

			void Pane::PickAccentColor()
			{
				PickColor(accentColor);
				previewAccentColor.Set(accentColor.text());
			}

			void Pane::ValidateFont(const QString &family,const int pointSize)
			{
				QFont candidate(family,pointSize);
				Valid(&font,candidate.exactMatch());
			}

			void Pane::ValidateFont(const QString &family)
			{
				ValidateFont(family,fontSize.value());
			}

			void Pane::ValidateFont(const int pointSize)
			{
				ValidateFont(font.text(),pointSize);
			}

			bool Pane::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &font || object == &fontSize || object == &selectFont) emit Help(QStringLiteral("The font that will be used in event panes (such as raid and subscription announcements)"));
					if (object == &foregroundColor || object == &selectForegroundColor) emit Help(QStringLiteral("The color of text in event panes (such as raid and subscription announcements)"));
					if (object == &backgroundColor || object == &selectBackgroundColor) emit Help(QStringLiteral("The color of the background in event panes (such as raid and subscription announcements)"));
					if (object == &accentColor || object == &selectAccentColor) emit Help(QStringLiteral("The color of text effects, such as drop shadows"));
					if (object == &duration) emit Help(QStringLiteral("The amount of time (in milliseconds) that an announcement will display. This only affects announcements that don't have an associated audio or video file, otherwise the duration will be the duration of the associated audio or video."));
				}

				if (event->type() == QEvent::HoverLeave) emit Help("");
				return false;
			}

			void Pane::Save()
			{
				settings.font.Set(font.text());
				settings.fontSize.Set(fontSize.value());
				settings.foregroundColor.Set(foregroundColor.text());
				settings.backgroundColor.Set(backgroundColor.text());
				settings.accentColor.Set(accentColor.text());
				settings.duration.Set(duration.value());
			}

			Music::Music(QWidget *parent,Settings settings) : Category(parent,QStringLiteral("Music")),
				suppressedVolume(this),
				settings(settings)
			{
				suppressedVolume.setRange(0,100);
				suppressedVolume.setSuffix("%");
				suppressedVolume.setValue(settings.suppressedVolume);

				Rows({
					{Label(QStringLiteral("Suppressed Volume")),&suppressedVolume}
				});
			}

			bool Music::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &suppressedVolume) emit Help(QStringLiteral("The volume the music should duck to when another pane is playing audio."));
				}

				if (event->type() == QEvent::HoverLeave) emit Help("");
				return false;
			}

			void Music::Save()
			{
				settings.suppressedVolume.Set(suppressedVolume.value());
			}

			Bot::Bot(QWidget *parent,Settings settings) : Category(parent,QStringLiteral("Bot Core")),
				arrivalSound(this),
				selectArrivalSound(Text::BROWSE,this),
				previewArrivalSound(Text::PREVIEW,this),
				portraitVideo(this),
				selectPortraitVideo(Text::BROWSE,this),
				previewPortraitVideo(Text::PREVIEW,this),
				cheerVideo(this),
				selectCheerVideo(Text::BROWSE,this),
				previewCheerVideo(Text::PREVIEW,this),
				subscriptionSound(this),
				selectSubscriptionSound(Text::BROWSE,this),
				previewSubscriptionSound(Text::PREVIEW,this),
				raidSound(this),
				selectRaidSound(Text::BROWSE,this),
				previewRaidSound(Text::PREVIEW,this),
				inactivityCooldown(this),
				helpCooldown(this),
				textWallSound(this),
				selectTextWallSound(Text::BROWSE,this),
				previewTextWallSound(Text::PREVIEW,this),
				textWallThreshold(this),
				settings(settings)
			{
				connect(&arrivalSound,&QLineEdit::textChanged,this,&Bot::ValidateArrivalSound);
				connect(&selectArrivalSound,&QPushButton::clicked,this,&Bot::OpenArrivalSound);
				connect(&previewArrivalSound,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayArrivalSound));
				connect(&portraitVideo,&QLineEdit::textChanged,this,&Bot::ValidatePortraitVideo);
				connect(&selectPortraitVideo,&QPushButton::clicked,this,&Bot::OpenPortraitVideo);
				connect(&previewPortraitVideo,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayPortraitVideo));
				connect(&cheerVideo,&QLineEdit::textChanged,this,&Bot::ValidateCheerVideo);
				connect(&selectCheerVideo,&QPushButton::clicked,this,&Bot::OpenCheerVideo);
				connect(&previewCheerVideo,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayCheerVideo));
				connect(&subscriptionSound,&QLineEdit::textChanged,this,&Bot::ValidateSubscriptionSound);
				connect(&selectSubscriptionSound,&QPushButton::clicked,this,&Bot::OpenSubscriptionSound);
				connect(&previewSubscriptionSound,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlaySubscriptionSound));
				connect(&raidSound,&QLineEdit::textChanged,this,&Bot::ValidateRaidSound);
				connect(&selectRaidSound,&QPushButton::clicked,this,&Bot::OpenRaidSound);
				connect(&previewRaidSound,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayRaidSound));
				connect(&textWallSound,&QLineEdit::textChanged,this,&Bot::ValidateTextWallSound);
				connect(&selectTextWallSound,&QPushButton::clicked,this,&Bot::OpenTextWallSound);
				connect(&previewTextWallSound,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayTextWallSound));

				arrivalSound.setText(settings.arrivalSound);
				portraitVideo.setText(settings.portraitVideo);
				cheerVideo.setText(settings.cheerVideo);
				subscriptionSound.setText(settings.subscriptionSound);
				raidSound.setText(settings.raidSound);
				inactivityCooldown.setRange(TimeConvert::Milliseconds(TimeConvert::OneSecond()).count(),std::numeric_limits<int>::max());
				inactivityCooldown.setValue(settings.inactivityCooldown);
				helpCooldown.setRange(TimeConvert::Milliseconds(TimeConvert::OneSecond()).count(),std::numeric_limits<int>::max());
				helpCooldown.setValue(settings.helpCooldown);
				textWallThreshold.setRange(1,std::numeric_limits<int>::max());
				textWallThreshold.setValue(settings.textWallThreshold);
				textWallSound.setText(settings.textWallSound);

				Rows({
					{Label(QStringLiteral("Arrival Announcement Audio")),&arrivalSound,&selectArrivalSound,&previewArrivalSound},
					{Label(QStringLiteral("Portrait (Ping) Video")),&portraitVideo,&selectPortraitVideo,&previewPortraitVideo},
					{Label(QStringLiteral("Cheer (Bits) Video")),&cheerVideo,&selectCheerVideo,&previewCheerVideo},
					{Label(QStringLiteral("Subscription Announcement")),&subscriptionSound,&selectSubscriptionSound,&previewSubscriptionSound},
					{Label(QStringLiteral("Raid Announcement")),&raidSound,&selectRaidSound,&previewRaidSound},
					{Label(QStringLiteral("Inactivity Cooldown")),&inactivityCooldown},
					{Label(QStringLiteral("Help Cooldown")),&helpCooldown},
					{Label(QStringLiteral("Wall-of-Text Sound")),&textWallSound,&selectTextWallSound,&previewTextWallSound,Label(QStringLiteral("Threshold")),&textWallThreshold}
				});
			}

			bool Bot::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &arrivalSound || object == &selectArrivalSound || object == &previewArrivalSound) emit Help(QStringLiteral("This is the sound that plays each time someone speak in chat for the first time. This can be a single audio file (mp3), or a folder of audio files. If it's a folder, a random audio file will be chosen from that folder each time."));
					if (object == &portraitVideo || object == &selectPortraitVideo || object == &previewPortraitVideo) emit Help(QStringLiteral("Every so often, Twitch will send a request to the bot asking if it's still connected (ping). This is a video that can play each time that happens."));
					if (object == &cheerVideo || object == &selectCheerVideo || object == &previewCheerVideo) emit Help(QStringLiteral("A video (mp4) that plays when a chatter cheers bits."));
					if (object == &subscriptionSound || object == &selectSubscriptionSound || object == &previewSubscriptionSound) emit Help(QStringLiteral("This is the sound that plays when a chatter subscribes to the channel."));
					if (object == &raidSound || object == &selectRaidSound || object == &previewRaidSound) emit Help(QStringLiteral("This is the sound that plays when another streamer raids the channel."));
					if (object == &inactivityCooldown) emit Help(QStringLiteral(R"(This is the amount of time (in milliseconds) that must pass without any chat messages before Celeste plays a "roast" video)"));
					if (object == &helpCooldown) emit Help(QStringLiteral(R"(This is the amount of time (in milliseconds) between "help" message. A help message is an explanation of a single, randomly chosen command.)"));
					if (object == &textWallThreshold || object == &textWallSound || object == &selectTextWallSound || object == &previewTextWallSound) emit Help(QStringLiteral("This is the sound that plays when a user spams chat with a super long message. The threshold is the number of characters the message needs to be to trigger the sound."));
				}

				if (event->type() == QEvent::HoverLeave) emit Help("");
				return false;
			}

			void Bot::OpenArrivalSound()
			{
				QString candidate=OpenAudio(this,arrivalSound.text());
				if (!candidate.isEmpty()) arrivalSound.setText(candidate);
			}

			void Bot::PlayArrivalSound()
			{
				const QString filename=arrivalSound.text();
				emit PlayArrivalSound(qApp->applicationName(),QImage{Resources::CELESTE},QFileInfo(filename).isDir() ? File::List(filename).Random() : filename);
			}

			void Bot::OpenPortraitVideo()
			{
				QString candidate=OpenVideo(this,portraitVideo.text());
				if (!candidate.isEmpty()) portraitVideo.setText(candidate);
			}

			void Bot::PlayPortraitVideo()
			{
				emit PlayPortraitVideo(portraitVideo.text());
			}

			void Bot::OpenCheerVideo()
			{
				QString candidate=OpenVideo(this,cheerVideo.text());
				if (!candidate.isEmpty()) cheerVideo.setText(candidate);
			}

			void Bot::PlayCheerVideo()
			{
				emit PlayCheerVideo(qApp->applicationName(),100,"Hype!",cheerVideo.text());
			}

			void Bot::OpenSubscriptionSound()
			{
				QString candidate=OpenAudio(this,subscriptionSound.text());
				if (!candidate.isEmpty()) subscriptionSound.setText(candidate);
			}

			void Bot::PlaySubscriptionSound()
			{
				emit PlaySubscriptionSound(qApp->applicationName(),subscriptionSound.text());
			}

			void Bot::OpenRaidSound()
			{
				QString candidate=OpenAudio(this,raidSound.text());
				if (!candidate.isEmpty()) raidSound.setText(candidate);
			}

			void Bot::PlayRaidSound()
			{
				emit PlayRaidSound(qApp->applicationName(),100,raidSound.text());
			}

			void Bot::OpenTextWallSound()
			{
				QString candidate=OpenAudio(this,textWallSound.text());
				if (!candidate.isEmpty()) textWallSound.setText(candidate);
			}

			void Bot::PlayTextWallSound()
			{
				QString message("Celeste ");
				message=message.repeated(textWallThreshold.value()/message.size());
				emit PlayTextWallSound(message,textWallSound.text());
			}

			void Bot::ValidateArrivalSound(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() && (candidate.isDir() || candidate.suffix() == Text::FILE_TYPE_AUDIO);
				Valid(&arrivalSound,valid);
				previewArrivalSound.setEnabled(valid);
			}

			void Bot::ValidatePortraitVideo(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() && candidate.suffix() == Text::FILE_TYPE_VIDEO;
				Valid(&portraitVideo,valid);
				previewPortraitVideo.setEnabled(valid);
			}

			void Bot::ValidateCheerVideo(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() && candidate.suffix() == Text::FILE_TYPE_VIDEO;
				Valid(&cheerVideo,valid);
				previewCheerVideo.setEnabled(valid);
			}

			void Bot::ValidateSubscriptionSound(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() && candidate.suffix() == Text::FILE_TYPE_AUDIO;
				Valid(&subscriptionSound,valid);
				previewSubscriptionSound.setEnabled(valid);
			}

			void Bot::ValidateRaidSound(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() && candidate.suffix() == Text::FILE_TYPE_AUDIO;
				Valid(&raidSound,valid);
				previewRaidSound.setEnabled(valid);
			}

			void Bot::ValidateTextWallSound(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() &&candidate.suffix() == Text::FILE_TYPE_AUDIO;
				Valid(&textWallSound,valid);
				textWallSound.setEnabled(valid);
			}

			void Bot::Save()
			{
				settings.arrivalSound.Set(arrivalSound.text());
				settings.portraitVideo.Set(portraitVideo.text());
				settings.cheerVideo.Set(cheerVideo.text());
				settings.subscriptionSound.Set(subscriptionSound.text());
				settings.raidSound.Set(raidSound.text());
				settings.inactivityCooldown.Set(inactivityCooldown.value());
				settings.helpCooldown.Set(helpCooldown.value());
				settings.textWallThreshold.Set(textWallThreshold.value());
				settings.textWallSound.Set(textWallSound.text());
			}

			Log::Log(QWidget *parent,Settings settings) : Category(parent,QStringLiteral("Logging")),
				directory(this),
				selectDirectory(Text::BROWSE,this),
				settings(settings)
			{
				connect(&directory,&QLineEdit::textChanged,this,&Log::ValidateDirectory);
				connect(&selectDirectory,&QPushButton::clicked,this,&Log::OpenDirectory);

				directory.setText(settings.directory);

				Rows({
					{Label(QStringLiteral("Folder")),&directory,&selectDirectory}
				});
			}

			void Log::OpenDirectory()
			{
				const QString initialPath=directory.text();
				QString candidate=QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this,Text::DIALOG_TITLE_DIRECTORY,initialPath.isEmpty() ? Filesystem::DataPath().absolutePath() : initialPath));
				if (!candidate.isEmpty()) directory.setText(candidate);
			}

			void Log::ValidateDirectory(const QString &path)
			{
				Valid(&directory,QDir(path).exists());
			}

			bool Log::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &directory || object == &selectDirectory) emit Help(QStringLiteral("The folder where the bot will store log files, one log file per day. The bot logs to the file for the day the bot was launched."));
				}

				if (event->type() == QEvent::HoverLeave) emit Help("");
				return false;
			}

			void Log::Save()
			{
				settings.directory.Set(directory.text());
			}

			Security::Security(QWidget *parent,::Security &settings) : Category(parent,QStringLiteral("Security")),
				administrator(this),
				clientID(this),
				clientSecret(this),
				token(this),
				serverToken(this),
				callbackURL(this),
				permissions(this),
				selectPermissions(Text::CHOOSE,this),
				settings(settings)
			{
				details->setVisible(false);

				administrator.setText(settings.Administrator());
				clientID.setText(settings.ClientID());
				clientID.setEchoMode(QLineEdit::Password);
				clientSecret.setText(settings.ClientSecret());
				clientSecret.setEchoMode(QLineEdit::Password);
				token.setText(settings.OAuthToken());
				token.setEchoMode(QLineEdit::Password);
				serverToken.setText(settings.ServerToken());
				serverToken.setEchoMode(QLineEdit::Password);
				permissions.setText(settings.Scope());

				callbackURL.setText(settings.CallbackURL());
				callbackURL.setInputMethodHints(Qt::ImhUrlCharactersOnly);

				connect(&selectPermissions,&QPushButton::clicked,this,&Security::SelectPermissions);
				connect(&callbackURL,&QLineEdit::textChanged,this,&Security::ValidateURL);

				Rows({
					{Label(QStringLiteral("Administrator (Broascaster)")),&administrator},
					{Label(QStringLiteral("Client ID")),&clientID},
					{Label(QStringLiteral("Client Secret")),&clientSecret},
					{Label(QStringLiteral("OAuth Token")),&token},
					{Label(QStringLiteral("Server Token")),&serverToken},
					{Label(QStringLiteral("Callback URL")),&callbackURL},
					{Label(QStringLiteral("Permissions")),&permissions,&selectPermissions}
				});
			}

			bool Security::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &administrator) emit Help(QStringLiteral("Twitch user name of the broadcaster."));
					if (object == &clientID) emit Help(QStringLiteral("Client ID from Twitch developer console."));
					if (object == &clientSecret) emit Help(QStringLiteral("Client Secret from Twitch developer console."));
					if (object == &token) emit Help(QStringLiteral(R"(OAuth token obtained from Twitch authorization process (usually automatic, but can be manually obtained and entered). This is for "Authorization code grant flow" for Celeste's main API calls.)"));
					if (object == &serverToken) emit Help(QStringLiteral(R"(Server token obtained from Twitch authorization process. This is for "Client credentials grant flow" for Celete's EventSub web hook support.)"));
					if (object == &callbackURL) emit Help(QStringLiteral("The URL Twitch will contact with an OAuth token (or error message)."));
					if (object == &permissions || object == &selectPermissions) emit Help(QStringLiteral(R"(The list of permissions (Twitch refers to as "scopes") the bot will require.)"));
				}

				if (event->type() == QEvent::HoverLeave) emit Help("");
				return false;
			}

			void Security::SelectPermissions()
			{
				UI::Security::Scopes scopes(this);
				if (scopes.exec()) permissions.setText(scopes().join(" "));
			}

			void Security::Save()
			{
				settings.Administrator().Set(administrator.text());
				settings.ClientID().Set(clientID.text());
				settings.ClientSecret().Set(clientSecret.text());
				settings.OAuthToken().Set(token.text());
				settings.ServerToken().Set(serverToken.text());
				settings.CallbackURL().Set(callbackURL.text());
				settings.Scope().Set(permissions.text());
			}

			void Security::ValidateURL(const QString &text)
			{
				Valid(&callbackURL,QUrl(text).isValid());
			}
		}

		Dialog::Dialog(QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			entriesFrame(this),
			help(this),
			buttons(this),
			discard(Text::BUTTON_DISCARD,this),
			save(Text::BUTTON_SAVE,this),
			apply(Text::BUTTON_APPLY,this),
			scrollLayout(nullptr)
		{
			setStyleSheet("QFrame { background-color: palette(window); } QScrollArea, QWidget#options { background-color: palette(base); }");

			setModal(true);
			setWindowTitle("Options");

			QVBoxLayout *mainLayout=new QVBoxLayout(this);
			setLayout(mainLayout);

			QWidget *upperContent=new QWidget(this);
			QHBoxLayout *upperLayout=new QHBoxLayout(upperContent);
			upperContent->setLayout(upperLayout);
			mainLayout->addWidget(upperContent);
			
			QScrollArea *scroll=new QScrollArea(this);
			scroll->setWidgetResizable(true);
			entriesFrame.setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed));
			entriesFrame.setObjectName("options");
			scroll->setWidget(&entriesFrame);
			scroll->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding));
			upperLayout->addWidget(scroll);

			scrollLayout=new QVBoxLayout(&entriesFrame);
			scrollLayout->setAlignment(Qt::AlignBottom);
			entriesFrame.setLayout(scrollLayout);

			QWidget *rightPane=new QWidget(this);
			QGridLayout *rightLayout=new QGridLayout(rightPane);
			rightPane->setLayout(rightLayout);
			rightLayout->addWidget(&help,0,0,1,2);
			upperLayout->addWidget(rightPane);

			QWidget *lowerContent=new QWidget(this);
			QHBoxLayout *lowerLayout=new QHBoxLayout(lowerContent);
			lowerContent->setLayout(lowerLayout);
			mainLayout->addWidget(lowerContent);

			buttons.addButton(&save,QDialogButtonBox::AcceptRole);
			buttons.addButton(&apply,QDialogButtonBox::ApplyRole);
			buttons.addButton(&discard,QDialogButtonBox::RejectRole);
			connect(&buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
			connect(&buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
			connect(this,&QDialog::accepted,this,QOverload<>::of(&Dialog::Save));
			connect(&apply,&QPushButton::clicked,this,QOverload<>::of(&Dialog::Save));
			lowerLayout->addWidget(&buttons);

			setSizeGripEnabled(true);
		}

		void Dialog::AddCategory(Categories::Category *category)
		{
			scrollLayout->addWidget(category);
			connect(category,&Categories::Category::Help,&help,&QTextEdit::setText);
			categories.push_back(category);
		}

		void Dialog::Save()
		{
			for (Categories::Category *category : categories) category->Save();
			emit Refresh();
		}
	}

	namespace Metrics
	{
		Dialog::Dialog(QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			layout(this),
			users(this)
		{
			layout.addWidget(&users);
			setModal(false);
			setSizeGripEnabled(true);
		}

		void Dialog::Joined(const QString &user)
		{
			QListWidgetItem *item=new QListWidgetItem(user);
			users.addItem(item);
			item->setForeground(palette().mid());
			UpdateTitle();
		}

		void Dialog::Acknowledged(const QString &name)
		{
			QList<QListWidgetItem*> items=users.findItems(name,Qt::MatchExactly);
			if (items.isEmpty()) return;
			QListWidgetItem *item=items.at(0);
			item->setForeground(palette().text());
		}

		void Dialog::Parted(const QString &user)
		{
			QList<QListWidgetItem*> items=users.findItems(user,Qt::MatchExactly);
			for (QListWidgetItem *item : items) delete users.takeItem(users.row(item));
			UpdateTitle();
		}

		void Dialog::UpdateTitle()
		{
			setWindowTitle(QStringLiteral("Metrics (%1)").arg(StringConvert::Integer(users.count())));
		}
	}

	namespace VibePlaylist
	{
		const int Dialog::COLUMN_COUNT=4;

		Dialog::Dialog(const File::List &files,QWidget *parent) : QDialog(parent),
			layout(this),
			list(0,COLUMN_COUNT,this),
			buttons(this),
			add(Text::BUTTON_ADD,this),
			remove(Text::BUTTON_REMOVE,this),
			discard(Text::BUTTON_DISCARD,this),
			save(Text::BUTTON_SAVE,this),
			reader(this),
			files(files)
		{
			setLayout(&layout);

			list.setHorizontalHeaderLabels({"Artist","Album","Title","Path"});
			list.setSelectionBehavior(QAbstractItemView::SelectRows);
			list.setSelectionMode(QAbstractItemView::ExtendedSelection);
			const QStringList paths=files();
			if (!paths.empty())
			{
				initialAddFilesPath={paths.first()};
				Add(paths,false);
			}
			else
			{
				initialAddFilesPath={Filesystem::HomePath().absolutePath()};
			}
			list.setSortingEnabled(true);
			layout.addWidget(&list);

			buttons.addButton(&save,QDialogButtonBox::AcceptRole);
			buttons.addButton(&discard,QDialogButtonBox::RejectRole);
			buttons.addButton(&add,QDialogButtonBox::ActionRole);
			buttons.addButton(&remove,QDialogButtonBox::ActionRole);
			connect(&buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
			connect(&buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
			connect(this,&QDialog::accepted,this,QOverload<>::of(&Dialog::Save));
			connect(&add,&QPushButton::clicked,this,QOverload<>::of(&Dialog::Add));
			connect(&remove,&QPushButton::clicked,this,&Dialog::Remove);
			layout.addWidget(&buttons);

			setSizeGripEnabled(true);
		}

		void Dialog::Save()
		{
			QStringList paths;
			for (int row=0; row < list.rowCount(); row++) paths.append(list.item(row,COLUMN_COUNT-1)->text());
			emit Save({paths});
		}

		void Dialog::Add()
		{
			const QStringList paths=QFileDialog::getOpenFileNames(this,Text::DIALOG_TITLE_FILE,initialAddFilesPath.absolutePath(),QString("Songs (*.%1)").arg(Text::FILE_TYPE_AUDIO));
			if (paths.isEmpty()) return;
			initialAddFilesPath={paths.front()};
			Add(paths,true);
		}

		void Dialog::Add(const QString &path)
		{
			Music::ID3::Tag tag=Music::ID3::Tag{path};
			auto artist=tag.Artist();
			auto title=tag.Title();
			if (!title || !artist) return;
			auto album=tag.AlbumTitle();
			list.insertRow(list.rowCount());
			int row=list.rowCount()-1;
			list.setItem(row,0,ReadOnlyItem(*artist));
			list.setItem(row,1,ReadOnlyItem(album ? *album : QString{}));
			list.setItem(row,2,ReadOnlyItem(*title));
			list.setItem(row,3,ReadOnlyItem(path));
		}

		void Dialog::Add(const QStringList &paths,bool failurePrompt)
		{
			list.setSortingEnabled(false);
			QStringList failed;
			for (const QString &file : paths)
			{
				try
				{
					Add(file);
				}
				catch (const std::runtime_error &exception)
				{
					failed.append(QString{"%1: %2"}.arg(file,exception.what()));
				}
			}
			if (failurePrompt && !failed.isEmpty())
			{
				QString message=failed.join('\n');
				QMessageBox{QMessageBox::Warning,"Failed to add files",failed.join('\n'),QMessageBox::Ok}.exec();
			}
			list.setSortingEnabled(true);
		}

		void Dialog::Remove()
		{
			QList<QTableWidgetItem*> items=list.selectedItems();
			for (QList<QTableWidgetItem*>::iterator candidate=items.begin(); candidate != items.end(); candidate+=COLUMN_COUNT)
			{
				list.removeRow(list.row(*candidate));
			}
		}

		QTableWidgetItem* Dialog::ReadOnlyItem(const QString &text)
		{
			QTableWidgetItem *item=new QTableWidgetItem(text);
			item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			return item;
		}
	}
}
