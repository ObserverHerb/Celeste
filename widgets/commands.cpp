#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include "widgets/widgets.h"

namespace UI
{
	namespace Commands
	{
		AliasesList::AliasesList(QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			list(this),
			name(this),
			add(Text::BUTTON_ADD,this),
			remove(Text::BUTTON_REMOVE,this)
		{
			setModal(true);
			setWindowTitle(u"Command Aliases"_s);

			name.setPlaceholderText(u"Alias"_s);

			auto topLevelLayout=new QVBoxLayout(this);
			auto contentFrame=new QFrame(this);
			contentFrame->setFrameShape(QFrame::StyledPanel);
			contentFrame->setFrameShadow(QFrame::Sunken);
			auto contentLayout=new QGridLayout(contentFrame);
			contentLayout->addWidget(&list,0,0,1,3);
			contentLayout->addWidget(&name,1,0);
			contentLayout->addWidget(&add,1,1);
			contentLayout->addWidget(&remove,1,2);
			topLevelLayout->addWidget(contentFrame);

			auto buttons=new QDialogButtonBox(QDialogButtonBox::Save,this);
			auto cancel=buttons->addButton(QDialogButtonBox::Cancel);
			cancel->setDefault(true);
			topLevelLayout->addWidget(buttons);

			connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
			connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
			connect(&add,&QPushButton::clicked,this,&AliasesList::Add);
			connect(&remove,&QPushButton::clicked,this,&AliasesList::Remove);

			setSizeGripEnabled(true);
		}

		void AliasesList::Populate(const QStringList &names)
		{
			list.clear();
			for (const QString &name : names) list.addItem(name);
		}

		QStringList AliasesList::Aliases() const
		{
			QStringList result;
			auto size=list.count();
			result.reserve(size);
			for (int index=0; index < size; index++) result.append(list.item(index)->text());
			return result;
		}

		void AliasesList::Add()
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

		void AliasesList::Remove()
		{
			QListWidgetItem *item=list.currentItem();
			if (!item) return;
			item=list.takeItem(list.row(item));
			if (item) delete item;
		}

		TriggersList::TriggersList(QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			viewerList(this),
			viewerName(this),
			addViewerName(Text::BUTTON_ADD,this),
			removeViewerName(Text::BUTTON_REMOVE,this),
			redemptionList(this)
		{
			setModal(true);
			setWindowTitle(u"Command Triggers"_s);

			auto topLevelLayout=new QVBoxLayout(this);
			setLayout(topLevelLayout);

			auto contentFrame=new QFrame(this);
			auto contentLayout=new QHBoxLayout(contentFrame);

			auto viewersFrame=new QGroupBox("Viewers",this);
			auto viewersLayout=new QGridLayout(viewersFrame);
			viewerName.setPlaceholderText(u"Name"_s);
			viewersLayout->addWidget(&viewerList,0,0,1,3);
			viewersLayout->addWidget(&viewerName,1,0);
			viewersLayout->addWidget(&addViewerName,1,1);
			viewersLayout->addWidget(&removeViewerName,1,2);
			contentLayout->addWidget(viewersFrame);

			auto *redemptionsFrame=new QGroupBox("Redemptions",this);
			auto *redemptionsLayout=new QGridLayout(redemptionsFrame); // grid layout because vboxlayout created styling issues on KDE (no highlight when focused)
			redemptionsLayout->addWidget(&redemptionList);
			contentLayout->addWidget(redemptionsFrame);

			topLevelLayout->addWidget(contentFrame);

			auto buttons=new QDialogButtonBox(QDialogButtonBox::Save,this);
			auto cancel=buttons->addButton(QDialogButtonBox::Cancel);
			cancel->setDefault(true);
			topLevelLayout->addWidget(buttons);

			connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
			connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
			connect(&addViewerName,&QPushButton::clicked,this,&TriggersList::AddViewerName);
			connect(&removeViewerName,&QPushButton::clicked,this,&TriggersList::RemoveViewerName);

			setSizeGripEnabled(true);
		}

		void TriggersList::Populate(const QStringList &viewerNames,const QString redemptionTitle,const QStringList &availableRedemptionTitles)
		{
			viewerList.clear();
			for (const QString &name : viewerNames) viewerList.addItem(name);
			for (const QString &candidateTitle : availableRedemptionTitles)
			{
				redemptionList.addItem(candidateTitle);
				if (redemptionTitle == candidateTitle)
					redemptionList.setCurrentRow(redemptionList.count()-1);
			}
		}

		QStringList TriggersList::ViewerNames() const
		{
			QStringList result;
			auto size=viewerList.count();
			result.reserve(size);
			for (int index=0; index < size; index++) result.append(viewerList.item(index)->text());
			return result;
		}

		QString TriggersList::RedemptionName() const
		{
			auto selection=redemptionList.currentItem();
			return selection ? selection->text() : QString{};
		}

		void TriggersList::AddViewerName()
		{
			const QString candidate=viewerName.text();
			if (!viewerList.findItems(candidate,Qt::MatchExactly).isEmpty())
			{
				QMessageBox duplicateDialog;
				duplicateDialog.setWindowTitle("Duplicate Name");
				duplicateDialog.setText("Viewer already exists in the list");
				duplicateDialog.setIcon(QMessageBox::Warning);
				duplicateDialog.setStandardButtons(QMessageBox::Ok);
				duplicateDialog.setDefaultButton(QMessageBox::Ok);
				duplicateDialog.exec();
				return;
			}

			viewerList.addItem(candidate);
			viewerName.clear();
		}

		void TriggersList::RemoveViewerName()
		{
			QListWidgetItem *item=viewerList.currentItem();
			if (!item) return;
			item=viewerList.takeItem(viewerList.row(item));
			if (item) delete item;
		}

		Entry::Entry(const Command &command,const QStringList &availableRedemptionTitles,Feedback::Error &errorReport,QWidget *parent) : QWidget(parent),
			layout(this),
			details(this),
			detailsLayout(nullptr),
			header(this),
			availableRedemptionTitles(availableRedemptionTitles),
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

			name=command.Name();
			description=command.Description();
			protect=command.Protected();
			path=command.Path();
			random=command.Random();
			duplicates=command.Duplicates();
			message=command.Message();
			triggers={
				.viewers=command.Viewers(),
				.redemption=command.Redemption()
			};

			switch (command.Type())
			{
			case CommandType::NATIVE:
			{
				type=static_cast<int>(Type::NATIVE);
				break;
			}
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

			connect(&header,&QPushButton::clicked,this,&Entry::ToggleFold);
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
			return aliases.Value();
		}

		void Entry::Aliases(const QStringList &names)
		{
			aliases=names;
			UpdateHeader();
		}

		QStringList Entry::ViewerNameTriggers() const
		{
			return triggers.Value().viewers;
		}

		QString Entry::RedemptionTrigger() const
		{
			return triggers.Value().redemption;
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
				path.Show();
				type.Show();
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
			widget->setFocusPolicy(Qt::StrongFocus);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,3,0,1,2);

			// connect before setCurrentIndex() so we fire an initial TypeChanged
			// this will enable and disable other fields appropriate to the type
			connect(widget,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&Entry::TypeChanged,Qt::QueuedConnection);
			widget->setCurrentIndex(type);
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
			widget->setText(u"&Protect"_s);
			widget->setChecked(protect);
			widget->setFocusPolicy(Qt::ClickFocus);
			connect(widget,&QCheckBox::checkStateChanged,this,&Entry::UpdateProtect);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,0,3);
		}

		void Entry::SetUpRandomCheckBox(QCheckBox *widget)
		{
			widget->setChecked(random);
			widget->setText(u"&Random"_s);
			widget->setFocusPolicy(Qt::ClickFocus);
			connect(widget,&QCheckBox::checkStateChanged,this,&Entry::RandomChanged);
			connect(widget,&QCheckBox::checkStateChanged,this,&Entry::UpdateRandom);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,3,2,1,1);
		}

		void Entry::SetUpDuplicatesCheckBox(QCheckBox *widget)
		{
			widget->setChecked(duplicates);
			widget->setEnabled(random);
			widget->setText(u"&Duplicates"_s);
			widget->setFocusPolicy(Qt::ClickFocus);
			connect(widget,&QCheckBox::checkStateChanged,this,&Entry::UpdateDuplicates);
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
			widget->setText(u"&"_s+Text::BROWSE);
			widget->setFocusPolicy(Qt::ClickFocus);
			connect(widget,&QPushButton::clicked,this,&Entry::Browse);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,2,3,1,1);
		}

		void Entry::SetUpAliasesButton(QPushButton *widget)
		{
			widget->setText(u"&Aliases"_s);
			widget->setFocusPolicy(Qt::ClickFocus);
			connect(widget,&QPushButton::clicked,this,&Entry::SelectAliases);
			widget->installEventFilter(this);
			detailsLayout.addWidget(widget,0,1);
		}

		void Entry::SetUpTriggersButton(QPushButton *widget)
		{
			widget->setText(u"&Triggers"_s);
			widget->setFocusPolicy(Qt::ClickFocus);
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
				name.Enable(true);
				description.Enable(true);
				aliases.Enable(false);
				protect.Enable(true);
				path.Enable(false);
				browse.Enable(false);
				random.Enable(false);
				message.Visible(false);
				return;
			}

			if (static_cast<UI::Commands::Type>(static_cast<int>(type)) == Type::NATIVE)
			{
				// type's visibility is not set here because you can't change a type
				// to native in the editor
				name.Enable(false);
				description.Enable(false);
				aliases.Enable(true);
				protect.Enable(false);
				path.Enable(false);
				browse.Enable(false);
				type.Enable(false);
				random.Enable(false);
				message.Visible(false);
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
			if (!candidate.isEmpty()) (*path)->setText(candidate);
		}

		void Entry::SelectAliases()
		{
			try
			{
				UI::Commands::AliasesList *dialog=new UI::Commands::AliasesList(this);
				dialog->Populate(aliases.Value());
				dialog->resize([]()->QSize {
					QSize screenSize=QSize(QGuiApplication::primaryScreen()->geometry().size());
					int shortestSide=std::min(screenSize.width(),screenSize.height())/2;
					return {shortestSide,shortestSide};
				}());
				connect(dialog,&QDialog::accepted,dialog,[dialog,this]() {
					aliases=dialog->Aliases();
				});
				connect(dialog,&QDialog::accepted,this,QOverload<>::of(&Entry::UpdateHeader));
				connect(dialog,&QDialog::accepted,dialog,&UI::Commands::AliasesList::deleteLater);
				dialog->show();
			}

			catch (const std::bad_alloc &exception)
			{
				QCoreApplication::exit(1);
			}
		}

		void Entry::SelectTriggers()
		{
			try
			{
				UI::Commands::TriggersList *dialog=new UI::Commands::TriggersList(this);
				dialog->Populate(triggers.Value().viewers,triggers.Value().redemption,availableRedemptionTitles);
				dialog->resize([]()->QSize {
					QSize screenSize=QSize(QGuiApplication::primaryScreen()->geometry().size());
					int shortestSide=std::min(screenSize.width(),screenSize.height());
					return {shortestSide/2,shortestSide/3};
				}());
				connect(dialog,&QDialog::accepted,dialog,[dialog,this]() {
					triggers={
						.viewers=dialog->ViewerNames(),
						.redemption=dialog->RedemptionName()
					};
				});
				connect(dialog,&QDialog::accepted,dialog,&UI::Commands::TriggersList::deleteLater);
				dialog->show();
			}

			catch (const std::bad_alloc &exception)
			{
				QCoreApplication::exit(1);
			}
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

		Dialog::Dialog(std::vector<const Command*> commands,QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
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
			statusBar(this),
			commands(std::move(commands))
		{
			setStyleSheet("QFrame { background-color: palette(window); } QScrollArea, QWidget#commands { background-color: palette(base); } QListWidget:enabled, QTextEdit:enabled { background-color: palette(base); }");
			setModal(true);
			setWindowTitle("Commands Editor");

			try
			{
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

				QSize screenSize=QSize(QGuiApplication::primaryScreen()->geometry().size());
				resize(screenSize.width()*0.75,screenSize.height()/2);
			}

			catch (const std::bad_alloc &exception)
			{
				QCoreApplication::exit(1);
			}
		}

		bool Dialog::event(QEvent *event)
		{
			try
			{
				// request list of redemptions for showing in triggers dialog
				if (event->type() == QEvent::Polish)
				{
					auto transaction=new Subsystem::Interchange::Transaction(QStringList{});
					connect(transaction,&Subsystem::Interchange::Transaction::AcknowledgeClosure,this,[this](const QVariant &subject) {
						availableRedemptionTitles=subject.toStringList();

						entriesFrame.setUpdatesEnabled(false);
						QStringList failedEntries;
						std::pair<int,Entry*> entryWithLongestHeader(0,nullptr);
						for (auto command : commands)
						{
							if (command->Parent()) continue; // throw aliases away because we'll build them from the children of the parent

							try
							{
								const QString commandName=command->Name();
								int headerLength=commandName.size();
								auto [entryIterator,inserted]=entries.try_emplace(commandName,nullptr);
								if (!inserted)
								{
									failedEntries.append(commandName);
									continue;
								}
								auto entry=new Entry(*command,availableRedemptionTitles,errorReport,&entriesFrame);
								connect(entry,&Entry::Help,&help,&QTextEdit::setText);
								connect(&errorReport,&Feedback::Error::Clear,&save,&QPushButton::setEnabled);
								connect(&errorReport,&Feedback::Error::Count,&errorBox,&QGroupBox::setVisible);
								connect(&errorReport,&Feedback::Error::ReportProblem,&errorMessages,&QLabel::setText);
								scrollLayout.addWidget(entry);
								entryIterator->second=entry;

								if (!command->Children().empty())
								{
									QStringList aliases;
									aliases.reserve(command->Children().size());
									for (const auto &childName : command->Children() | std::views::transform(&Command::Name))
									{
										aliases.append(childName);
										headerLength+=childName.size();
									}
									entry->Aliases(aliases);
								}

								if (headerLength > entryWithLongestHeader.first) entryWithLongestHeader={headerLength,entry};
							}

							catch (const std::logic_error &exception)
							{
								statusBar.showMessage(exception.what());
							}
						}
						entriesFrame.setUpdatesEnabled(true);

						if (entryWithLongestHeader.first > 0)
						{
							entryWithLongestHeader.second->ToggleFold();
							QMetaObject::invokeMethod(this,&QDialog::adjustSize,Qt::QueuedConnection);
						}

						if (!failedEntries.isEmpty()) statusBar.showMessage(u"Failed to load %1 for %2"_s.arg(StringConvert::NumberAgreement("command","commands",NumberConvert::Positive(failedEntries.size())),failedEntries.join(", ")));
					});

					// it is possible for polish to be called too soon,
					// before main has a chance to connect the receiver,
					// so ensure it waits until control returns to the event loop
					QMetaObject::invokeMethod(this,&UI::Commands::Dialog::RequestRedemptionList,Qt::QueuedConnection,transaction);
				}
			}

			catch (const std::bad_alloc &exception)
			{
				QCoreApplication::exit(1);
				event->accept();
				return true;
			}

			return QDialog::event(event);
		}

		void Dialog::Add()
		{
			const QString name=QInputDialog::getText(this,"New Command","Please provide a name for the new command.");
			if (name.isEmpty())
			{
				statusBar.showMessage("New command's name was left empty");
				return;
			}
			auto [entryIterator,inserted]=entries.try_emplace(name,nullptr);
			if (!inserted)
			{
				statusBar.showMessage("Failed to create a new entry for the command");
				return;
			}
			auto entry=new Entry(
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
					{},
					false
				},
				availableRedemptionTitles,
				errorReport,
				this
			);
			scrollLayout.addWidget(entry);
			entryIterator->second=entry;
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
			// using std::deque here to minimize resizing and eliminate relocation (Command objects don't tolerate this well)
			// because aliases are nested, there isn't an efficient way to calculate and reserve memory upfront
			std::deque<Command> commands;

			for (const auto& [name,entry] : entries)
			{
				auto &command=commands.emplace_back(
					entry->Name(),
					entry->Description(),
					entry->Type(),
					entry->Random(),
					entry->Duplicates(),
					entry->Path(),
					entry->Filters(),
					entry->Message(),
					entry->RedemptionTrigger(),
					entry->ViewerNameTriggers(),
					entry->Protected()
				);

				const auto aliases=entry->Aliases();
				for (const QString &alias : aliases)
					commands.emplace_back(alias,&command);
			}

			emit Save(commands);
			accept();
		}
	}
}
