#include <QScrollBar>
#include <QShowEvent>
#include <QEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QFontDialog>
#include <QVideoWidget>
#include <QWindow>
#include <QScreen>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextFrame>
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

namespace UI
{
	int ScreenWidthThird(QWidget *widget)
	{
		return widget->window()->windowHandle()->screen()->availableGeometry().width()/3;
	}

	QTableWidgetItem* ReadOnlyItem(const QString &text)
	{
		QTableWidgetItem *item=new QTableWidgetItem(text);
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		return item;
	}

	QString OpenVideo(QWidget *parent,QString initialPath)
	{
		return QDir::toNativeSeparators(QFileDialog::getOpenFileName(parent,Text::DIALOG_TITLE_FILE,initialPath.isEmpty() ? Filesystem::HomePath().absolutePath() : initialPath,QString("Videos (*.%1)").arg(Text::FILE_TYPE_VIDEO)));
	}

	QString OpenAudio(QWidget *parent,QString initialPath)
	{
		return QDir::toNativeSeparators(QFileDialog::getOpenFileName(parent,Text::DIALOG_TITLE_FILE,initialPath.isEmpty() ? Filesystem::HomePath().absolutePath() : initialPath,QString("Audios (*.%1)").arg(Text::FILE_TYPE_AUDIO)));
	}

	namespace Feedback
	{
		Error::Error() : errors(0) { }

		void Error::SwapTrackingName(const QString &oldName,const QString &newName)
		{
			auto candidate=errors.find(oldName);
			if (candidate != errors.end())
			{
				errors.erase(candidate);
				errors.insert(newName);
			}
			CompileErrorMessages();
		}

		void Error::Valid(QWidget *widget)
		{
			widget->setStyleSheet("background-color: none;");
			errors.erase(widget->objectName());
			if (errors.size() == 0) emit Clear(true);
			CompileErrorMessages();
		}

		void Error::Invalid(QWidget *widget)
		{
			widget->setStyleSheet("background-color: LavenderBlush;");
			if (errors.insert(widget->objectName()).second) emit Clear(false);
			CompileErrorMessages();
		}

		void Error::CompileErrorMessages()
		{
			QString messages;
			for (const QString &error : errors) messages+=error+"<br>";
			emit ReportProblem(messages.trimmed());
			emit Count(messages.size());
		}

		Help::Help(QWidget *parent) : QTextEdit(parent)
		{
			setEnabled(false);
			setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding));
			setStyleSheet(QStringLiteral("border: none; color: palette(window-text);"));
		}
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
			const auto selectedItems=list.selectedItems();
			for (QListWidgetItem *item : selectedItems) scopes.append(item->text());
		}
	}

	namespace Commands
	{
		/*!
		 * \brief NamesList::NamesList
		 * \param title Window title of the dialog
		 * \param placeholder Descriptive text for input field for adding items to the list
		 * \param parent Parent QWidget that contains the dialog
		 */
		NamesList::NamesList(const QString &title,const QString &placeholder,QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			layout(this),
			list(this),
			name(this),
			add(Text::BUTTON_ADD,this),
			remove(Text::BUTTON_REMOVE,this)
		{
			setModal(true);
			setWindowTitle(title);

			setLayout(&layout);

			name.setPlaceholderText(placeholder);

			layout.addWidget(&list,0,0,1,3);
			layout.addWidget(&name,1,0);
			layout.addWidget(&add,1,1);
			layout.addWidget(&remove,1,2);

			connect(&add,&QPushButton::clicked,this,&NamesList::Add);
			connect(&remove,&QPushButton::clicked,this,&NamesList::Remove);

			setSizeGripEnabled(true);
		}

		void NamesList::Populate(const QStringList &names)
		{
			list.clear();
			for (const QString &name : names) list.addItem(name);
		}

		NamesList::operator QStringList() const
		{
			QStringList result;
			for (int index=0; index < list.count(); index++) result.append(list.item(index)->text());
			return result;
		}

		void NamesList::Add()
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

		void NamesList::Remove()
		{
			QListWidgetItem *item=list.currentItem();
			if (!item) return;
			item=list.takeItem(list.row(item));
			if (item) delete item;
		}

		void NamesList::hideEvent(QHideEvent *event)
		{
			emit Finished();
			QDialog::hideEvent(event);
		}

		Entry::Entry(Feedback::Error &errorReport,QWidget *parent) : QWidget(parent),
			layout(this),
			details(this),
			detailsLayout(nullptr),
			header(this),
			name(u"Command Name"_s,std::bind_front(&Entry::SetUpCommandNameTextEdit,this),&details),
			description(u"Command Description"_s,std::bind_front(&Entry::SetUpDescriptionTextEdit,this),&details),
			aliases(u"Aliases Dialog"_s,std::bind_front(&Entry::SetUpAliasesButton,this),&details),
			triggers(u"Triggers Dialog"_s,std::bind_front(&Entry::SetUpTriggersButton,this),&details),
			path(u"Media Location"_s,std::bind_front(&Entry::SetUpPathTextEdit,this),&details),
			browse(u"Browse"_s,std::bind_front(&Entry::SetUpBrowseButton,this),&details),
			type(u"Type"_s,std::bind_front(&Entry::SetUpTypeList,this),&details),
			random(u"Choose Random Media"_s,std::bind_front(&Entry::SetUpRandomCheckBox,this),&details),
			duplicates(u"Allow Duplicates"_s,std::bind_front(&Entry::SetUpDuplicatesCheckBox,this),&details),
			protect(u"Protect"_s,std::bind_front(&Entry::SetUpProtectCheckBox,this),&details),
			message(u"Message"_s,std::bind_front(&Entry::SetUpMessageTextEdit,this),&details),
			errorReport(errorReport)
		{
			type=static_cast<int>(UI::Commands::Type::INVALID);

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

			details.setLayout(&detailsLayout);
			details.setFrameShape(QFrame::NoFrame);
			details.setVisible(false);
			frameLayout->addWidget(&details);

			connect(&header,&QPushButton::clicked,this,&Entry::ToggleFold);
		}

		Entry::Entry(const Command &command,Feedback::Error &errorReport,QWidget *parent) : Entry(errorReport,parent)
		{
			name=command.Name();
			description=command.Description();
			protect=command.Protected();
			path=command.Path();
			random=command.Random();
			duplicates=command.Duplicates();
			message=command.Message();
			triggers=command.Viewers();

			switch (command.Type())
			{
			case CommandType::BLANK:
				throw std::logic_error(u"Warning: Type for command !%1 was blank."_s.arg(name).toStdString());
			case CommandType::NATIVE:
				type=static_cast<int>(Type::NATIVE);
				break;
			case CommandType::VIDEO:
				type=static_cast<int>(Type::VIDEO);
				break;
			case CommandType::AUDIO:
				type=static_cast<int>(Type::AUDIO);
				break;
			case CommandType::PULSAR:
				type=static_cast<int>(Type::PULSAR);
				break;
			}

			UpdateHeader(Name());
		}

		QString Entry::Name() const
		{
			return name;
		}

		void Entry::UpdateName()
		{
			if (!ValidateName(name))
			{
				name.RevertValue();
				ValidateName(name);
				return;
			}
			errorReport.SwapTrackingName(BuildErrorTrackingName(name.CachedValue()),BuildErrorTrackingName(name));
			path.Name(BuildErrorTrackingName(name,path.Name()));
			name.CacheValue();
			name.Name(BuildErrorTrackingName(name));
			UpdateHeader(name);
		}

		QString Entry::Description() const
		{
			return description;
		}

		void Entry::UpdateDescription(const QString &text)
		{
			description=text;
		}

		QStringList Entry::Aliases() const
		{
			return aliases;
		}

		void Entry::Aliases(const QStringList &names)
		{
			aliases=names;
			UpdateHeader();
		}

		QStringList Entry::Triggers() const
		{
			return triggers;
		}

		QString Entry::Path() const
		{
			return path;
		}

		void Entry::UpdatePath(const QString &text)
		{
			path=text;
		}

		QStringList Entry::Filters() const
		{
			return Command::FileListFilters(Type());
		}

		CommandType Entry::Type() const
		{
			UI::Commands::Type candidate;
			if (type == static_cast<int>(UI::Commands::Type::INVALID))
				candidate=static_cast<enum Type>(static_cast<int>(type));
			else
				candidate=static_cast<enum Type>(static_cast<int>(type));

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
				throw std::logic_error(u"Fatal: An unrecognized type was selected for command !%1."_s.arg(name).toStdString());
			}
		}

		bool Entry::Random() const
		{
			return random;
		}

		void Entry::UpdateRandom(int state)
		{
			random=state == Qt::Checked;
		}

		bool Entry::Duplicates() const
		{
			return duplicates;
		}

		void Entry::UpdateDuplicates(int state)
		{
			duplicates=state == Qt::Checked;
		}

		QString Entry::Message() const
		{
			return message;
		}

		void Entry::UpdateMessage()
		{
			message.CacheValue();
		}

		bool Entry::Protected() const
		{
			return protect;
		}

		void Entry::UpdateProtect(int state)
		{
			protect=state == Qt::Checked;
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
			if (details.isVisible())
			{
				name.Hide();
				description.Hide();
				type.Hide();
				path.Hide();
				protect.Hide();
				duplicates.Hide();
				random.Hide();
				message.Hide();
				browse.Hide();
				aliases.Hide();
				triggers.Show();
				details.setVisible(false);
			}
			else
			{
				name.Show();
				description.Show();
				type.Show();
				path.Show();
				protect.Show();
				duplicates.Show();
				random.Show();
				message.Show();
				browse.Show();
				aliases.Show();
				triggers.Show();
				details.setVisible(true);
			}
		}

		void Entry::SetUpCommandNameTextEdit(QLineEdit *widget)
		{
			widget->setPlaceholderText(u"Command Name"_s);
			widget->setObjectName(BuildErrorTrackingName(name));
			widget->setText(name);
			connect(widget,&QLineEdit::textChanged,this,&Entry::ValidateName);
			connect(widget,&QLineEdit::editingFinished,this,&Entry::UpdateName);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,0,0);
		}

		void Entry::SetUpDescriptionTextEdit(QLineEdit *widget)
		{
			widget->setPlaceholderText(u"Description of the action this command takes"_s);
			widget->setText(description);
			connect(widget,&QLineEdit::textChanged,this,&Entry::UpdateDescription);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,1,0,1,4);
		}

		void Entry::SetUpTypeList(QComboBox *widget)
		{
			widget->addItems({
				u"Video"_s,
				u"Announce"_s,
				u"Pulsar"_s
			});
			widget->setPlaceholderText(u"Native"_s);
			widget->setCurrentIndex(type);
			widget->setFocusPolicy(Qt::StrongFocus);
			connect(widget,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&Entry::TypeChanged);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,3,0,1,2);
		}

		void Entry::SetUpPathTextEdit(QLineEdit *widget)
		{
			widget->setPlaceholderText(path.Name());
			widget->setObjectName(BuildErrorTrackingName(Name(),path.Name()));
			widget->setText(path);
			connect(widget,&QLineEdit::textChanged,this,QOverload<const QString&>::of(&Entry::ValidatePath));
			connect(widget,&QLineEdit::textChanged,this,&Entry::UpdatePath);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,2,0,1,3);
		}

		void Entry::SetUpProtectCheckBox(QCheckBox *widget)
		{
			widget->setText(u"Protect"_s);
			widget->setChecked(protect);
			connect(widget,&QCheckBox::stateChanged,this,&Entry::UpdateProtect);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,0,3);
		}

		void Entry::SetUpRandomCheckBox(QCheckBox *widget)
		{
			widget->setChecked(random);
			widget->setText(u"Random"_s);
			connect(widget,&QCheckBox::stateChanged,this,&Entry::RandomChanged);
			connect(widget,&QCheckBox::stateChanged,this,&Entry::UpdateRandom);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,3,2,1,1);
		}

		void Entry::SetUpDuplicatesCheckBox(QCheckBox *widget)
		{
			widget->setChecked(duplicates);
			widget->setEnabled(random);
			widget->setText(u"Duplicates"_s);
			connect(widget,&QCheckBox::stateChanged,this,&Entry::UpdateDuplicates);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,3,3,1,1);
		}

		void Entry::SetUpMessageTextEdit(QTextEdit *widget)
		{
			widget->setPlaceholderText(u"Message to display in announcement"_s);
			widget->setText(message);
			widget->setVisible(type == static_cast<int>(UI::Commands::Type::AUDIO));
			connect(widget,&QTextEdit::textChanged,this,&Entry::ValidateMessage);
			connect(widget,&QTextEdit::textChanged,this,&Entry::UpdateMessage);
			widget->viewport()->installEventFilter(this);
			detailsLayout.addWidget(widget,4,0,1,4);
		}

		void Entry::SetUpBrowseButton(QPushButton *widget)
		{
			widget->setText(Text::BROWSE);
			connect(widget,&QPushButton::clicked,this,&Entry::Browse);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,2,3,1,1);
		}

		void Entry::SetUpAliasesButton(QPushButton *widget)
		{
			widget->setText(u"Aliases"_s);
			connect(widget,&QPushButton::clicked,this,&Entry::SelectAliases);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,0,1);
		}

		void Entry::SetUpTriggersButton(QPushButton *widget)
		{
			widget->setText(u"Triggers"_s);
			connect(widget,&QPushButton::clicked,this,&Entry::SelectTriggers);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,0,2);
		}

		bool Entry::ValidateName(const QString &text)
		{
			bool valid=!text.isEmpty();
			if (valid)
				errorReport.Valid(*name);
			else
				errorReport.Invalid(*name);
			return valid;
		}

		bool Entry::ValidatePath(const QString &text)
		{
			QFileInfo candidate(text);

			if (!candidate.exists())
			{
				errorReport.Invalid(*path);
				return false;
			}

			if (Random())
			{
				bool valid=candidate.isDir();
				if (valid)
					errorReport.Valid(*path);
				else
					errorReport.Invalid(*path);
				return valid;
			}
			else
			{
				if (candidate.isDir())
				{
					errorReport.Invalid(*path);
					return false;
				}

				bool valid;
				const QString extension=candidate.suffix();
				switch (static_cast<UI::Commands::Type>(static_cast<int>(type)))
				{
				case Type::VIDEO:
					valid=extension == Text::FILE_TYPE_VIDEO;
					break;
				case Type::AUDIO:
					valid=extension == Text::FILE_TYPE_AUDIO;
					break;
				default:
					valid=false;
					break;
				}
				if (valid)
					errorReport.Valid(*path);
				else
					errorReport.Invalid(*path);
				return valid;
			}
		}

		bool Entry::ValidateMessage()
		{
			if (static_cast<UI::Commands::Type>(static_cast<int>(type)) != UI::Commands::Type::AUDIO) return false;
			bool valid=Message().isEmpty();
			if (valid)
				errorReport.Valid(*message);
			else
				errorReport.Invalid(*message);
			return valid;
		}

		void Entry::RandomChanged(const int state)
		{
			bool checked=state == Qt::Checked;
			duplicates.Enable(checked);
			ValidatePath(path);
		}

		void Entry::TypeChanged(int index)
		{
			type=index;

			if (static_cast<UI::Commands::Type>(static_cast<int>(type)) == Type::PULSAR)
			{
				Pulsar();
				return;
			}

			if (static_cast<UI::Commands::Type>(static_cast<int>(type)) == Type::NATIVE)
			{
				Native();
				return;
			}

			name.Enable(true);
			description.Enable(true);
			aliases.Enable(true);
			protect.Enable(true);
			path.Enable(true);
			browse.Enable(true);
			type.Enable(true);
			random.Enable(true);
			message.Visible(type == static_cast<int>(Type::VIDEO) ? false : true);

			ValidatePath(path);
		}

		void Entry::Native()
		{
			name.Enable(false);
			description.Enable(false);
			aliases.Enable(true);
			protect.Enable(false);
			path.Enable(false);
			browse.Enable(false);
			type.Enable(false);
			random.Enable(false);
			message.Visible(false);
		}

		void Entry::Pulsar()
		{
			name.Enable(true);
			description.Enable(true);
			aliases.Enable(false);
			protect.Enable(true);
			path.Enable(false);
			browse.Enable(false);
			random.Enable(false);
			message.Visible(false);
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
				if (static_cast<UI::Commands::Type>(static_cast<int>(type)) == UI::Commands::Type::VIDEO)
					candidate=OpenVideo(this);
				else
					candidate=OpenAudio(this);
			}
			if (!candidate.isEmpty()) path=candidate;
		}

		void Entry::SelectAliases()
		{
			UI::Commands::NamesList *dialog=new UI::Commands::NamesList(u"Command Aliases"_s,u"Alias"_s,this);
			dialog->Populate(aliases);
			connect(dialog,&UI::Commands::NamesList::Finished,dialog,[dialog,this]() {
				aliases=*dialog;
			});
			connect(dialog,&UI::Commands::NamesList::Finished,this,QOverload<>::of(&Entry::UpdateHeader));
			connect(dialog,&UI::Commands::NamesList::Finished,dialog,&UI::Commands::NamesList::deleteLater);
			dialog->show();
		}

		void Entry::SelectTriggers()
		{
			UI::Commands::NamesList *dialog=new UI::Commands::NamesList(u"Command Triggers"_s,u"Viewer Name"_s,this);
			dialog->Populate(triggers);
			connect(dialog,&UI::Commands::NamesList::Finished,dialog,[dialog,this]() {
				triggers=*dialog;
			});
			connect(dialog,&UI::Commands::NamesList::Finished,dialog,&UI::Commands::NamesList::deleteLater);
			dialog->show();
		}

		bool Entry::eventFilter(QObject *object,QEvent *event)
		{
			if (event->type() == QEvent::Enter)
			{
				if (object == name) emit Help("Name of the command. This is the text that must appear after an exclamation mark (!).");
				if (object == description) emit Help("Short description of the command that will appear in in list of commands and showcase rotation.");
				if (object == protect) emit Help("When enabled, only the broadcaster and moderators will be able to use this command.");
				if (object == aliases) emit Help("List of alternate command names.");
				if (object == triggers) emit Help("List of viewers who's arrival will trigger this command. If multiple viewers are listed, all of them have to arrive before the command will be triggered.");
				if (object == path || object == browse) emit Help("Location of the media that will be played for command types that use media, such as Video and Announce");
				if (object == random) emit Help("When enabled, the media path must point to a folder instead of a file. An appropriate file will be selected from that path when the command is triggered.");
				if (object == duplicates) emit Help("When choosing a random media file from a folder, sometimes the same file can be chosen back-to-back. Check this box to prevent that.");
				if (object == type) emit Help("Determines what kind of action is taken in response to a command.\n\nValid values are:\nAnnounce - Displays text while playing an audio file\nVideo - Displays a video");
				if (std::optional<QWidget*> viewport=message.Viewport(); viewport && object == message.Viewport()) emit Help("If the command will display text in conjunction with the media, this is the message that will be shown.");
			}

			if (event->type() == QEvent::Wheel)
			{
				QComboBox *combo=qobject_cast<QComboBox*>(object);
				if (combo && !combo->hasFocus())
				{
					wheelEvent(static_cast<QWheelEvent*>(event));
					return true;
				}
			}

			return false;
		}

		QString Entry::BuildErrorTrackingName(const QString &commandName,const QString message)
		{
			QString trackingName=QString("<b>!%1</b>").arg(commandName);
			if (!message.isEmpty()) trackingName+=" "+message;
			return trackingName;
		}

		QString Entry::BuildErrorTrackingName(const QString &commandName)
		{
			return BuildErrorTrackingName(commandName,{});
		}

		Dialog::Dialog(const Command::Lookup &commands,QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			entriesFrame(this),
			scrollLayout(&entriesFrame),
			helpBox("Help",this),
			help(this),
			labelFilter("Filter:",this),
			filter(this),
			buttons(this),
			discard(Text::BUTTON_DISCARD,this),
			save(Text::BUTTON_SAVE,this),
			newEntry("&New",this),
			errorBox("Errors",this),
			errorMessages(&errorBox),
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
			QVBoxLayout *helpBoxLayout=new QVBoxLayout(&helpBox);
			helpBox.setLayout(helpBoxLayout);
			helpBoxLayout->addWidget(&help);
			rightLayout->addWidget(&helpBox,0,0,1,2);
			QVBoxLayout *errorBoxLayout=new QVBoxLayout(&errorBox);
			errorBox.setLayout(errorBoxLayout);
			errorBox.setVisible(false);
			errorBoxLayout->addWidget(&errorMessages);
			rightLayout->addWidget(&errorBox,1,0,1,2);
			statusBar.setSizeGripEnabled(false);
			rightLayout->addWidget(&statusBar,2,0,1,2);
			labelFilter.setSizePolicy({QSizePolicy::Fixed,QSizePolicy::Fixed});
			rightLayout->addWidget(&labelFilter,2,0);
			filter.addItems({"All","Native","Dynamic","Pulsar"});
			filter.setCurrentIndex(0);
			connect(&filter,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&Dialog::FilterChanged);
			rightLayout->addWidget(&filter,2,1);
			upperLayout->addWidget(rightPane);

			QWidget *lowerContent=new QWidget(this);
			QHBoxLayout *lowerLayout=new QHBoxLayout(lowerContent);
			lowerContent->setLayout(lowerLayout);
			mainLayout->addWidget(lowerContent);

			buttons.addButton(&save,QDialogButtonBox::AcceptRole);
			buttons.addButton(&discard,QDialogButtonBox::RejectRole);
			buttons.addButton(&newEntry,QDialogButtonBox::ActionRole);
			connect(&buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
			connect(&buttons,&QDialogButtonBox::accepted,this,QOverload<>::of(&Dialog::Save));
			connect(&newEntry,&QPushButton::clicked,this,&UI::Commands::Dialog::Add);
			lowerLayout->addWidget(&buttons);

			setSizeGripEnabled(true);

			// we have to create a dummy entry and unfold it to correctly
			// calculate the initial size of the dialog because it's based
			// on the width of the widgets within an entry
			Entry ruler(errorReport,this); // there seems to be no problem with putting this on the stack (atm)
			ruler.ToggleFold();
			scrollLayout.addWidget(&ruler);
			adjustSize();
		}

		void Dialog::PopulateEntries(const Command::Lookup &commands)
		{
			entriesFrame.setUpdatesEnabled(false);
			std::unordered_map<QString,QStringList> aliases;
			for (const auto& [name,command] : commands)
			{
				if (command.Parent())
				{
					aliases[command.Parent()->Name()].push_back(name); // NOTE: This is an assignment, not just a lookup
					continue;
				}

				try
				{
					auto [entryIterator,inserted]=entries.try_emplace(command.Name(),nullptr);
					if (!inserted) throw std::runtime_error(QString(R"(Failed to create an entry for "%1" command)").arg(command.Name()).toStdString());
					auto& [name,entry]=*entryIterator;
					entry=new Entry(command,errorReport,&entriesFrame);
					connect(entry,&Entry::Help,&help,&QTextEdit::setText);
					connect(&errorReport,&Feedback::Error::Clear,&save,&QPushButton::setEnabled);
					connect(&errorReport,&Feedback::Error::Count,&errorBox,&QGroupBox::setVisible);
					connect(&errorReport,&Feedback::Error::ReportProblem,&errorMessages,&QLabel::setText);
					scrollLayout.addWidget(entry);
				}

				catch (const std::logic_error &exception)
				{
					statusBar.showMessage(exception.what());
				}
			}
			for (const auto& [commandName,aliasNames] : aliases)
			{
				auto entry=entries.find(commandName);
				if (entry != entries.end()) entry->second->Aliases(aliasNames);
			}
			entriesFrame.setUpdatesEnabled(true);
		}

		void Dialog::Add()
		{
			const QString name=QInputDialog::getText(this,"New Command","Please provide a name for the new command.");
			if (name.isEmpty()) return;
			auto [entryIterator,inserted]=entries.try_emplace(name,nullptr);
			if (!inserted) throw std::runtime_error("Failed to create a new entry for the command");
			entryIterator->second=new Entry(
				Command{
					name,
					{},
					CommandType::VIDEO, // has a type, so no need to catch possible exception here
					false,
					true,
					{},
					Command::FileListFilters(CommandType::VIDEO),
					{},
					{},
					false
				},
				errorReport,
				this
			);
			scrollLayout.addWidget(entryIterator->second);
		}

		void Dialog::FilterChanged(int index)
		{
			switch (static_cast<Filter>(index))
			{
			case Filter::ALL:
				for (auto& [name,entry] : entries) entry->show();
				break;
			case Filter::DYNAMIC:
				for (auto& [name,entry] : entries)
				{
					CommandType type=entry->Type();
					if (type == CommandType::AUDIO || type == CommandType::VIDEO)
						entry->show();
					else
						entry->hide();
				}
				break;
			case Filter::NATIVE:
				for (auto& [name,entry] : entries)
				{
					if (entry->Type() == CommandType::NATIVE)
						entry->show();
					else
						entry->hide();
				}
				break;
			case Filter::PULSAR:
				for (auto& [name,entry] : entries)
				{
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

			for (const auto& [name,entry] : entries)
			{
				commands.try_emplace(name,
					entry->Name(),
					entry->Description(),
					entry->Type(),
					entry->Random(),
					entry->Duplicates(),
					entry->Path(),
					entry->Filters(),
					entry->Message(),
					entry->Triggers(),
					entry->Protected()
				);

				const auto aliases=entry->Aliases();
				for (const QString &alias : aliases) commands.try_emplace(alias,alias,&commands.at(name));
			}

			emit Save(commands);
			accept();
		}
	}

	namespace Options
	{
		namespace Categories
		{
			Category::Category(QWidget *parent,const QString &name) : QFrame(parent),
				verticalLayout(this),
				header(this),
				details(nullptr),
				detailsLayout(nullptr)
			{
				setLayout(&verticalLayout);
				setFrameShape(QFrame::Box);

				header.setStyleSheet(QString("font-size: %1pt; font-weight: bold; text-align: left;").arg(header.font().pointSizeF()*1.25));
				header.setCursor(Qt::PointingHandCursor);
				header.setFlat(true);
				header.setText(name);
				verticalLayout.addWidget(&header);

				details=new QFrame(this);
				details->setLayout(&detailsLayout);
				verticalLayout.addWidget(details);

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
					if (std::ssize(row) > maxColumns) maxColumns=row.size();
				}

				for (int rowIndex=0; rowIndex < std::ssize(widgets); rowIndex++)
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

			Channel::Channel(Settings settings,std::shared_ptr<Feedback::Error> errorReport,QWidget *parent) : Category(parent,QStringLiteral("Channel")),
				name(this),
				protection(this),
				settings(settings),
				errorReport(errorReport)
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
				bool valid=!text.isEmpty();
				if (valid)
					errorReport->Valid(&name);
				else
					errorReport->Invalid(&name);
			}

			void Channel::Save()
			{
				settings.name.Set(name.text());
				settings.protection.Set(protection.isChecked());

				// only need to do this on one of the settings for all of the categories, because it
				// is all the same QSettings object under the hood, so it's saving all of the settings
				settings.name.Save();
			}

			Window::Window(Settings settings,QWidget *parent) : Category(parent,QStringLiteral("Main Window")),
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

			Status::Status(Settings settings,std::shared_ptr<Feedback::Error> errorReport,QWidget *parent) : Category(parent,QStringLiteral("Status")),
				font(this),
				fontSize(this),
				selectFont(Text::CHOOSE,this),
				foregroundColor(this),
				previewForegroundColor(this,settings.foregroundColor),
				selectForegroundColor(Text::CHOOSE,this),
				backgroundColor(this),
				previewBackgroundColor(this,settings.backgroundColor),
				selectBackgroundColor(Text::CHOOSE,this),
				settings(settings),
				errorReport(errorReport)
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
				if (candidate.exactMatch())
					errorReport->Valid(&font);
				else
					errorReport->Invalid(&font);
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

			Chat::Chat(Settings settings,std::shared_ptr<Feedback::Error> errorReport,QWidget *parent) : Category(parent,QStringLiteral("Chat")),
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
				settings(settings),
				errorReport(errorReport)
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
				if (candidate.exactMatch())
					errorReport->Valid(&font);
				else
					errorReport->Invalid(&font);
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

			Pane::Pane(Settings settings,std::shared_ptr<Feedback::Error> errorReport,QWidget *parent) : Category(parent,QStringLiteral("Panes")),
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
				settings(settings),
				errorReport(errorReport)
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
				if (candidate.exactMatch())
					errorReport->Valid(&font);
				else
					errorReport->Invalid(&font);
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

			Music::Music(Settings settings,QWidget *parent) : Category(parent,QStringLiteral("Music")),
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

			Bot::Bot(Settings settings,std::shared_ptr<Feedback::Error> errorReport,QWidget *parent) : Category(parent,QStringLiteral("Bot Core")),
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
				postRaidEventDelay(this),
				postRaidEventDelayThreshold(this),
				selectRaidSound(Text::BROWSE,this),
				previewRaidSound(Text::PREVIEW,this),
				inactivityCooldown(this),
				helpCooldown(this),
				textWallSound(this),
				selectTextWallSound(Text::BROWSE,this),
				previewTextWallSound(Text::PREVIEW,this),
				textWallThreshold(this),
				pulsarEnabled(this),
				settings(settings),
				errorReport(errorReport)
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
				postRaidEventDelay.setRange(1,std::numeric_limits<int>::max());
				postRaidEventDelay.setValue(settings.raidInterruptDuration);
				postRaidEventDelayThreshold.setRange(0,std::numeric_limits<int>::max());
				postRaidEventDelayThreshold.setValue(settings.raidInterruptDelayThreshold);
				inactivityCooldown.setRange(TimeConvert::Milliseconds(TimeConvert::OneSecond()).count(),std::numeric_limits<int>::max());
				inactivityCooldown.setValue(settings.inactivityCooldown);
				helpCooldown.setRange(TimeConvert::Milliseconds(TimeConvert::OneSecond()).count(),std::numeric_limits<int>::max());
				helpCooldown.setValue(settings.helpCooldown);
				textWallThreshold.setRange(1,std::numeric_limits<int>::max());
				textWallThreshold.setValue(settings.textWallThreshold);
				textWallSound.setText(settings.textWallSound);
				pulsarEnabled.setChecked(settings.pulsarEnabled);

				Rows({
					{Label(u"Arrival Announcement Audio"_s),&arrivalSound,&selectArrivalSound,&previewArrivalSound},
					{Label(u"Portrait (Ping) Video"_s),&portraitVideo,&selectPortraitVideo,&previewPortraitVideo},
					{Label(u"Cheer (Bits) Video"_s),&cheerVideo,&selectCheerVideo,&previewCheerVideo},
					{Label(u"Subscription Announcement"_s),&subscriptionSound,&selectSubscriptionSound,&previewSubscriptionSound},
					{Label(u"Raid Announcement"_s),&raidSound,&selectRaidSound,&previewRaidSound},
					{Label(u"Post-Raid Greeting Delay"_s),&postRaidEventDelay,Label(u"Threshold"_s),&postRaidEventDelayThreshold},
					{Label(u"Inactivity Cooldown"_s),&inactivityCooldown},
					{Label(u"Help Cooldown"_s),&helpCooldown},
					{Label(u"Wall-of-Text Sound"_s),&textWallSound,&selectTextWallSound,&previewTextWallSound,Label(u"Threshold"_s),&textWallThreshold},
					{Label(u"Use Pulsar"_s),&pulsarEnabled}
				});
			}

			bool Bot::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &arrivalSound || object == &selectArrivalSound || object == &previewArrivalSound) emit Help(u"This is the sound that plays each time someone speak in chat for the first time. This can be a single audio file (mp3), or a folder of audio files. If it's a folder, a random audio file will be chosen from that folder each time."_s);
					if (object == &portraitVideo || object == &selectPortraitVideo || object == &previewPortraitVideo) emit Help(u"Every so often, Twitch will send a request to the bot asking if it's still connected (ping). This is a video that can play each time that happens."_s);
					if (object == &cheerVideo || object == &selectCheerVideo || object == &previewCheerVideo) emit Help(u"A video (mp4) that plays when a chatter cheers bits."_s);
					if (object == &subscriptionSound || object == &selectSubscriptionSound || object == &previewSubscriptionSound) emit Help(u"This is the sound that plays when a chatter subscribes to the channel."_s);
					if (object == &raidSound || object == &selectRaidSound || object == &previewRaidSound) emit Help(u"This is the sound that plays when another streamer raids the channel."_s);
					if (object == &postRaidEventDelay) emit Help(u"How long (in milliseconds) to wait after a raid before allowing additional media to be triggered"_s);
					if (object == &postRaidEventDelayThreshold) emit Help(u"Under this many viewers, there will not be a delay following the raid"_s);
					if (object == &inactivityCooldown) emit Help(uR"(This is the amount of time (in milliseconds) that must pass without any chat messages before Celeste plays a "roast" video)"_s);
					if (object == &helpCooldown) emit Help(uR"(This is the amount of time (in milliseconds) between "help" message. A help message is an explanation of a single, randomly chosen command.)"_s);
					if (object == &textWallThreshold || object == &textWallSound || object == &selectTextWallSound || object == &previewTextWallSound) emit Help(u"This is the sound that plays when a user spams chat with a super long message. The threshold is the number of characters the message needs to be to trigger the sound."_s);
					if (object == &pulsarEnabled) emit Help(u"Check this to enable the Pulsar plugin which allows the bot to communicate with OBS Studio. This will only work if the Pulsar plugin was installed when the bot was installed."_s);
				}

				if (event->type() == QEvent::HoverLeave) emit Help(u""_s);
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
				emit PlayArrivalSound(qApp->applicationName(),std::make_shared<QImage>(Resources::CELESTE),QFileInfo(filename).isDir() ? File::List(filename).Random() : filename);
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
				if (valid)
					errorReport->Valid(&arrivalSound);
				else
					errorReport->Invalid(&arrivalSound);
				previewArrivalSound.setEnabled(valid);
			}

			void Bot::ValidatePortraitVideo(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() && candidate.suffix() == Text::FILE_TYPE_VIDEO;
				if (valid)
					errorReport->Valid(&portraitVideo);
				else
					errorReport->Invalid(&portraitVideo);
				previewPortraitVideo.setEnabled(valid);
			}

			void Bot::ValidateCheerVideo(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() && candidate.suffix() == Text::FILE_TYPE_VIDEO;
				if (valid)
					errorReport->Valid(&cheerVideo);
				else
					errorReport->Invalid(&cheerVideo);
				previewCheerVideo.setEnabled(valid);
			}

			void Bot::ValidateSubscriptionSound(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() && candidate.suffix() == Text::FILE_TYPE_AUDIO;
				if (valid)
					errorReport->Valid(&subscriptionSound);
				else
					errorReport->Invalid(&subscriptionSound);
				previewSubscriptionSound.setEnabled(valid);
			}

			void Bot::ValidateRaidSound(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() && candidate.suffix() == Text::FILE_TYPE_AUDIO;
				if (valid)
					errorReport->Valid(&raidSound);
				else
					errorReport->Invalid(&raidSound);
				previewRaidSound.setEnabled(valid);
			}

			void Bot::ValidateTextWallSound(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() &&candidate.suffix() == Text::FILE_TYPE_AUDIO;
				if (valid)
					errorReport->Valid(&textWallSound);
				else
					errorReport->Invalid(&textWallSound);
				textWallSound.setEnabled(valid);
			}

			void Bot::Save()
			{
				if (QString text=arrivalSound.text(); !text.isEmpty()) settings.arrivalSound.Set(text);
				if (QString text=portraitVideo.text(); !text.isEmpty()) settings.portraitVideo.Set(text);
				if (QString text=cheerVideo.text(); !text.isEmpty()) settings.cheerVideo.Set(text);
				if (QString text=subscriptionSound.text(); !text.isEmpty()) settings.subscriptionSound.Set(text);
				if (QString text=raidSound.text(); !text.isEmpty()) settings.raidSound.Set(text);
				settings.raidInterruptDuration.Set(postRaidEventDelay.value());
				settings.raidInterruptDelayThreshold.Set(postRaidEventDelayThreshold.value());
				settings.inactivityCooldown.Set(inactivityCooldown.value());
				settings.helpCooldown.Set(helpCooldown.value());
				settings.textWallThreshold.Set(textWallThreshold.value());
				if (QString text=textWallSound.text(); !text.isEmpty()) settings.textWallSound.Set(text);
				settings.pulsarEnabled.Set(pulsarEnabled.isChecked());
			}

			Log::Log(Settings settings,std::shared_ptr<Feedback::Error> errorReport,QWidget *parent) : Category(parent,QStringLiteral("Logging")),
				directory(this),
				selectDirectory(Text::BROWSE,this),
				settings(settings),
				errorReport(errorReport)
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
				if (QDir(path).exists())
					errorReport->Valid(&directory);
				else
					errorReport->Invalid(&directory);
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

			Security::Security(::Security &settings,std::shared_ptr<Feedback::Error> errorReport,QWidget *parent) : Category(parent,QStringLiteral("Security")),
				administrator(this),
				clientID(this),
				token(this),
				callbackURL(this),
				permissions(this),
				selectPermissions(Text::CHOOSE,this),
				settings(settings),
				errorReport(errorReport)
			{
				details->setVisible(false);

				administrator.setText(settings.Administrator());
				clientID.setText(settings.ClientID());
				clientID.setEchoMode(QLineEdit::Password);
				token.setText(settings.OAuthToken());
				token.setEchoMode(QLineEdit::Password);
				callbackURL.setText(settings.CallbackURL());
				permissions.setText(settings.Scope());
				callbackURL.setText(settings.CallbackURL());
				callbackURL.setInputMethodHints(Qt::ImhUrlCharactersOnly);

				connect(&selectPermissions,&QPushButton::clicked,this,&Security::SelectPermissions);
				connect(&callbackURL,&QLineEdit::textChanged,this,&Security::ValidateURL);

				Rows({
					{Label(QStringLiteral("Administrator (Broascaster)")),&administrator},
					{Label(QStringLiteral("Client ID")),&clientID},
					{Label(QStringLiteral("OAuth Token")),&token},
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
					if (object == &token) emit Help(QStringLiteral(R"(OAuth token obtained from Twitch authorization process (usually automatic, but can be manually obtained and entered). This is for "Authorization code grant flow" for Celeste's main API calls.)"));
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
				settings.OAuthToken().Set(token.text());
				settings.CallbackURL().Set(callbackURL.text());
				settings.Scope().Set(permissions.text());
			}

			void Security::ValidateURL(const QString &text)
			{
				if (QUrl(text).isValid())
					errorReport->Valid(&callbackURL);
				else
					errorReport->Invalid(&callbackURL);
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
			// need to do it this way because all UI elements in Celeste require
			// a parent, but we have to wait until after the parent dialog exists
			// to create and attach the categories, so these can't be passed into
			// the constructor
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
			const auto items=users.findItems(user,Qt::MatchExactly);
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
		Dialog::Dialog(const File::List &files,QWidget *parent) : QDialog(parent),
			layout(this),
			list(0,static_cast<int>(Columns::MAX),this),
			buttons(this),
			add(Text::BUTTON_ADD,this),
			remove(Text::BUTTON_REMOVE,this),
			discard(Text::BUTTON_DISCARD,this),
			save(Text::BUTTON_SAVE,this),
			mediaControls(this),
			mediaControlsLayout(&mediaControls),
			volume(Qt::Horizontal,&mediaControls),
			start("\u23F5",&mediaControls),
			stop("\u23F9",&mediaControls),
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

			mediaControlsLayout.addWidget(&stop,0);
			mediaControlsLayout.addWidget(&start,0);
			mediaControlsLayout.addWidget(&volume,1);
			layout.addWidget(&mediaControls);
			connect(&volume,&QSlider::valueChanged,this,&Dialog::Volume);
			connect(&start,&QPushButton::pressed,this,[this]() {
				Play(nullptr); // FIXME: there has to be a better way to do this than a while lambda
			});
			connect(&stop,&QPushButton::pressed,this,&Dialog::Stop);

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

			connect(&list,&QTableWidget::itemDoubleClicked,this,QOverload<QTableWidgetItem*>::of(&Dialog::Play));

			setSizeGripEnabled(true);
		}

		Dialog::Dialog(const File::List &files,const QString currentlyPlayingFile,QWidget *parent): Dialog(files,parent)
		{
			const auto items=list.findItems(currentlyPlayingFile,Qt::MatchExactly);
			for (QTableWidgetItem *item : items) list.selectRow(item->row());
		}

		void Dialog::showEvent(QShowEvent *event)
		{
			setMinimumWidth(ScreenWidthThird(this));
			QDialog::showEvent(event);
		}

		void Dialog::Save()
		{
			QStringList paths;
			for (int row=0; row < list.rowCount(); row++) paths.append(list.item(row,static_cast<int>(Columns::MAX)-1)->text());
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
			list.setItem(row,static_cast<int>(Columns::PATH),ReadOnlyItem(path));
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
				QMessageBox{QMessageBox::Warning,"Failed to add files",failed.join('\n'),QMessageBox::Ok}.exec();
			}
			list.setSortingEnabled(true);
			list.resizeColumnsToContents();
		}

		void Dialog::Remove()
		{
			QList<QTableWidgetItem*> items=list.selectedItems();
			for (QList<QTableWidgetItem*>::iterator candidate=items.begin(); candidate != items.end(); candidate+=static_cast<int>(Columns::MAX))
			{
				list.removeRow(list.row(*candidate));
			}
		}

		void Dialog::Play(QTableWidgetItem *item)
		{
			if (!item)
			{
				if (list.selectedItems().empty()) return;
				item=list.selectedItems().front();
			}
			QTableWidgetItem *candidate=item->column() == static_cast<int>(Columns::PATH) ? item : list.item(item->row(),static_cast<int>(Columns::PATH));
			if (candidate) emit Play(QUrl::fromLocalFile(candidate->text()));
		}

	}

	namespace EventSubscriptions
	{
		const char *HEADER_ID="ID";

		const int Dialog::COLUMN_COUNT=4;

		Dialog::Dialog(QWidget *parent) : QDialog(parent),
			layout(this),
			list(0,COLUMN_COUNT,this),
			buttons(this),
			remove(Text::BUTTON_REMOVE,this),
			close(Text::BUTTON_CLOSE,this)
		{
			setLayout(&layout);

			list.setHorizontalHeaderLabels({u"Type"_s,u"Creation Date"_s,u"CallbackURL"_s,HEADER_ID});
			list.setSelectionBehavior(QAbstractItemView::SelectRows);
			list.setSelectionMode(QAbstractItemView::SingleSelection);
			list.setSortingEnabled(true);
			layout.addWidget(&list);

			buttons.addButton(&close,QDialogButtonBox::AcceptRole);
			buttons.addButton(&remove,QDialogButtonBox::ActionRole);
			connect(&buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
			connect(&buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
			connect(&remove,&QPushButton::clicked,this,&Dialog::Remove);
			layout.addWidget(&buttons);

			remove.setEnabled(false);

			setSizeGripEnabled(true);
		}

		void Dialog::showEvent(QShowEvent *event)
		{
			setMinimumWidth(ScreenWidthThird(this));
			emit RequestSubscriptionList();
			QDialog::showEvent(event);
		}

		void Dialog::Add(const QString &id,const QString &type,const QDateTime &creationDate,const QString &callbackURL)
		{
			list.setSortingEnabled(false);
			list.insertRow(list.rowCount());
			int row=list.rowCount()-1;
			list.setItem(row,0,ReadOnlyItem(type));
			list.setItem(row,1,ReadOnlyItem(creationDate.toString(Qt::RFC2822Date)));
			list.setItem(row,2,ReadOnlyItem(callbackURL));
			list.setItem(row,3,ReadOnlyItem(id));
			list.setSortingEnabled(true);
			list.resizeColumnsToContents();
			remove.setEnabled(true);
		}

		void Dialog::Remove()
		{
			// we get all of the items for the selected row
			// look for the item that has the correct column header (ID)
			// pass that item's text to EventSub as the one to be removed
			QList<QTableWidgetItem*> items=list.selectedItems();
			for (auto candidate=items.begin(); candidate != items.end(); candidate++)
			{
				if (list.horizontalHeaderItem((*candidate)->column())->text() == HEADER_ID) emit RemoveSubscription((*candidate)->text());
			}
			int rowCount=list.rowCount();
			if (rowCount < 1) remove.setEnabled(false);
		}

		void Dialog::Removed(const QString &id)
		{
			QList<QTableWidgetItem*> items=list.findItems(id,Qt::MatchFixedString|Qt::MatchCaseSensitive);
			for (auto candidate=items.begin(); candidate != items.end(); candidate++)
			{
				list.removeRow(list.row(*candidate));
			}
		}
	}
}
