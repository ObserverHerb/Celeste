#pragma once

#include <QTextEdit>
#include <QTimer>
#include <QPropertyAnimation>
#include <QLineEdit>
#include <QListWidget>
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
#include <QSizeGrip>
#include <QDialog>
#include "entities.h"

namespace StyleSheet
{
	const QString Colors(const QColor &foreground,const QColor &background);
}

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
		inline const char *DIRECTORY_HOME="/home";
		inline const char *BUTTON_SAVE="&Save";
		inline const char *BUTTON_DISCARD="&Discard";
		inline const char *BUTTON_APPLY="&Apply";
	}

	namespace Commands
	{
		enum class Type
		{
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

		class Aliases : public QDialog
		{
			Q_OBJECT
		public:
			Aliases(QWidget *parent);
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
			QString Path() const;
			enum Type Type() const;
			bool Random() const;
			QString Message() const;
			bool Protected() const;
		protected:
			QGridLayout layout;
			QFrame details;
			QPushButton header;
			QLineEdit name;
			QLineEdit description;
			QPushButton openAliases;
			QLineEdit path;
			QPushButton browse;
			QComboBox type;
			QCheckBox random;
			QCheckBox protect;
			QTextEdit message;
			UI::Commands::Aliases aliases;
			void TypeChanged(int index);
			void Native();
			void Pulsar();
			void Browse();
			void UpdateHeader();
			void ToggleFold();
			void ValidatePath(const QString &text,bool random,const enum Type type);
			bool eventFilter(QObject *object,QEvent *event) override;
		signals:
			void Help(const QString &text);
		protected slots:
			void UpdateHeader(const QString &commandName);
			void ValidateName(const QString &text);
			void ValidateDescription(const QString &text);
			void ValidatePath(const QString &text);
			void ValidatePath(const int state);
			void ValidateMessage();
		};

		class Dialog : public QDialog		
		{
			Q_OBJECT
		public:
			Dialog(const Command::Lookup &commands,QWidget *parent);
		protected:
			QWidget entriesFrame;
			Help help;
			QLabel labelFilter;
			QComboBox filter;
			QDialogButtonBox buttons;
			QPushButton discard;
			QPushButton save;
			QPushButton newEntry;
			std::unordered_map<QString,Entry*> entries;
			void PopulateEntries(const Command::Lookup &commands,QLayout *layout);
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
				QPushButton header;
				QFrame details;
				QGridLayout detailsLayout;
				QLabel* Label(const QString &text);
				void Rows(std::vector<std::vector<QWidget*>> widgets);
				virtual bool eventFilter(QObject *object,QEvent *event) override=0;
			private:
				QVBoxLayout layout;
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
				void Save();
			protected:
				QLineEdit name;
				QCheckBox protection;
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

			class Bot : public Category
			{
				Q_OBJECT
			public:
				struct Settings
				{
					//ApplicationSetting &vibePlaylist; // TODO: implement this when the decision as to how I will be changing playlists for Qt 6 is made
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
				bool eventFilter(QObject *object,QEvent *event) override;
			signals:
				void PlayArrivalSound(const QString &name,QImage profileImage,const QString &audioPath);
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
			QListWidget rawUsers;
			QListWidget validUsers;
			QTimer acknowledgeDelay;
			static const QString TITLE;
			void UpdateTitle();
		signals:
			void Acknowledged();
		public slots:
			void Joined(const QString &user);
			void Acknowledged(const QStringList &names);
			void Parted(const QString &user);
		};
	}
}