#include <QMessageBox>
#include <QFontDialog>
#include <QFileDialog>
#include "widgets/widgets.h"

namespace UI
{
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

			QLabel* Category::Subheading(const QString &text)
			{
				QLabel *label=new QLabel(text,this);
				label->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
				label->setStyleSheet("font-size: 150%; font-weight: bold; border-bottom: 1px solid palette(text); margin-top: 0.5em;");
				return label;
			}

			void Category::Rows(std::vector<std::vector<QWidget*>> widgets)
			{
				// find longest row (searching through vector of rows of widgets)
				int maxColumns=2;
				for (const std::vector<QWidget*> &row : widgets)
				{
					maxColumns=std::max(static_cast<int>(std::ssize(row)),maxColumns);
				}

				for (int rowIndex=0; rowIndex < std::ssize(widgets); rowIndex++)
				{
					int columns=widgets[rowIndex].size();
					int columnIndexOffset=0;
					for (int columnIndex=0; columnIndex < columns; columnIndex++)
					{
						int columnSpan=1;
						if (columnIndex < 2 && columns < maxColumns)
						{
							if (columnIndex < 1)
							{
								if (columns < 2)
								{
									columnSpan=maxColumns; // only widget, so span all columns
								}
							}
							else
							{
								columnSpan=maxColumns-columns+1; // second widget, so span difference between this row's length and the longest row's length
							}
						}
						QWidget *widget=widgets[rowIndex][columnIndex];
						detailsLayout.addWidget(widget,rowIndex,columnIndex+columnIndexOffset,1,columnSpan);
						if (columnSpan > 1) columnIndexOffset=columnSpan-1;
						widget->installEventFilter(this); // NOTE: this will not fire for labels because they do not fire an enterEvent for mouse hovers
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

			Channel::Channel(Settings::Channel &settings,std::shared_ptr<Feedback::Error> errorReport,QWidget *parent) : Category(parent,QStringLiteral("Channel")),
				name(this),
				protection(this),
				settings(settings),
				errorReport(errorReport)
			{
				connect(&name,&QLineEdit::textChanged,this,&Channel::ValidateName);

				name.setText(settings.name);
				protection.setChecked(settings.protect);

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
				bool changed=settings.name != name.text();

				settings.name.Set(name.text());
				settings.protect.Set(protection.isChecked());

				// only need to do this on one of the settings for all of the categories, because it
				// is all the same QSettings object under the hood, so it's saving all of the settings
				settings.name.Save();

				if (changed) emit Changed();
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
				statusInterval.setRange(TimeConvert::ONE_SECOND_IN_MILLISECONDS.count(),std::numeric_limits<int>::max());

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
				duration.setRange(TimeConvert::ONE_SECOND_IN_MILLISECONDS.count(),std::numeric_limits<int>::max());
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

			Bot::Bot(Settings::Bot &settings,std::shared_ptr<Feedback::Error> errorReport,QWidget *parent) : Category(parent,QStringLiteral("Bot Core")),
				settings(settings),
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
				adBreakWarningVideo(this),
				selectAdBreakWarningVideo(Text::BROWSE,this),
				previewAdBreakWarningVideo(Text::PREVIEW,this),
				adBreakWarningLeadTime(this),
				adBreakFinishedVideo(this),
				selectAdBreakFinishedVideo(Text::BROWSE,this),
				previewAdBreakFinishedVideo(Text::PREVIEW,this),
				adScheduleRefreshInterval(this),
				monkeyKeyboardBleepRootFrequency(this),
				monkeyKeyboardBleepLength(this),
				monkeyKeyboardBloopRootFrequency(this),
				monkeyKeyboardBloopLength(this),
				monkeyKeyboardVolume(Qt::Horizontal,this),
				monkeyKeyboardVolumeValue(this),
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
				connect(&adBreakWarningVideo,&QLineEdit::textChanged,this,&Bot::ValidateAdBreakWarningVideo);
				connect(&selectAdBreakWarningVideo,&QPushButton::clicked,this,&Bot::OpenAdBreakWarningVideo);
				connect(&previewAdBreakWarningVideo,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayAdBreakWarningVideo));
				connect(&adBreakFinishedVideo,&QLineEdit::textChanged,this,&Bot::ValidateAdBreakFinishedVideo);
				connect(&selectAdBreakFinishedVideo,&QPushButton::clicked,this,&Bot::OpenAdBreakFinishedVideo);
				connect(&previewAdBreakFinishedVideo,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayAdBreakFinishedVideo));
				connect(&monkeyKeyboardVolume,&QSlider::valueChanged,this,&Bot::MonkeyKeyboardVolumeChanged);

				arrivalSound.setText(settings.arrivalSound);
				portraitVideo.setText(settings.portraitVideo);
				cheerVideo.setText(settings.cheerVideo);
				subscriptionSound.setText(settings.subscriptionSound);
				raidSound.setText(settings.raidSound);
				postRaidEventDelay.setRange(1,std::numeric_limits<int>::max());
				postRaidEventDelay.setValue(settings.raidInterruptDuration);
				postRaidEventDelayThreshold.setRange(0,std::numeric_limits<int>::max());
				postRaidEventDelayThreshold.setValue(settings.raidInterruptDelayThreshold);
				inactivityCooldown.setRange(TimeConvert::ONE_SECOND_IN_MILLISECONDS.count(),std::numeric_limits<int>::max());
				inactivityCooldown.setValue(settings.inactivityCooldown);
				helpCooldown.setRange(TimeConvert::ONE_SECOND_IN_MILLISECONDS.count(),std::numeric_limits<int>::max());
				helpCooldown.setValue(settings.helpCooldown);
				textWallThreshold.setRange(1,std::numeric_limits<int>::max());
				textWallThreshold.setValue(settings.textWallThreshold);
				textWallSound.setText(settings.textWallSound);
				adBreakWarningVideo.setText(settings.adWarningVideo);
				adBreakWarningLeadTime.setRange(1,std::numeric_limits<int>::max());
				adBreakWarningLeadTime.setValue(settings.adWarningLeadTime);
				adBreakFinishedVideo.setText(settings.adFinishedVideo);
				adScheduleRefreshInterval.setRange(30,std::numeric_limits<int>::max());
				adScheduleRefreshInterval.setValue(settings.adScheduleRefreshInterval);
				monkeyKeyboardBleepRootFrequency.setRange(Natural::MINIMUM_AUDIBLE_FREQUENCY,Natural::MAXIMUM_AUDIBLE_FREQUENCY);
				monkeyKeyboardBleepRootFrequency.setValue(settings.monkeyKeyboardBleepRootFrequency);
				monkeyKeyboardBleepLength.setRange(1,std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(1)).count());
				monkeyKeyboardBleepLength.setValue(settings.monkeyKeyboardBleepLength);
				monkeyKeyboardBloopRootFrequency.setRange(Natural::MINIMUM_AUDIBLE_FREQUENCY,Natural::MAXIMUM_AUDIBLE_FREQUENCY);
				monkeyKeyboardBloopRootFrequency.setValue(settings.monkeyKeyboardBloopRootFrequency);
				monkeyKeyboardBloopLength.setRange(1,std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(1)).count());
				monkeyKeyboardBloopLength.setValue(settings.monkeyKeyboardBloopLength);
				monkeyKeyboardVolume.setRange(0,100);
				monkeyKeyboardVolume.setValue(settings.monkeyKeyboardVolume);
				monkeyKeyboardVolumeValue.setText(QString::number(monkeyKeyboardVolume.value())+"%");

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
					{Subheading(u"Ads"_s)},
					{Label(u"Ad Break Warning Video"_s),&adBreakWarningVideo,&selectAdBreakWarningVideo,&previewAdBreakWarningVideo,Label(u"Lead Time"_s),&adBreakWarningLeadTime},
					{Label(u"Ad Break Finished Video"_s),&adBreakFinishedVideo,&selectAdBreakFinishedVideo,&previewAdBreakFinishedVideo,Label(u"Refresh Interval"_s),&adScheduleRefreshInterval},
					{Subheading(u"Monkey Keyboard"_s)},
					{Label(u"Bleep Root Frequency"_s),&monkeyKeyboardBleepRootFrequency,Label(u"Bleep Length"_s),&monkeyKeyboardBleepLength,Label(u"Bloop Root Frequency"_s),&monkeyKeyboardBloopRootFrequency,Label(u"Bloopp Length"_s),&monkeyKeyboardBloopLength},
					{Label(u"Volume"_s),&monkeyKeyboardVolume,&monkeyKeyboardVolumeValue}
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
					if (object == &adBreakWarningVideo || object == &selectAdBreakWarningVideo || object == &previewAdBreakWarningVideo) emit Help(u"A video (mp4) that plays when an ad break is about to begin"_s);
					if (object == &adBreakWarningLeadTime) emit Help(u"How many seconds to warn in advance that an ad break is about to begin"_s);
					if (object == &adBreakFinishedVideo || object == &selectAdBreakFinishedVideo || object == &previewAdBreakFinishedVideo) emit Help(u"A video (mp4) that plays when an break has finished"_s);
					if (object == &adScheduleRefreshInterval) emit Help(u"How often (in seconds) to ask Twitch for the ad manager schedule"_s);
					if (object == &monkeyKeyboardBleepRootFrequency) emit Help(u"This will be the frequency that will be treated as A. The notes B through G will be calculated based off of this one. It is common convention for A4 to be 440Hz. Halve this to drop an octave (A3 = 220Hz), double it to raise an octave (A5 = 880Hz). While this is the international standard, you are not required to follow it. You can set this to any value between 20Hz and 20kHz. Also note that bleep is intended to be the higher note, but this is also not a requirement."_s);
					if (object == &monkeyKeyboardBleepLength) emit Help(u"The length, in milliseconds, of the note. Bleep is intended to be the short note, but this is not required or enforced. Set it to any length you like between 1 millisecond and 1 full second."_s);
					if (object == &monkeyKeyboardBloopRootFrequency) emit Help(u"This will be the frequency that will be treated as A. The notes B through G will be calculated based off of this one. It is common convention for A4 to be 440Hz. Halve this to drop an octave (A3 = 220Hz), double it to raise an octave (A5 = 880Hz). While this is the international standard, you are not required to follow it. You can set this to any value between 20Hz and 20kHz. Also note that bloop is intended to be the lower note, but this is also not a requirement."_s);
					if (object == &monkeyKeyboardBloopLength) emit Help(u"The length, in milliseconds, of the note. Bloop is intended to be the long note, but this is not required or enforced. Set it to any length you like between 1 millisecond and 1 full second."_s);
					if (object == &monkeyKeyboardVolume) emit Help(u"How loud a monkey keyboard note is."_s);
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
				static const QString OPERATION="Audio Preview Failed";

				try
				{
					const QString filename=arrivalSound.text();
					emit PlayArrivalSound(qApp->applicationName(),std::make_shared<QImage>(Resources::CELESTE),QFileInfo(filename).isDir() ? File::List(filename).Random() : filename);
				}

				catch (const std::out_of_range &exception)
				{
					QMessageBox{QMessageBox::Warning,OPERATION,u"Memory error: "_s+exception.what(),QMessageBox::Ok}.exec();
				}

				catch (const std::exception &exception)
				{
					QMessageBox{QMessageBox::Warning,OPERATION,u"Unknown error"_s,QMessageBox::Ok}.exec();
				}
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

			void Bot::OpenAdBreakWarningVideo()
			{
				QString candidate=OpenVideo(this,adBreakWarningVideo.text());
				if (!candidate.isEmpty()) adBreakWarningVideo.setText(candidate);
			}

			void Bot::PlayAdBreakWarningVideo()
			{
				emit PlayAdBreakWarningVideo(adBreakWarningVideo.text());
			}

			void Bot::OpenAdBreakFinishedVideo()
			{
				QString candidate=OpenVideo(this,adBreakFinishedVideo.text());
				if (!candidate.isEmpty()) adBreakFinishedVideo.setText(candidate);
			}

			void Bot::PlayAdBreakFinishedVideo()
			{
				emit PlayAdBreakFinishedVideo(adBreakFinishedVideo.text());
			}

			void Bot::MonkeyKeyboardVolumeChanged(int value)
			{
				monkeyKeyboardVolumeValue.setText(QString::number(value)+"%");
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

			void Bot::ValidateAdBreakWarningVideo(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() && candidate.suffix() == Text::FILE_TYPE_VIDEO;
				if (valid)
					errorReport->Valid(&adBreakWarningVideo);
				else
					errorReport->Invalid(&adBreakWarningVideo);
				previewAdBreakWarningVideo.setEnabled(valid);
			}

			void Bot::ValidateAdBreakFinishedVideo(const QString &path)
			{
				QFileInfo candidate(path);
				bool valid=candidate.exists() && candidate.suffix() == Text::FILE_TYPE_VIDEO;
				if (valid)
					errorReport->Valid(&adBreakFinishedVideo);
				else
					errorReport->Invalid(&adBreakFinishedVideo);
				previewAdBreakFinishedVideo.setEnabled(valid);
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
				if (QString text=adBreakWarningVideo.text(); !text.isEmpty()) settings.adWarningVideo.Set(text);
				settings.adWarningLeadTime.Set(adBreakWarningLeadTime.value());
				if (QString text=adBreakFinishedVideo.text(); !text.isEmpty()) settings.adFinishedVideo.Set(text);
				settings.adScheduleRefreshInterval.Set(adScheduleRefreshInterval.value());
				settings.monkeyKeyboardBleepRootFrequency.Set(monkeyKeyboardBleepRootFrequency.value());
				settings.monkeyKeyboardBleepLength.Set(monkeyKeyboardBleepLength.value());
				settings.monkeyKeyboardBloopRootFrequency.Set(monkeyKeyboardBloopRootFrequency.value());
				settings.monkeyKeyboardBloopLength.Set(monkeyKeyboardBloopLength.value());
				settings.monkeyKeyboardVolume.Set(monkeyKeyboardVolume.value());
			}

			Pulsar::Pulsar(Settings::Pulsar &settings,QWidget *parent) : Category(parent,u"Pulsar"_s),
				settings(settings),
				subsystemEnabled(this),
				reconnectDelay(this)
			{
				subsystemEnabled.setChecked(settings.enabled);
				reconnectDelay.setRange(1,std::numeric_limits<int>::max());
				reconnectDelay.setValue(settings.reconnectDelay);

				Rows({
					{Label(u"Use Pulsar"_s),&subsystemEnabled},
					{Label(u"Reconnect Delay"_s),&reconnectDelay}
				});
			}

			bool Pulsar::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					if (object == &subsystemEnabled) emit Help(u"Check this to enable the Pulsar plugin which allows the bot to communicate with OBS Studio. This will only work if the Pulsar plugin was installed when the bot was installed."_s);
					if (object == &reconnectDelay) emit Help(u"How long (in milliseconds) to wait until attempting to reconnect to the OBS plugin after a failed attempt"_s);
				}

				if (event->type() == QEvent::HoverLeave) emit Help(u""_s);
				return false;
			}

			void Pulsar::Save()
			{
				settings.enabled.Set(subsystemEnabled.isChecked());
				settings.reconnectDelay.Set(reconnectDelay.value());
			}

			Log::Log(Settings settings,std::shared_ptr<Feedback::Error> errorReport,QWidget *parent) : Category(parent,u"Logging"_s),
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
			connect(category,&Categories::Category::Help,&help,&UI::Feedback::Help::Message);
			categories.push_back(category);
		}

		void Dialog::Save()
		{
			for (Categories::Category *category : categories) category->Save();
			emit Refresh();
		}
	}
}
