#include <QHeaderView>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include "widgets/widgets.h"
#include "globals.h"

namespace UI
{
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
				QMessageBox{QMessageBox::Warning,OPERATION,u"Unknown error: "_s+exception.what(),QMessageBox::Ok}.exec();
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
}