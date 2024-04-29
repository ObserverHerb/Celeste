#pragma once

#include <QTextEdit>
#include <QTimer>
#include <QPropertyAnimation>
#include <QLineEdit>
#include <QListWidget>
#include <QTableWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QStatusBar>
#include <QSizeGrip>
#include <QDialog>
#include <QDir>
#include "entities.h"

namespace StyleSheet
{
	template<Concept::Widget T> const QString Colors(const QColor &foreground,const QColor &background)
	{
		return QString{"%9 { color: rgba(%1,%2,%3,%4); background-color: rgba(%5,%6,%7,%8); }"}.arg(
			StringConvert::Integer(foreground.red()),
			StringConvert::Integer(foreground.green()),
			StringConvert::Integer(foreground.blue()),
			StringConvert::Integer(foreground.alpha()),
			StringConvert::Integer(background.red()),
			StringConvert::Integer(background.green()),
			StringConvert::Integer(background.blue()),
			StringConvert::Integer(background.alpha()),
			T::staticMetaObject.className()
		);
	}
}

class StaticTextEdit : public QTextEdit
{
	Q_OBJECT
public:
	StaticTextEdit(QWidget *parent);
protected:
	void contextMenuEvent(QContextMenuEvent *event) override;
signals:
	void ContextMenu(QContextMenuEvent *event);
};

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
	void Valid(QWidget *widget,bool valid);
	void Require(QWidget *widget,bool empty);
	QString OpenVideo(QWidget *parent,QString initialPath=QString());
	QString OpenAudio(QWidget *parent,QString initialPath=QString());

	class Help : public QTextEdit
	{
		Q_OBJECT
	public:
		Help(QWidget *parent);
	};

	class Color : public QLabel
	{
		Q_OBJECT
	public:
		Color(QWidget *parent,const QString &color);
		void Set(const QString &color);
	};

	namespace Text
	{
		inline const char *BROWSE="Browse";
		inline const char *CHOOSE="Pick";
		inline const char *PREVIEW="Preview";
		inline const char *DIALOG_TITLE_FILE="Choose File";
		inline const char *DIALOG_TITLE_DIRECTORY="Choose Directory";
		inline const char *DIALOG_TITLE_FONT="Choose Font";
		inline const char *FILE_TYPE_VIDEO="mp4";
		inline const char *FILE_TYPE_AUDIO="mp3";
		inline const char *BUTTON_SAVE="&Save";
		inline const char *BUTTON_DISCARD="&Discard";
		inline const char *BUTTON_APPLY="&Apply";
		inline const char *BUTTON_ADD="&Add";
		inline const char *BUTTON_REMOVE="&Remove";
		inline const char *BUTTON_CLOSE="&Close";
	}

	namespace Security
	{
		class Scopes : public QDialog
		{
			Q_OBJECT
		public:
			Scopes(QWidget *parent);
			QStringList operator()();
		protected:
			QGridLayout layout;
			QListWidget list;
			QStringList scopes;
			void Save();
		};
	}

	namespace Commands
	{
		enum class Type
		{
			INVALID=-2,
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

		class NamesList : public QDialog
		{
			Q_OBJECT
		public:
			NamesList(const QString &title,const QString &placeholder,QWidget *parent);
			void Populate(const QStringList &names);
			QStringList operator()() const;
		protected:
			QGridLayout layout;
			QListWidget list;
			QLineEdit name;
			QPushButton add;
			QPushButton remove;
			void Add();
			void Remove();
			void hideEvent(QHideEvent *event) override;
		signals:
			void Finished();
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
			QStringList Triggers() const;
			QString Path() const;
			QStringList Filters() const;
			CommandType Type() const;
			bool Random() const;
			bool Duplicates() const;
			QString Message() const;
			bool Protected() const;
			void ToggleFold();
		protected:
			QString commandName;
			QString commandDescription;
			bool commandProtect;
			QString commandPath;
			bool commandRandom;
			bool commandDuplicates;
			QString commandMessage;
			UI::Commands::Type commandType;
			QStringList commandAliases;
			QStringList commandTriggers;
			QGridLayout layout;
			QFrame *details;
			QGridLayout detailsLayout;
			QPushButton header;
			QLineEdit *name;
			QLineEdit *description;
			QPushButton *openAliases;
			QPushButton *openTriggers;
			QLineEdit *path;
			QPushButton *browse;
			QComboBox *type;
			QCheckBox *random;
			QCheckBox *duplicates;
			QCheckBox *protect;
			QTextEdit *message;
			UI::Commands::NamesList *aliases;
			UI::Commands::NamesList *triggers;
			void UpdateName(const QString &text);
			void UpdateDescription(const QString &text);
			void UpdateMessage();
			void UpdatePath(const QString &text);
			void UpdateProtect(int state);
			void UpdateRandom(int state);
			void UpdateDuplicates(int state);
			void UpdateAliases();
			void UpdateTriggers();
			void Native();
			void Pulsar();
			void Browse();
			void UpdateHeader();
			void TryBuildUI();
			void ValidatePath(const QString &text,bool random,const enum Type type);
			bool eventFilter(QObject *object,QEvent *event) override;
		signals:
			void Help(const QString &text);
		protected slots:
			void UpdateHeader(const QString &commandName);
			void ValidateName(const QString &text);
			void ValidateDescription(const QString &text);
			void ValidatePath(const QString &text);
			void ValidateMessage();
			void RandomChanged(const int state);
			void TypeChanged(int index);
		};

		class Dialog : public QDialog
		{
			Q_OBJECT
		public:
			Dialog(const Command::Lookup &commands,QWidget *parent);
		protected:
			QWidget entriesFrame;
			QVBoxLayout scrollLayout;
			Help help;
			QLabel labelFilter;
			QComboBox filter;
			QDialogButtonBox buttons;
			QPushButton discard;
			QPushButton save;
			QPushButton newEntry;
			QStatusBar statusBar;
			std::unordered_map<QString,Entry*> entries;
			void PopulateEntries(const Command::Lookup &commands);
			void Add();
			void Save();
		signals:
			void Save(const Command::Lookup &commands);
		public slots:
			void FilterChanged(int index);
		};
	}

	namespace Options
	{
		namespace Categories
		{
			class Category : public QFrame
			{
				Q_OBJECT
			public:
				Category(QWidget *parent,const QString &name);
				virtual void Save()=0;
			protected:
				QVBoxLayout verticalLayout;
				QPushButton header;
				QFrame *details;
				QGridLayout detailsLayout;
				QLabel* Label(const QString &text);
				void Rows(std::vector<std::vector<QWidget*>> widgets);
				virtual bool eventFilter(QObject *object,QEvent *event) override=0;
			signals:
				void Help(const QString &text);
			protected slots:
				void ToggleDetails();
				void PickColor(QLineEdit &control);
			};

			class Channel : public Category
			{
				Q_OBJECT
			public:
				struct Settings
				{
					ApplicationSetting &name;
					ApplicationSetting &protection;
				};
				Channel(QWidget *parent,Settings settings);
				void Save() override;
			protected:
				QLineEdit name;
				QCheckBox protection;
				Settings settings;
				bool eventFilter(QObject *object,QEvent *event) override;
			protected slots:
				void ValidateName(const QString &text);
			};

			class Window : public Category
			{
				Q_OBJECT
			public:
				struct Settings
				{
					ApplicationSetting &backgroundColor;
					ApplicationSetting &dimensions;
				};
				Window(QWidget *parent,Settings settings);
				void Save() override;
			protected:
				QLineEdit backgroundColor;
				QPushButton selectBackgroundColor;
				QSpinBox width;
				QSpinBox height;
				Settings settings;
				bool eventFilter(QObject *object,QEvent *event) override;
			};

			class Status : public Category
			{
				Q_OBJECT
			public:
				struct Settings
				{
					ApplicationSetting font;
					ApplicationSetting fontSize;
					ApplicationSetting foregroundColor;
					ApplicationSetting backgroundColor;
				};
				Status(QWidget *parent,Settings settings);
				void Save() override;
			protected:
				QLineEdit font;
				QSpinBox fontSize;
				QPushButton selectFont;
				QLineEdit foregroundColor;
				Color previewForegroundColor;
				QPushButton selectForegroundColor;
				QLineEdit backgroundColor;
				Color previewBackgroundColor;
				QPushButton selectBackgroundColor;
				Settings settings;
				void PickFont();
				void PickForegroundColor();
				void PickBackgroundColor();
				void ValidateFont(const QString &family,const int pointSize);
				void ValidateFont(const QString &family);
				void ValidateFont(const int pointSize);
				bool eventFilter(QObject *object,QEvent *event) override;
			};

			class Chat : public Category
			{
				Q_OBJECT
			public:
				struct Settings
				{
					ApplicationSetting font;
					ApplicationSetting fontSize;
					ApplicationSetting foregroundColor;
					ApplicationSetting backgroundColor;
					ApplicationSetting statusInterval;
				};
				Chat(QWidget *parent,Settings settings);
				void Save() override;
			protected:
				QLineEdit font;
				QSpinBox fontSize;
				QPushButton selectFont;
				QLineEdit foregroundColor;
				Color previewForegroundColor;
				QPushButton selectForegroundColor;
				QLineEdit backgroundColor;
				Color previewBackgroundColor;
				QPushButton selectBackgroundColor;
				QSpinBox statusInterval;
				Settings settings;
				bool eventFilter(QObject *object,QEvent *event) override;
			protected slots:
				void PickFont();
				void PickForegroundColor();
				void PickBackgroundColor();
				void ValidateFont(const QString &family,const int pointSize);
				void ValidateFont(const QString &family);
				void ValidateFont(const int pointSize);
			};

			class Pane : public Category
			{
				Q_OBJECT
			public:
				struct Settings
				{
					ApplicationSetting font;
					ApplicationSetting fontSize;
					ApplicationSetting foregroundColor;
					ApplicationSetting backgroundColor;
					ApplicationSetting accentColor;
					ApplicationSetting duration;
				};
				Pane(QWidget *parent,Settings settings);
				void Save() override;
			protected:
				QLineEdit font;
				QSpinBox fontSize;
				QPushButton selectFont;
				QLineEdit foregroundColor;
				Color previewForegroundColor;
				QPushButton selectForegroundColor;
				QLineEdit backgroundColor;
				Color previewBackgroundColor;
				QPushButton selectBackgroundColor;
				QLineEdit accentColor;
				Color previewAccentColor;
				QPushButton selectAccentColor;
				QSpinBox duration;
				Settings settings;
				bool eventFilter(QObject *object,QEvent *event) override;
			protected slots:
				void PickFont();
				void PickForegroundColor();
				void PickBackgroundColor();
				void PickAccentColor();
				void ValidateFont(const QString &family,const int pointSize);
				void ValidateFont(const QString &family);
				void ValidateFont(const int pointSize);
			};

			class Music : public Category
			{
				Q_OBJECT
			public:
				struct Settings
				{
					ApplicationSetting &suppressedVolume;
				};
				Music(QWidget *parent,Settings settings);
				void Save() override;
			protected:
				QSpinBox suppressedVolume;
				Settings settings;
				bool eventFilter(QObject *object,QEvent *event) override;
			};

			class Bot : public Category
			{
				Q_OBJECT
			public:
				struct Settings
				{
					ApplicationSetting &arrivalSound;
					ApplicationSetting &portraitVideo;
					ApplicationSetting &cheerVideo;
					ApplicationSetting &subscriptionSound;
					ApplicationSetting &raidSound;
					ApplicationSetting &inactivityCooldown;
					ApplicationSetting &helpCooldown;
					ApplicationSetting &textWallThreshold;
					ApplicationSetting &textWallSound;
				};
				Bot(QWidget *parent,Settings settings);
				void Save() override;
			protected:
				QLineEdit arrivalSound;
				QPushButton selectArrivalSound;
				QPushButton previewArrivalSound;
				QLineEdit portraitVideo;
				QPushButton selectPortraitVideo;
				QPushButton previewPortraitVideo;
				QLineEdit cheerVideo;
				QPushButton selectCheerVideo;
				QPushButton previewCheerVideo;
				QLineEdit subscriptionSound;
				QPushButton selectSubscriptionSound;
				QPushButton previewSubscriptionSound;
				QLineEdit raidSound;
				QPushButton selectRaidSound;
				QPushButton previewRaidSound;
				QSpinBox inactivityCooldown;
				QSpinBox helpCooldown;
				QLineEdit textWallSound;
				QPushButton selectTextWallSound;
				QPushButton previewTextWallSound;
				QSpinBox textWallThreshold;
				Settings settings;
				bool eventFilter(QObject *object,QEvent *event) override;
			signals:
				void PlayArrivalSound(const QString &name,std::shared_ptr<QImage> profileImage,const QString &audioPath);
				void PlayPortraitVideo(const QString &path);
				void PlayCheerVideo(const QString &chatter,const unsigned int count,const QString &message,const QString &path);
				void PlaySubscriptionSound(const QString &chatter,const QString &path);
				void PlayRaidSound(const QString &chatter,const unsigned int raiders,const QString &path);
				void PlayTextWallSound(const QString &message,const QString &path);
			protected slots:
				void OpenArrivalSound();
				void PlayArrivalSound();
				void OpenPortraitVideo();
				void PlayPortraitVideo();
				void OpenCheerVideo();
				void PlayCheerVideo();
				void OpenSubscriptionSound();
				void PlaySubscriptionSound();
				void OpenRaidSound();
				void PlayRaidSound();
				void OpenTextWallSound();
				void PlayTextWallSound();
				void ValidateArrivalSound(const QString &path);
				void ValidatePortraitVideo(const QString &path);
				void ValidateCheerVideo(const QString &path);
				void ValidateSubscriptionSound(const QString &path);
				void ValidateRaidSound(const QString &path);
				void ValidateTextWallSound(const QString &path);
			};

			class Log : public Category
			{
				Q_OBJECT
			public:
				struct Settings
				{
					ApplicationSetting &directory;
				};
				Log(QWidget *parent,Settings settings);
				void Save() override;
			protected:
				QLineEdit directory;
				QPushButton selectDirectory;
				Settings settings;
				bool eventFilter(QObject *object,QEvent *event) override;
			protected slots:
				void OpenDirectory();
				void ValidateDirectory(const QString &path);
			};

			class Security : public Category
			{
				Q_OBJECT
			public:
				Security(QWidget *parent,::Security &settings);
				void Save() override;
			protected:
				QLineEdit administrator;
				QLineEdit clientID;
				QLineEdit token;
				QLineEdit callbackURL;
				QLineEdit permissions;
				QPushButton selectPermissions;
				::Security &settings;
				bool eventFilter(QObject *object,QEvent *event) override;
			protected slots:
				void SelectPermissions();
				void ValidateURL(const QString &text);
			};
		}

		class Dialog : public QDialog
		{
			Q_OBJECT
		public:
			Dialog(QWidget *parent);
			void AddCategory(Categories::Category *category);
		protected:
			QWidget entriesFrame;
			Help help;
			QDialogButtonBox buttons;
			QPushButton discard;
			QPushButton save;
			QPushButton apply;
			QVBoxLayout *scrollLayout;
			std::vector<Categories::Category*> categories;
		signals:
			void Refresh();
		protected slots:
			void Save();
		};
	}

	namespace Metrics
	{
		class Dialog : public QDialog
		{
			Q_OBJECT
		public:
			Dialog(QWidget *parent);
		protected:
			QHBoxLayout layout;
			QListWidget users;
			static const QString TITLE;
			void UpdateTitle();
		public slots:
			void Joined(const QString &user);
			void Acknowledged(const QString &name);
			void Parted(const QString &user);
		};
	}

	namespace VibePlaylist
	{
		const int COLUMN_PATH=3;

		class Dialog : public QDialog
		{
			Q_OBJECT
		public:
			Dialog(const File::List &files,QWidget *parent);
		protected:
			QVBoxLayout layout;
			QTableWidget list;
			QDialogButtonBox buttons;
			QPushButton add;
			QPushButton remove;
			QPushButton discard;
			QPushButton save;
			QMediaPlayer reader;
			const File::List &files;
			QDir initialAddFilesPath;
			void Save();
			void Add(const QString &path);
			void Add(const QStringList &paths,bool failurePrompt);
			void showEvent(QShowEvent *event) override;
			static const int COLUMN_COUNT;
		signals:
			void Save(const File::List &files);
			void Play(QUrl songPath);
		protected slots:
			void Add();
			void Remove();
			void Play(QTableWidgetItem *item);
		};
	}

	namespace EventSubscriptions
	{
		class Dialog : public QDialog
		{
			Q_OBJECT
		public:
			Dialog(QWidget *parent);
		protected:
			QVBoxLayout layout;
			QTableWidget list;
			QDialogButtonBox buttons;
			QPushButton remove;
			QPushButton close;
			static const int COLUMN_COUNT;
			void showEvent(QShowEvent *event) override;
		signals:
			void RequestSubscriptionList();
			void RemoveSubscription(const QString &id);
		public slots:
			void Add(const QString &id,const QString &type,const QDateTime &creationDate,const QString &callbackURL);
			void Removed(const QString &id);
		protected slots:
			void Remove();
		};
	}

	namespace Status
	{
		template <typename T> // TODO: tie this to a concept that says this had to be a derivative of QWidget
		class Window
		{
		public:
			Window(QWidget *parent) : dialog(new QDialog(parent))
			{

				pane=new T(&dialog);
				pane->EnableScrollBar();
				QGridLayout *gridLayout=new QGridLayout(&dialog);
				gridLayout->setContentsMargins(0,0,0,0);
				dialog.setLayout(gridLayout);
				gridLayout->addWidget(pane);
			}

			QDialog& Dialog()
			{
				return &dialog;
			}

			T& Pane()
			{
				return *pane;
			}

			void Open()
			{
				dialog.open();
			}
		protected:
			QDialog dialog;
			T *pane;
		};
	}
}