#include <QScrollBar>
#include <QShowEvent>
#include <QEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QVideoWidget>
#include <QScreen>
#include <QMessageBox>
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
		return QDir::toNativeSeparators(QFileDialog::getOpenFileName(parent,Text::DIALOG_TITLE_FILE,initialPath.isEmpty() ? Text::DIRECTORY_HOME : initialPath,QString("Videos (*.%1)").arg(Text::FILE_TYPE_VIDEO)));
	}

	QString OpenAudio(QWidget *parent,QString initialPath)
	{
		return QDir::toNativeSeparators(QFileDialog::getOpenFileName(parent,Text::DIALOG_TITLE_FILE,initialPath.isEmpty() ? Text::DIRECTORY_HOME : initialPath,QString("Audios (*.%1)").arg(Text::FILE_TYPE_AUDIO)));
	}

	Help::Help(QWidget *parent) : QTextEdit(parent)
	{
		setEnabled(false);
		setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding));
		setStyleSheet(QStringLiteral("border: none; color: palette(window-text);"));
	}

	namespace Commands
	{
		Aliases::Aliases(QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			layout(this),
			list(this),
			name(this),
			add("Add",this),
			remove("Remove",this)
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
		}

		Entry::Entry(QWidget *parent) : layout(this),
			details(this),
			header(this),
			name(this),
			description(this),
			openAliases("Aliases",this),
			path(this),
			browse(Text::BROWSE,this),
			type(this),
			random("Random",this),
			protect("Protect",this),
			message(this),
			aliases(this)
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

			QGridLayout *detailsLayout=new QGridLayout(&details);
			details.setLayout(detailsLayout);
			details.setFrameShape(QFrame::NoFrame);
			name.setPlaceholderText("Command Name");
			detailsLayout->addWidget(&name,0,0);
			detailsLayout->addWidget(&openAliases,0,1);
			detailsLayout->addWidget(&protect,0,2);
			description.setPlaceholderText("Command Description");
			detailsLayout->addWidget(&description,1,0,1,3);
			detailsLayout->addWidget(&path,2,0,1,2);
			detailsLayout->addWidget(&browse,2,2,1,1);
			type.addItems({
				"Video",
				"Announce",
				"Pulsar"
			});
			type.setPlaceholderText("Native");
			detailsLayout->addWidget(&type,3,0,1,2);
			detailsLayout->addWidget(&random,3,2,1,1);
			detailsLayout->addWidget(&message,4,0,1,3);
			frameLayout->addWidget(&details);

			connect(&header,&QPushButton::clicked,this,&Entry::ToggleFold);
			connect(&name,&QLineEdit::textChanged,this,&Entry::ValidateName);
			connect(&name,&QLineEdit::textChanged,this,QOverload<const QString&>::of(&Entry::UpdateHeader));
			connect(&description,&QLineEdit::textChanged,this,&Entry::ValidateDescription);
			connect(&openAliases,&QPushButton::clicked,&aliases,&QDialog::show);
			connect(&path,&QLineEdit::textChanged,this,QOverload<const QString&>::of(&Entry::ValidatePath));
			connect(&random,&QCheckBox::stateChanged,this,QOverload<const int>::of(&Entry::ValidatePath));
			connect(&message,&QTextEdit::textChanged,this,&Entry::ValidateMessage);
			connect(&type,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&Entry::TypeChanged);
			connect(&browse,&QPushButton::clicked,this,&Entry::Browse);
			connect(&aliases,&Aliases::Finished,this,QOverload<>::of(&Entry::UpdateHeader));

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
			return aliases();
		}

		void Entry::Aliases(const QStringList &names)
		{
			aliases.Populate(names);
			UpdateHeader();
		}

		QString Entry::Path() const
		{
			return path.text();
		}

		enum Type Entry::Type() const
		{
			return static_cast<enum Type>(type.currentIndex());
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
			details.setVisible(!details.isVisible());
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
			ValidatePath(text,Random(),Type());
		}

		void Entry::ValidatePath(const int state)
		{
			ValidatePath(Path(),state == Qt::Checked ? true : false,Type());
		}

		void Entry::ValidatePath(const QString &text,bool random,const enum Type type)
		{
			QFileInfo candidate(text);

			if (!candidate.exists())
			{
				Valid(&path,false);
				return;
			}

			if (random)
			{
				Valid(&path,candidate.isDir());
				return;
			}
			else
			{
				if (candidate.isDir())
				{
					Valid(&path,false);
					return;
				}

				const QString extension=candidate.suffix();
				switch (type)
				{
				case Type::VIDEO:
					Valid(&path,extension == Text::FILE_TYPE_VIDEO);
					break;
				case Type::AUDIO:
					Valid(&path,extension == Text::FILE_TYPE_AUDIO);
					break;
				default:
					Valid(&path,false);
					break;
				}
			}
		}

		void Entry::ValidateMessage()
		{
			if (Type() == Type::AUDIO) Require(&message,Message().isEmpty());
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

			name.setEnabled(true);
			description.setEnabled(true);
			aliases.setEnabled(true);
			protect.setEnabled(true);
			path.setEnabled(true);
			browse.setEnabled(true);
			type.setEnabled(true);
			random.setEnabled(true);
			message.setEnabled(currentType == Type::VIDEO ? false : true);

			ValidatePath(Path(),Random(),currentType);
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
			QString candidate;
			if (Random())
			{
				candidate=QFileDialog::getExistingDirectory(this, Text::DIALOG_TITLE_DIRECTORY,Text::DIRECTORY_HOME,QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
			}
			else
			{
				if (Type() == Type::VIDEO)
					candidate=OpenVideo(this);
				else
					candidate=OpenAudio(this);
			}
			if (!candidate.isEmpty()) path.setText(candidate);
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
			discard(Text::BUTTON_DISCARD,this),
			save(Text::BUTTON_SAVE,this),
			newEntry("&New",this)
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

			QVBoxLayout *scrollLayout=new QVBoxLayout(&entriesFrame);
			entriesFrame.setLayout(scrollLayout);
			PopulateEntries(commands,scrollLayout);

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
				connect(entry,&Entry::Help,&help,&QTextEdit::setText);
				entries[entry->Name()]=entry;
				layout->addWidget(entry);
			}
			for (const std::pair<QString,QStringList> &pair : aliases) entries.at(pair.first)->Aliases(pair.second);
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

	namespace Options
	{
		namespace Categories
		{
			Category::Category(QWidget *parent,const QString &name) : QFrame(parent),
				layout(this),
				header(this),
				details(this),
				detailsLayout(&details)
			{
				setLayout(&layout);
				setFrameShape(QFrame::Box);

				header.setStyleSheet(QString("font-size: %1pt; font-weight: bold; text-align: left;").arg(header.font().pointSizeF()*1.25));
				header.setCursor(Qt::PointingHandCursor);
				header.setFlat(true);
				header.setText(name);
				layout.addWidget(&header);
				layout.addWidget(&details);

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
				static const int MINIMUM_COLUMNS=2;
				int maxColumns=2;
				for (const std::vector<QWidget*> &row : widgets)
				{
					if (row.size() < MINIMUM_COLUMNS) throw std::logic_error("A row requires at least one label and one input field");
					if (row.size() > maxColumns) maxColumns=row.size();
				}

				for (int rowIndex=0; rowIndex < widgets.size(); rowIndex++)
				{					
					for (int columnIndex=0; columnIndex < widgets[rowIndex].size(); columnIndex++)
					{
						// if we're on the last column, calculate how far it is from the number of columns in the longest row
						// this determines our column span
						int columnSpan=1;
						if (columnIndex == widgets[rowIndex].size()-1) columnSpan=maxColumns-columnIndex;
						detailsLayout.addWidget(widgets[rowIndex][columnIndex],rowIndex,columnIndex,1,columnSpan);
					}
				}
			}

			void Category::ToggleDetails()
			{
				details.setVisible(!details.isVisible());
			}

			void Category::PickColor(QLineEdit &control)
			{
				QColor color=QColorDialog::getColor(control.text(),this,QStringLiteral("Choose a Color"),QColorDialog::ShowAlphaChannel);
				if (color.isValid()) control.setText(color.name(QColor::HexArgb));
			}

			Channel::Channel(QWidget *parent,Settings settings) : Category(parent,QStringLiteral("Channel")),
				name(this),
				protection(this)
			{
				connect(&name,&QLineEdit::textChanged,this,&Channel::ValidateName);

				name.setText(settings.name);
				protection.setChecked(settings.protection);

				Rows({
					{Label(QStringLiteral("Name")),&name},
					{Label(QStringLiteral("Protection")),&protection}
				});

				name.installEventFilter(this);
				protection.installEventFilter(this);
			}

			bool Channel::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &name) emit Help(QStringLiteral("Name of the channel Celeste will join on launch"));
					if (object == &protection) emit Help(QStringLiteral("When the bot is closed, enable protections such as turning on emote-only chat? This is intended to prevent situations such as offline hate raids."));
				}

				return false;
			}

			void Channel::ValidateName(const QString &text)
			{
				Require(&name,text.isEmpty());
			}

			Window::Window(QWidget *parent,Settings settings) : Category(parent,QStringLiteral("Main Window")),
				backgroundColor(this),
				selectBackgroundColor(Text::CHOOSE,this),
				accentColor(this),
				selectAccentColor(Text::CHOOSE,this),
				width(this),
				height(this)
			{
				connect(&selectBackgroundColor,&QPushButton::clicked,this,[this]() { PickColor(backgroundColor); });
				connect(&selectAccentColor,&QPushButton::clicked,this,[this]() { PickColor(accentColor); });

				backgroundColor.setText(settings.backgroundColor);
				accentColor.setText(settings.accentColor);
				QRect desktop=QGuiApplication::primaryScreen()->availableVirtualGeometry();
				width.setRange(1,desktop.width());
				width.setValue(static_cast<QSize>(settings.dimensions).width());
				height.setRange(1,desktop.height());
				height.setValue(static_cast<QSize>(settings.dimensions).height());

				Rows({
					{Label(QStringLiteral("Background Color")),&backgroundColor,&selectBackgroundColor},
					{Label(QStringLiteral("Accent Color")),&accentColor,&selectAccentColor},
					{Label(QStringLiteral("Width")),&width},
					{Label(QStringLiteral("Height")),&height}
				});

				backgroundColor.installEventFilter(this);
				accentColor.installEventFilter(this);
				width.installEventFilter(this);
				height.installEventFilter(this);
			}

			bool Window::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &backgroundColor) emit Help(QStringLiteral("This is the background color of the main window. Note that this is <em>not</em> the background color of individual panes (such as the chat pane)."));
					if (object == &accentColor) emit Help(QStringLiteral("The color of text effects, such as drop shadows"));
					if (object == &width) emit Help(QStringLiteral("The width (in pixels) of the application window's contents (the part seen by OBS)"));
					if (object == &height) emit Help(QStringLiteral("The height (in pixels) of the application window's contents (the part seen by OBS)"));
				}

				return false;
			}

			Bot::Bot(QWidget *parent,Settings settings) : Category(parent,QStringLiteral("Bot Core")),
				arrivalSound(this),
				selectArrivalSound(Text::BROWSE,this),
				previewArrivalSound(Text::PREVIEW,this),
				portraitVideo(this),
				selectPortraitVideo(Text::BROWSE,this),
				previewPortraitVideo(Text::PREVIEW,this)
			{
				connect(&arrivalSound,&QLineEdit::textChanged,this,&Bot::ValidateArrivalSound);
				connect(&selectArrivalSound,&QPushButton::clicked,this,&Bot::OpenArrivalSound);
				connect(&previewArrivalSound,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayArrivalSound));
				connect(&portraitVideo,&QLineEdit::textChanged,this,&Bot::ValidatePortraitVideo);
				connect(&selectPortraitVideo,&QPushButton::clicked,this,&Bot::OpenPortraitVideo);
				connect(&previewPortraitVideo,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayPortraitVideo));

				arrivalSound.setText(settings.arrivalSound);
				portraitVideo.setText(settings.portraitVideo);

				Rows({
					{Label(QStringLiteral("Arrival Announcement Audio")),&arrivalSound,&selectArrivalSound,&previewArrivalSound},
					{Label(QStringLiteral("Portrait (Ping) Video")),&portraitVideo,&selectPortraitVideo,&previewPortraitVideo}
				});

				arrivalSound.installEventFilter(this);
				portraitVideo.installEventFilter(this);
			}

			bool Bot::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &arrivalSound) emit Help(QStringLiteral("This is the sound that plays each time someone speak in chat for the first time. This can be a single audio file (mp3), or a folder of audio files. If it's a folder, a random audio file will be chosen from that folder each time."));
					if (object == &portraitVideo) emit Help(QStringLiteral("Every so often, Twitch will send a request to the bot asking if it's still connected (ping). This is a video that can play each time that happens."));
				}

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
				emit PlayArrivalSound(QStringLiteral("Celeste"),QImage(),QFileInfo(filename).isDir() ? File::List(filename).Random() : filename);
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
		}

		Dialog::Dialog(QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			entriesFrame(this),
			help(this),
			buttons(this),
			discard(Text::BUTTON_DISCARD,this),
			save(Text::BUTTON_SAVE,this),
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
			buttons.addButton(&discard,QDialogButtonBox::RejectRole);
			connect(&buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
			connect(&buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
			//connect(this,&QDialog::accepted,this,QOverload<>::of(&Dialog::Save));
			lowerLayout->addWidget(&buttons);

			setSizeGripEnabled(true);
		}

		void Dialog::AddCategory(Categories::Category *category)
		{
			scrollLayout->addWidget(category);
			connect(category,&Categories::Category::Help,&help,&QTextEdit::setText);
		}
	}

	namespace Metrics
	{
		Dialog::Dialog(QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			layout(this),
			rawUsers(this),
			validUsers(this)
		{
			layout.addWidget(&validUsers);
			layout.addWidget(&rawUsers);
			setSizeGripEnabled(true);

			connect(&acknowledgeDelay,&QTimer::timeout,this,QOverload<>::of(&Dialog::Acknowledged));
		}

		void Dialog::Joined(const QString &user)
		{
			rawUsers.addItem(user);
			UpdateTitle();
			acknowledgeDelay.start(1000);
		}

		void Dialog::Acknowledged(const QStringList &names)
		{
			validUsers.clear();
			for (const QString &name : names) validUsers.addItem(name);
			QList<QListWidgetItem*> items=rawUsers.findItems("*",Qt::MatchWildcard);
			for (QListWidgetItem *item : items)
			{
				if (validUsers.findItems(item->text(),Qt::MatchExactly).isEmpty())
					item->setForeground(palette().mid());
				else
					item->setForeground(palette().text());
			}
			UpdateTitle();
		}

		void Dialog::Parted(const QString &user)
		{
			QList<QListWidgetItem*> items=rawUsers.findItems(user,Qt::MatchExactly);
			for (QListWidgetItem *item : items) delete rawUsers.takeItem(rawUsers.row(item));
			items=validUsers.findItems(user,Qt::MatchExactly);
			for (QListWidgetItem *item : items) delete validUsers.takeItem(validUsers.row(item));
			UpdateTitle();
		}

		void Dialog::UpdateTitle()
		{
			setWindowTitle(QStringLiteral("Metrics (%1/%2)").arg(StringConvert::Integer(validUsers.count()),StringConvert::Integer(rawUsers.count())));
		}
	}
}
