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
#include <QButtonGroup>
#include <QRadioButton>
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
	std::optional<QString> OpenVideo(QWidget *parent,const QString &initialPath=QString());
	std::optional<QString> OpenAudio(QWidget *parent,const QString &initialPath=QString());
	std::optional<QString> OpenDirectory(QWidget *parent,const QString &initialPath=QString());
	std::optional<QString> PickColor(QWidget *parent,const QString &initialColor);
	std::optional<std::tuple<QString,int>> PickFont(QWidget *parent,const QString &initialFamily,int initialPointSize);

	namespace Feedback
	{
		class Error: public QObject
		{
			Q_OBJECT
		public:
			Error();
			void SwapTrackingName(const QString &oldName,const QString &newName);
			void ValidStyle(QWidget *widget);
			void InvalidStyle(QWidget *widget);
		protected:
			std::unordered_set<QString> errors;
			void CompileErrorMessages();
		signals:
			void Clear(bool clear);
			void Count(int errors);
			void ReportProblem(const QString &message);
		public slots:
			void StyleSource(bool valid,QWidget *widget);
		};

		class Help: public QGroupBox
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

	class ColorPreview: public QLabel
	{
		Q_OBJECT
	public:
		ColorPreview(QWidget *parent,const QString &color);
	public slots:
		void Set(const QString &color);
	};

	class ColorEdit: public QLineEdit
	{
		Q_OBJECT
	public:
		ColorEdit(QWidget *parent);
	signals:
		void NeedValidatedStyle(bool valid,QWidget *widget);
		void Valid(bool valid);
	protected slots:
		void Validate(const QString &color);
	};

	class RequiredEdit: public QLineEdit
	{
		Q_OBJECT
	public:
		RequiredEdit(QWidget *parent);
	signals:
		void NeedValidatedStyle(bool valid,QWidget *widget);
		void Valid(bool valid);
	protected slots:
		void Validate(const QString &text);
	};

	class PathEdit: public QLineEdit
	{
		Q_OBJECT
	public:
		PathEdit(const QStringList &acceptableFileExtensions,QWidget *parent,bool allowDirectories=false);
		QStringList acceptableFileExtensions;
		bool allowDirectories;
	signals:
		void NeedValidatedStyle(bool valid,QWidget *widget);
		void Valid(bool valid);
	protected slots:
		void Validate(const QString &path);
	};

	class DirectoryEdit: public QLineEdit
	{
		Q_OBJECT
	public:
		DirectoryEdit(QWidget *parent);
	signals:
		void NeedValidatedStyle(bool valid,QWidget *widget);
		void Valid(bool valid);
	protected slots:
		void Validate(const QString &path);
	};

	class FontEdit: public QLineEdit
	{
		Q_OBJECT
	public:
		FontEdit(QSpinBox *pointSize,QWidget *parent);
	protected:
		QSpinBox *pointSize;
	signals:
		void NeedValidatedStyle(bool valid,QWidget *widget);
		void Valid(bool valid);
	protected slots:
		void Validate(const QString &family);
		void Validate(int pointSize);
		void Validate(const QString &family,int pointSize);
	};

	class WebAddressEdit: public QLineEdit
	{
		Q_OBJECT
	public:
		WebAddressEdit(QWidget *parent);
	signals:
		void NeedValidatedStyle(bool valid,QWidget *widget);
		void Valid(bool valid);
	protected slots:
		void Validate(const QString &address);
	};

	namespace Text
	{
		inline const char *BROWSE="Browse";
		inline const char *CHOOSE="Pick";
		inline const char *PREVIEW="Preview";
		inline const char *DIALOG_TITLE_FILE="Choose File";
		inline const char *DIALOG_TITLE_DIRECTORY="Choose Directory";
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
			};

			class Channel : public Category
			{
				Q_OBJECT
			public:
				Channel(Settings::Channel &settings,Feedback::Error &errorReport);
				void Save() override;
			protected:
				RequiredEdit name;
				QCheckBox protection;
				Settings::Channel &settings;
				Feedback::Error &errorReport;
				bool eventFilter(QObject *object,QEvent *event) override;
			signals:
				void Changed();
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
				Window(Settings settings,Feedback::Error &errorReport);
				void Save() override;
			protected:
				ColorEdit backgroundColor;
				ColorPreview previewBackgroundColor;
				QPushButton selectBackgroundColor;
				QSpinBox width;
				QSpinBox height;
				Settings settings;
				Feedback::Error &errorReport;
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
				Status(Settings settings,Feedback::Error &errorReport);
				void Save() override;
			protected:
				QSpinBox fontSize;
				FontEdit font;
				QPushButton selectFont;
				ColorEdit foregroundColor;
				ColorPreview previewForegroundColor;
				QPushButton selectForegroundColor;
				ColorEdit backgroundColor;
				ColorPreview previewBackgroundColor;
				QPushButton selectBackgroundColor;
				Settings settings;
				Feedback::Error &errorReport;
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
				Chat(Settings settings,Feedback::Error &errorReport);
				void Save() override;
			protected:
				QSpinBox fontSize;
				FontEdit font;
				QPushButton selectFont;
				ColorEdit foregroundColor;
				ColorPreview previewForegroundColor;
				QPushButton selectForegroundColor;
				ColorEdit backgroundColor;
				ColorPreview previewBackgroundColor;
				QPushButton selectBackgroundColor;
				QSpinBox statusInterval;
				Settings settings;
				Feedback::Error &errorReport;
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
				Pane(Settings settings,Feedback::Error &errorReport);
				void Save() override;
			protected:
				QSpinBox fontSize;
				FontEdit font;
				QPushButton selectFont;
				ColorEdit foregroundColor;
				ColorPreview previewForegroundColor;
				QPushButton selectForegroundColor;
				ColorEdit backgroundColor;
				ColorPreview previewBackgroundColor;
				QPushButton selectBackgroundColor;
				ColorEdit accentColor;
				ColorPreview previewAccentColor;
				QPushButton selectAccentColor;
				QSpinBox duration;
				Settings settings;
				Feedback::Error &errorReport;
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
				Bot(Settings::Bot &settings,Feedback::Error &errorReport);
				void Save() override;
			protected:
				Settings::Bot &settings;
				PathEdit arrivalSound;
				QPushButton selectArrivalSound;
				QPushButton previewArrivalSound;
				PathEdit portraitVideo;
				QPushButton selectPortraitVideo;
				QPushButton previewPortraitVideo;
				PathEdit cheerVideo;
				QPushButton selectCheerVideo;
				QPushButton previewCheerVideo;
				PathEdit subscriptionSound;
				QPushButton selectSubscriptionSound;
				QPushButton previewSubscriptionSound;
				PathEdit raidSound;
				QSpinBox postRaidEventDelay;
				QSpinBox postRaidEventDelayThreshold;
				QPushButton selectRaidSound;
				QPushButton previewRaidSound;
				QSpinBox inactivityCooldown;
				QSpinBox helpCooldown;
				PathEdit textWallSound;
				QPushButton selectTextWallSound;
				QPushButton previewTextWallSound;
				QSpinBox textWallThreshold;
				PathEdit adBreakWarningVideo;
				QPushButton selectAdBreakWarningVideo;
				QPushButton previewAdBreakWarningVideo;
				QSpinBox adBreakWarningLeadTime;
				PathEdit adBreakFinishedVideo;
				QPushButton selectAdBreakFinishedVideo;
				QPushButton previewAdBreakFinishedVideo;
				QSpinBox adScheduleRefreshInterval;
				QSpinBox monkeyKeyboardBleepRootFrequency;
				QSpinBox monkeyKeyboardBleepLength;
				QSpinBox monkeyKeyboardBloopRootFrequency;
				QSpinBox monkeyKeyboardBloopLength;
				QButtonGroup monkeyKeyboardWaveTypeGroup;
				QRadioButton monkeyKeyboardWaveTypeSine;
				QRadioButton monkeyKeyboardWaveTypeTriangle;
				QRadioButton monkeyKeyboardWaveTypeSawtooth;
				QRadioButton monkeyKeyboardWaveTypeSquare;
				QPushButton monkeyKeyboardNoteA;
				QPushButton monkeyKeyboardNoteB;
				QPushButton monkeyKeyboardNoteC;
				QPushButton monkeyKeyboardNoteD;
				QPushButton monkeyKeyboardNoteE;
				QPushButton monkeyKeyboardNoteF;
				QPushButton monkeyKeyboardNoteG;
				QSlider monkeyKeyboardVolume;
				QLabel monkeyKeyboardVolumeValue;
				QButtonGroup monkeyKeyboardPreviewTypeGroup;
				QRadioButton monkeyKeyboardPreviewTypeBleep;
				QRadioButton monkeyKeyboardPreviewTypeBloop;
				Feedback::Error &errorReport;
				bool eventFilter(QObject *object,QEvent *event) override;
			signals:
				void PlayArrivalSound(const QString &name,std::shared_ptr<QImage> profileImage,const QString &audioPath);
				void PlayPortraitVideo(const QString &path);
				void PlayCheerVideo(const QString &chatter,const unsigned int count,const QString &message,const QString &path);
				void PlaySubscriptionSound(const QString &chatter,const QString &path);
				void PlayRaidSound(const QString &chatter,const unsigned int raiders,const QString &path);
				void PlayTextWallSound(const QString &message,const QString &path);
				void PlayMonkeyKeyboardNote(std::chrono::microseconds duration,int rootFrequency,const QString &note);
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
				void PlayMonkeyKeyboardNote();
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
				Log(Settings settings,Feedback::Error &errorReport);
				void Save() override;
			protected:
				DirectoryEdit directory;
				QPushButton selectDirectory;
				Settings settings;
				Feedback::Error &errorReport;
				bool eventFilter(QObject *object,QEvent *event) override;
			protected slots:
				void OpenDirectory();
			};

			class Security : public Category
			{
				Q_OBJECT
			public:
				Security(::Security &settings,Feedback::Error &errorReport);
				void Save() override;
			protected:
				RequiredEdit administrator;
				RequiredEdit clientID;
				RequiredEdit token;
				WebAddressEdit callbackURL;
				RequiredEdit permissions;
				QPushButton selectPermissions;
				::Security &settings;
				Feedback::Error &errorReport;
				bool eventFilter(QObject *object,QEvent *event) override;
			protected slots:
				void SelectPermissions();
			};
		}

		class Dialog : public QDialog
		{
			Q_OBJECT
		public:
			Dialog(std::vector<Categories::Category*> categories,std::unique_ptr<Feedback::Error> errorReport,QWidget *parent);
		protected:
			QWidget entriesFrame;
			Feedback::Help help;
			QDialogButtonBox buttons;
			QPushButton discard;
			QPushButton save;
			QPushButton apply;
			QVBoxLayout *scrollLayout;
			QGroupBox errorBox;
			QLabel errorMessages;
			std::unique_ptr<Feedback::Error> errorReport;
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
		template <Concept::Widget TWidget>
		class Window
		{
		public:
			Window(QWidget *parent) : dialog(new QDialog(parent))
			{

				pane=new TWidget(&dialog);
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

			TWidget& Pane()
			{
				return *pane;
			}

			void Open()
			{
				dialog.open();
			}
		protected:
			QDialog dialog;
			TWidget *pane;
		};
	}
}
