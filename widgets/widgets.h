#pragma once

#include <QTextEdit>
#include <QTextBlockUserData>
#include <QTimer>
#include <QPropertyAnimation>
#include <QLineEdit>
#include <QListWidget>
#include <QTableWidget>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QGroupBox>
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
#include <unordered_set>
#include <deque>
#include <concepts>
#include "widgets/lazy.h"
#include "entities.h"
#include "subsystem/subsystem.h"

namespace StyleSheet
{
	template<Concept::Widget TWidget> const QString Colors(const QColor &foreground,const QColor &background)
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
			TWidget::staticMetaObject.className()
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
	void Append(const QString &text,const QString &id);
	void Remove(const QString &id);
protected:
	std::unordered_map<QString,QTextFrame*> frames;
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

class SingleSelectionListWidget: public QListWidget
{
	Q_OBJECT
public:
	SingleSelectionListWidget(QWidget *parent);
protected:
	void mousePressEvent(QMouseEvent *event) override;
};

namespace UI
{
	QString OpenVideo(QWidget *parent,QString initialPath=QString());
	QString OpenAudio(QWidget *parent,QString initialPath=QString());

	class Color : public QLabel
	{
		Q_OBJECT
	public:
		Color(QWidget *parent,const QString &color);
		void Set(const QString &color);
	};

	namespace Feedback
	{
		class Error : public QObject
		{
			Q_OBJECT
		private:
			using ErrorList=std::unordered_set<QString>;
		public:
			Error();
			void SwapTrackingName(const QString &oldName,const QString &newName);
		protected:
			ErrorList errors;
			void CompileErrorMessages();
		signals:
			void Clear(bool clear);
			void Count(int errors);
			void ReportProblem(const QString &message);
		public slots:
			void Valid(QWidget *widget);
			void Invalid(QWidget *widget);
			void ValidateFont(QWidget *widget,const QString &family,const int pointSize);
		};

		class Help : public QGroupBox
		{
			Q_OBJECT
		public:
			Help(QWidget *parent);
		protected:
			QTextEdit text;
		signals:
			void Message(const QString &message);
		};
	}

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

		class AliasesList : public QDialog
		{
			Q_OBJECT
		public:
			AliasesList(QWidget *parent);
			void Populate(const QStringList &names);
			QStringList Aliases() const;
		protected:
			QListWidget list;
			QLineEdit name;
			QPushButton add;
			QPushButton remove;
			void Add();
			void Remove();
		};

		class TriggersList : public QDialog
		{
			Q_OBJECT
		public:
			TriggersList(QWidget *parent);
			void Populate(const QStringList &viewerNames,const QString redemptionTitlem,const QStringList &availableRedemptionTitles);
			QStringList ViewerNames() const;
			QString RedemptionName() const;
		protected:
			QListWidget viewerList;
			QLineEdit viewerName;
			QPushButton addViewerName;
			QPushButton removeViewerName;
			SingleSelectionListWidget redemptionList;
			void AddViewerName();
			void RemoveViewerName();
		};

		class Entry : public QWidget
		{
			Q_OBJECT

			struct Triggers
			{
				QStringList viewers;
				QString redemption;
			};
		public:
			Entry(const Command &command,const QStringList &availableRedemptionTitles,Feedback::Error &errorReport,QWidget *parent);
			QString Name() const;
			QString Description() const;
			QStringList Aliases() const;
			void Aliases(const QStringList &names);
			QStringList ViewerNameTriggers() const;
			QString RedemptionTrigger() const;
			QString Path() const;
			QStringList Filters() const;
			CommandType Type() const;
			bool Random() const;
			bool Duplicates() const;
			QString Message() const;
			bool Protected() const;
			void ToggleFold();
		protected:
			QGridLayout layout;
			QFrame details;
			QGridLayout detailsLayout;
			QPushButton header;
			const QStringList &availableRedemptionTitles; // not worried about const here because QObjects are non-copyable anyway
			LazyWidget<QLineEdit> name;
			LazyWidget<QLineEdit> description;
			GraftedLazyWidget<QPushButton,QStringList> aliases;
			GraftedLazyWidget<QPushButton,Triggers> triggers;
			LazyWidget<QLineEdit> path;
			LazyWidget<QPushButton> browse;
			LazyWidget<QComboBox> type;
			LazyWidget<QCheckBox> random;
			LazyWidget<QCheckBox> duplicates;
			LazyWidget<QCheckBox> protect;
			LazyWidget<QTextEdit> message;
			Feedback::Error &errorReport;
			void UpdateName();
			void UpdateDescription(const QString &text);
			void UpdateMessage();
			void UpdatePath(const QString &text);
			void UpdateProtect(int state);
			void UpdateRandom(int state);
			void UpdateDuplicates(int state);
			void Browse();
			void SelectAliases();
			void SelectTriggers();
			void UpdateHeader();
			void SetUpCommandNameTextEdit(QLineEdit *widget);
			void SetUpDescriptionTextEdit(QLineEdit *widget);
			void SetUpTypeList(QComboBox *widget);
			void SetUpPathTextEdit(QLineEdit *widget);
			void SetUpProtectCheckBox(QCheckBox *widget);
			void SetUpRandomCheckBox(QCheckBox *widget);
			void SetUpDuplicatesCheckBox(QCheckBox *widget);
			void SetUpMessageTextEdit(QTextEdit *widget);
			void SetUpBrowseButton(QPushButton *widget);
			void SetUpAliasesButton(QPushButton *widget);
			void SetUpTriggersButton(QPushButton *widget);
			bool eventFilter(QObject *object,QEvent *event) override;
			static QString BuildErrorTrackingName(const QString &commandName,const QString message);
			static QString BuildErrorTrackingName(const QString &commandName);
		signals:
			void Help(const QString &text);
		protected slots:
			void UpdateHeader(const QString &commandName);
			bool ValidateName(const QString &text);
			bool ValidatePath(const QString &text);
			bool ValidateMessage();
			void RandomChanged(const int state);
			void TypeChanged(int index);
		};

		class Dialog : public QDialog
		{
			Q_OBJECT
		public:
			Dialog(std::vector<const Command*> commands,QWidget *parent);
		protected:
			QWidget entriesFrame;
			QVBoxLayout scrollLayout;
			Feedback::Help help;
			Feedback::Error errorReport;
			QLabel labelFilter;
			QComboBox filter;
			QLineEdit search;
			QDialogButtonBox buttons;
			QPushButton discard;
			QPushButton save;
			QPushButton newEntry;
			QGroupBox errorBox;
			QLabel errorMessages;
			QStatusBar statusBar;
			std::vector<const Command*> commands;
			QStringList availableRedemptionTitles;
			std::unordered_map<QString,Entry*> entries;
			bool event(QEvent *event) override;

			struct SearchNode
			{
				std::unordered_map<QChar,std::shared_ptr<SearchNode>> children;
				std::unordered_set<const Entry*> matches;
			};

			class SearchEngine
			{
			public:
				SearchEngine(): root(new SearchNode()) { }
				~SearchEngine();
				void PopulateEntry(const Entry *entry);
				std::unordered_set<const Entry*> Search(const QString &key);
			protected:
				SearchNode *root;
				void PopulateSuffixes(const QString &text,const Entry *entry);
			};

			SearchEngine searchEngine;
		signals:
			void Save(const std::deque<Command> &commands);
			void RequestRedemptionList(Subsystem::Interchange::Transaction *transaction);
		public slots:
			void FilterChanged(int index);
		protected slots:
			void Add();
			void Save();
			void Search(const QString &key);
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
				Category(const QString &name);
				virtual void Save()=0;
			protected:
				QVBoxLayout verticalLayout;
				QPushButton header;
				QFrame *details;
				QGridLayout detailsLayout;
				QLabel* Label(const QString &text);
				QLabel* Subheading(const QString &text);
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
				Channel(Settings::Channel &settings,std::shared_ptr<Feedback::Error> errorReport);
				void Save() override;
			protected:
				QLineEdit name;
				QCheckBox protection;
				Settings::Channel &settings;
				std::shared_ptr<Feedback::Error> errorReport;
				bool eventFilter(QObject *object,QEvent *event) override;
			signals:
				void Changed();
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
				Window(Settings settings);
				void Save() override;
			protected:
				QLineEdit backgroundColor;
				Color previewBackgroundColor;
				QPushButton selectBackgroundColor;
				QSpinBox width;
				QSpinBox height;
				Settings settings;
				void PickBackgroundColor();
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
				Status(Settings settings,std::shared_ptr<Feedback::Error> errorReport);
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
				std::shared_ptr<Feedback::Error> errorReport;
				void PickFont();
				void PickForegroundColor();
				void PickBackgroundColor();
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
				Chat(Settings settings,std::shared_ptr<Feedback::Error> errorReport);
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
				std::shared_ptr<Feedback::Error> errorReport;
				bool eventFilter(QObject *object,QEvent *event) override;
			protected slots:
				void PickFont();
				void PickForegroundColor();
				void PickBackgroundColor();
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
				Pane(Settings settings,std::shared_ptr<Feedback::Error> errorReport);
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
				std::shared_ptr<Feedback::Error> errorReport;
				bool eventFilter(QObject *object,QEvent *event) override;
			protected slots:
				void PickFont();
				void PickForegroundColor();
				void PickBackgroundColor();
				void PickAccentColor();
			};

			class Music : public Category
			{
				Q_OBJECT
			public:
				struct Settings
				{
					ApplicationSetting &suppressedVolume;
				};
				Music(Settings settings);
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
				Bot(Settings::Bot &settings,std::shared_ptr<Feedback::Error> errorReport);
				void Save() override;
			protected:
				Settings::Bot &settings;
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
				QSpinBox postRaidEventDelay;
				QSpinBox postRaidEventDelayThreshold;
				QPushButton selectRaidSound;
				QPushButton previewRaidSound;
				QSpinBox inactivityCooldown;
				QSpinBox helpCooldown;
				QLineEdit textWallSound;
				QPushButton selectTextWallSound;
				QPushButton previewTextWallSound;
				QSpinBox textWallThreshold;
				QLineEdit adBreakWarningVideo;
				QPushButton selectAdBreakWarningVideo;
				QPushButton previewAdBreakWarningVideo;
				QSpinBox adBreakWarningLeadTime;
				QLineEdit adBreakFinishedVideo;
				QPushButton selectAdBreakFinishedVideo;
				QPushButton previewAdBreakFinishedVideo;
				QSpinBox adScheduleRefreshInterval;
				QSpinBox monkeyKeyboardBleepRootFrequency;
				QSpinBox monkeyKeyboardBleepLength;
				QSpinBox monkeyKeyboardBloopRootFrequency;
				QSpinBox monkeyKeyboardBloopLength;
				QSlider monkeyKeyboardVolume;
				QLabel monkeyKeyboardVolumeValue;
				std::shared_ptr<Feedback::Error> errorReport;
				bool eventFilter(QObject *object,QEvent *event) override;
			signals:
				void PlayArrivalSound(const QString &name,std::shared_ptr<QImage> profileImage,const QString &audioPath);
				void PlayPortraitVideo(const QString &path);
				void PlayCheerVideo(const QString &chatter,const unsigned int count,const QString &message,const QString &path);
				void PlaySubscriptionSound(const QString &chatter,const QString &path);
				void PlayRaidSound(const QString &chatter,const unsigned int raiders,const QString &path);
				void PlayTextWallSound(const QString &message,const QString &path);
				void PlayAdBreakWarningVideo(const QString &path);
				void PlayAdBreakFinishedVideo(const QString &path);
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
				void OpenAdBreakWarningVideo();
				void PlayAdBreakWarningVideo();
				void OpenAdBreakFinishedVideo();
				void PlayAdBreakFinishedVideo();
				void MonkeyKeyboardVolumeChanged(int value);
				void ValidateArrivalSound(const QString &path);
				void ValidatePortraitVideo(const QString &path);
				void ValidateCheerVideo(const QString &path);
				void ValidateSubscriptionSound(const QString &path);
				void ValidateRaidSound(const QString &path);
				void ValidateTextWallSound(const QString &path);
				void ValidateAdBreakWarningVideo(const QString &path);
				void ValidateAdBreakFinishedVideo(const QString &path);
			};

			class Pulsar: public Category
			{
				Q_OBJECT
			public:
				Pulsar(Settings::Pulsar &settings);
				void Save() override;
			protected:
				Settings::Pulsar &settings;
				QCheckBox subsystemEnabled;
				QSpinBox reconnectDelay;
				bool eventFilter(QObject *object,QEvent *event) override;
			};

			class Log : public Category
			{
				Q_OBJECT
			public:
				struct Settings
				{
					ApplicationSetting &directory;
				};
				Log(Settings settings,std::shared_ptr<Feedback::Error> errorReport);
				void Save() override;
			protected:
				QLineEdit directory;
				QPushButton selectDirectory;
				Settings settings;
				std::shared_ptr<Feedback::Error> errorReport;
				bool eventFilter(QObject *object,QEvent *event) override;
			protected slots:
				void OpenDirectory();
				void ValidateDirectory(const QString &path);
			};

			class Security : public Category
			{
				Q_OBJECT
			public:
				Security(::Security &settings,std::shared_ptr<Feedback::Error> errorReport);
				void Save() override;
			protected:
				QLineEdit administrator;
				QLineEdit clientID;
				QLineEdit token;
				QLineEdit callbackURL;
				QLineEdit permissions;
				QPushButton selectPermissions;
				::Security &settings;
				std::shared_ptr<Feedback::Error> errorReport;
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
			Dialog(std::vector<Categories::Category*> categories,QWidget *parent);
		protected:
			QWidget entriesFrame;
			Feedback::Help help;
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
		enum class Columns
		{
			ARTIST,
			ALBUM,
			TITLE,
			PATH,
			MAX
		};

		class Dialog : public QDialog
		{
			Q_OBJECT
		public:
			Dialog(const File::List &files,QWidget *parent);
			Dialog(const File::List &files,const QString currentlyPlayingFile,QWidget *parent);
		protected:
			QVBoxLayout layout;
			QTabWidget tabs;
			QPushButton newPlaylist;
			QDialogButtonBox buttons;
			QPushButton add;
			QPushButton remove;
			QPushButton discard;
			QPushButton save;
			QFrame options;
			QHBoxLayout optionsLayout;
			QFrame mediaControls;
			QHBoxLayout mediaControlsLayout;
			QSlider volume;
			QPushButton start;
			QPushButton stop;
			QComboBox playlistNames;
			QDir initialAddFilesPath; // "initial" in that this is the first place we look when we open the browse dialog
			void AddTab(const QString &name,const QStringList &paths);
			void Save();
			void AddFile(const QString &path,QStandardItemModel &model);
			void AddFiles(const QStringList &paths,QTableView &table,bool failurePrompt);
			void AddPlaylist();
			void showEvent(QShowEvent *event) override;
			static const int COLUMN_COUNT;
		signals:
			void Save(const File::List &files);
			void Play(QUrl songPath);
			void Stop();
			void Volume(int value);
		protected slots:
			void AddFiles();
			void RemoveFiles();
			void RemoveTab(int index);
			void Play();
			void Play(const QModelIndex &index);
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
				return dialog;
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
