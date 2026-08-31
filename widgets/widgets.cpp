#include <QScrollBar>
#include <QShowEvent>
#include <QEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QFontDialog>
#include <QVideoWidget>
#include <QTableView>
#include <QHeaderView>
#include <QWindow>
#include <QScreen>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextFrame>
#include <algorithm>
#include "globals.h"
#include "widgets/widgets.h"

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

		Help::Help(QWidget *parent) : QGroupBox(u"Help"_s,parent), text(this)
		{
			auto topLevelLayout=new QVBoxLayout(this);
			text.setEnabled(false);
			text.setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding));
			text.setStyleSheet(QStringLiteral("border: none; color: palette(window-text);"));
			auto textPalette=text.palette();
			textPalette.setColor(text.backgroundRole(),palette().color(backgroundRole()));
			text.setPalette(textPalette);
			text.setAutoFillBackground(true);
			topLevelLayout->addWidget(&text);

			connect(this,&UI::Feedback::Help::Message,&text,&QTextEdit::setText);
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

			auto buttons=new QDialogButtonBox(this);
			auto okay=buttons->addButton(QDialogButtonBox::Ok);
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
			tabs(this),
			newPlaylist(style()->standardIcon(QStyle::SP_FileDialogNewFolder),"New Playlist",this),
			buttons(this),
			add(Text::BUTTON_ADD,this),
			remove(Text::BUTTON_REMOVE,this),
			discard(Text::BUTTON_DISCARD,this),
			save(Text::BUTTON_SAVE,this),
			options(this),
			optionsLayout(&options),
			mediaControls(this),
			mediaControlsLayout(&mediaControls),
			volume(Qt::Horizontal,&mediaControls),
			start(style()->standardIcon(QStyle::SP_MediaPlay),"Play",&mediaControls),
			stop(style()->standardIcon(QStyle::SP_MediaPause),"Pause",&mediaControls),
			playlistNames(this),
			initialAddFilesPath(Filesystem::HomePath().absolutePath())
		{
			setLayout(&layout);
			
			for (const auto &[name,paths] : static_cast<const std::unordered_map<QString,QStringList>&>(files))
			{
				AddTab(name,paths);
				if (name == files.ListName() && !paths.empty()) initialAddFilesPath={paths.first()};
			}
			tabs.setCurrentWidget(tabs.findChild<QWidget*>(files.ListName()));
			tabs.setCornerWidget(&newPlaylist);
			tabs.setTabsClosable(true);
			connect(&tabs,&QTabWidget::tabCloseRequested,this,&Dialog::RemoveTab);
			connect(&newPlaylist,&QPushButton::pressed,this,&Dialog::AddPlaylist);
			layout.addWidget(&tabs);

			mediaControlsLayout.addWidget(&stop,0);
			mediaControlsLayout.addWidget(&start,0);
			QPushButton *volumeIndicator=new QPushButton(style()->standardIcon(QStyle::SP_MediaVolumeMuted),{},&mediaControls);
			volumeIndicator->setFlat(true);
			mediaControlsLayout.addWidget(volumeIndicator);
			mediaControlsLayout.addWidget(&volume,1);
			volumeIndicator=new QPushButton(style()->standardIcon(QStyle::SP_MediaVolume),{},&mediaControls);
			volumeIndicator->setFlat(true);
			mediaControlsLayout.addWidget(volumeIndicator);
			optionsLayout.addWidget(&mediaControls);
			connect(&volume,&QSlider::valueChanged,this,&Dialog::Volume);
			connect(&start,&QPushButton::pressed,this,QOverload<>::of(&Dialog::Play));
			connect(&stop,&QPushButton::pressed,this,&Dialog::Stop);
			optionsLayout.addWidget(new QLabel(u"Active"_s));
			optionsLayout.addWidget(&playlistNames);
			playlistNames.setCurrentIndex(playlistNames.findText(files.ListName()));
			layout.addWidget(&options);

			buttons.addButton(&save,QDialogButtonBox::AcceptRole);
			buttons.addButton(&discard,QDialogButtonBox::RejectRole);
			buttons.addButton(&add,QDialogButtonBox::ActionRole);
			buttons.addButton(&remove,QDialogButtonBox::ActionRole);
			connect(&buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
			connect(&buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
			connect(this,&QDialog::accepted,this,QOverload<>::of(&Dialog::Save));
			connect(&add,&QPushButton::clicked,this,QOverload<>::of(&Dialog::AddFiles));
			connect(&remove,&QPushButton::clicked,this,&Dialog::RemoveFiles);
			layout.addWidget(&buttons);

			setSizeGripEnabled(true);
		}

		Dialog::Dialog(const File::List &files,const QString currentlyPlayingFile,QWidget *parent): Dialog(files,parent)
		{
			auto table=qobject_cast<QTableView*>(tabs.currentWidget());
			auto model=table->model();
			QModelIndexList matches=model->match(model->index(0,static_cast<int>(Columns::PATH)),Qt::DisplayRole,currentlyPlayingFile,1,Qt::MatchExactly);
			if (matches.isEmpty()) return;
			QModelIndex index=matches.first();
			table->selectRow(index.row());
			table->scrollTo(index,QAbstractItemView::PositionAtCenter);
		}

		void Dialog::showEvent(QShowEvent *event)
		{
			setMinimumWidth(ScreenWidthThird(this));
			QDialog::showEvent(event);
		}

		void Dialog::AddTab(const QString &name,const QStringList &paths)
		{
			QTableView *table=new QTableView();
			table->setObjectName(name);
			table->setSelectionBehavior(QAbstractItemView::SelectRows);
			table->setSelectionMode(QAbstractItemView::ExtendedSelection);
			table->setSortingEnabled(true);
			table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
			table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
			table->setEditTriggers(QAbstractItemView::NoEditTriggers);
			QStandardItemModel *model=new QStandardItemModel(table);
			model->setHorizontalHeaderLabels({"Artist","Album","Title","Path"});
			table->setModel(model);
			tabs.addTab(table,name);
			AddFiles(paths,*table,false);
			playlistNames.addItem(name);
			connect(table,&QTableView::doubleClicked,this,QOverload<const QModelIndex&>::of(&Dialog::Play));
		}

		void Dialog::Save()
		{
			static const QString OPERATION=u"Save Playlist Failed"_s;

			try
			{
				std::unordered_map<QString,QStringList> files;
				QStringList failed;
				for (int tabIndex=0; tabIndex < tabs.count(); tabIndex++)
				{
					auto table=qobject_cast<QTableView*>(tabs.widget(tabIndex));
					auto model=table->model();
					QStringList paths;
					for (int modelIndex= 0; modelIndex < model->rowCount(); modelIndex++) paths.append(model->index(modelIndex,static_cast<int>(Columns::PATH)).data().toString());
					auto [insertedItem,insertionResult]=files.try_emplace(table->objectName(),paths);
					if (!insertionResult) failed.append(table->objectName());
				}
				if (!failed.isEmpty())
				{
					QMessageBox{QMessageBox::Warning,OPERATION,u"The follow files were not added: \n\n"_s+failed.join('\n'),QMessageBox::Ok}.exec();
					return;
				}
				File::List lists(files);
				lists.ListName(playlistNames.currentText());
				emit Save(lists);
			}

			catch (const std::out_of_range &exception)
			{
				QMessageBox{QMessageBox::Warning,OPERATION,u"Memory error: "_s+exception.what(),QMessageBox::Ok}.exec();
			}

			catch (const std::exception &exception)
			{
				QMessageBox{QMessageBox::Warning,OPERATION,u"Unknown error: "_s,QMessageBox::Ok}.exec();
			}
		}

		void Dialog::AddFiles()
		{
			const QStringList paths=QFileDialog::getOpenFileNames(this,Text::DIALOG_TITLE_FILE,initialAddFilesPath.absolutePath(),QString("Songs (*.%1)").arg(Text::FILE_TYPE_AUDIO));
			if (paths.isEmpty()) return;
			initialAddFilesPath={paths.front()};
			AddFiles(paths,*qobject_cast<QTableView*>(tabs.currentWidget()),true);
		}

		void Dialog::AddFile(const QString &path,QStandardItemModel &model)
		{
			Music::ID3::Tag tag=Music::ID3::Tag{path};
			auto artist=tag.Artist();
			auto title=tag.Title();
			if (!title || !artist) return;
			auto album=tag.AlbumTitle();
			model.appendRow({
				new QStandardItem(*artist),
				new QStandardItem(album ? *album : QString{}),
				new QStandardItem(*title),
				new QStandardItem(path)
			});
		}

		void Dialog::AddFiles(const QStringList &paths,QTableView &table,bool failurePrompt)
		{
			table.setSortingEnabled(false);
			QStringList failed;
			for (const QString &file : paths)
			{
				try
				{
					AddFile(file,*qobject_cast<QStandardItemModel*>(table.model()));
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
			table.setSortingEnabled(true);
			table.resizeColumnsToContents();
		}

		void Dialog::AddPlaylist()
		{
			bool ok=false;
			QString playlistName=QInputDialog::getText(this,u"New Playlist"_s,u"Playlist name:"_s,QLineEdit::Normal,{},&ok);
			if (!ok || playlistName.isEmpty()) return;
			AddTab(playlistName,{});
		}

		void Dialog::RemoveFiles()
		{
			QTableView &table=*qobject_cast<QTableView*>(tabs.currentWidget());
			QModelIndexList selectedRows=table.selectionModel()->selectedRows();

			// sort in descending order to avoid index shifting
			std::sort(selectedRows.begin(),selectedRows.end(),[](const QModelIndex &a,const QModelIndex &b) {
				return a.row() > b.row();
			});

			for (const QModelIndex &index : selectedRows)
			{
				table.model()->removeRow(index.row());
			}
		}

		void Dialog::RemoveTab(int index)
		{
			auto tab=tabs.widget(index);
			auto confirmation=QMessageBox::question(this,"Confirm Removal",QString(R"(Are you sure you wish to remove playlist "%1")").arg(tab->objectName()),QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
			if (confirmation == QMessageBox::No) return;
			tabs.removeTab(index);
			int playlistNameIndex=playlistNames.findText(tab->objectName());
			delete tab;
			if (playlistNameIndex < 0) return;
			playlistNames.removeItem(playlistNameIndex);
		}

		void Dialog::Play()
		{
			const QModelIndex frontIndex=qobject_cast<QTableView*>(tabs.currentWidget())->model()->index(0,0);
			if (!frontIndex.isValid()) return;
			emit Play(QUrl::fromLocalFile(frontIndex.data().toString()));
		}

		void Dialog::Play(const QModelIndex &index)
		{
			if (!index.isValid())
			{
				Play();
				return;
			}
			emit Play(QUrl::fromLocalFile(index.siblingAtColumn(static_cast<int>(Columns::PATH)).data().toString()));
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
