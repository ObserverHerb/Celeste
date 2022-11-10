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

	Color::Color(QWidget *parent,const QString &color) : QLabel(parent)
	{
		Set(color);
		setText(QStringLiteral("preview"));
	}

	void Color::Set(const QString &color)
	{
		setStyleSheet(QString("border: 1px solid black; color: %1; background-color: %1;").arg(color));
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
				details.setVisible(!details.isVisible());
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
				textWallThreshold(this),
				textWallSound(this),
				selectTextWallSound(Text::BROWSE,this),
				previewTextWallSound(Text::PREVIEW,this)
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
