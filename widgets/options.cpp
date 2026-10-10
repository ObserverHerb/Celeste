#include <QMessageBox>
#include <QFontDialog>
#include <QFileDialog>
#include "widgets/widgets.h"

const char *STYLE_NO_FILL="No Fill";

namespace UI
{
	namespace Options
	{
		namespace Categories
		{
			Category::Category(const QString &name,Feedback::Error &errorReport) : QFrame(nullptr),
				verticalLayout(this),
				header(this),
				details(nullptr),
				detailsLayout(nullptr),
				errorReport(errorReport)
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
						QWidget *widget=widgets[rowIndex][columnIndex];
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
								if (!widget->property(STYLE_NO_FILL).toBool()) columnSpan=maxColumns-columns+1; // second widget, so span difference between this row's length and the longest row's length
							}
						}
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

			bool Category::eventFilter(QObject *object,QEvent *event)
			{
				if (event->type() == QEvent::HoverEnter)
				{
					auto element=helpTexts.find(object);
					if (element != helpTexts.end()) emit Help(*element->second);
					return false;
				}

				if (event->type() == QEvent::HoverLeave) emit Help("");
				return false;
			}

			Channel::Channel(Settings::Channel &settings,Feedback::Error &errorReport) : Category(u"Channel"_s,errorReport),
				name(this),
				protection(this),
				settings(settings)
			{
				helpTexts={
					{&name,std::make_shared<QString>(u"Name of the channel Celeste will join on launch"_s)},
					{&protection,std::make_shared<QString>(u"When the bot is closed, enable protections such as turning on emote-only chat? This is intended to prevent situations such as offline hate raids."_s)}
				};

				connect(&name,&RequiredEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);

				name.setObjectName(u"Channel name"_s);
				name.setText(settings.name);
				protection.setChecked(settings.protect);

				Rows({
					{Label(u"Name"_s),&name},
					{Label(u"Protection"_s),&protection}
				});
			}

			void Channel::Save()
			{
				bool changed=settings.name != name.text();

				settings.name.Set(name.text());
				settings.protect.Set(protection.isChecked());

				// only need to do this on one of the settings for all of the categories, because it
				// is all the same QSettings object under the hood, so it's saving all of the settings
				settings.name.Save();

				if (changed) emit Changed(); // need to reconnect to Twitch if this setting gets changed
			}

			Window::Window(Settings settings,Feedback::Error &errorReport): Category(u"Main Window"_s,errorReport),
				backgroundColor(this),
				previewBackgroundColor(this,settings.backgroundColor),
				selectBackgroundColor(Text::CHOOSE,this),
				width(this),
				height(this),
				settings(settings)
			{
				auto backgroundColorHelpText=std::make_shared<QString>(u"This is the background color of the main window. Note that this is <em>not</em> the background color of individual panes (such as the chat pane)."_s);
				helpTexts={
					{&backgroundColor,backgroundColorHelpText},{&selectBackgroundColor,backgroundColorHelpText},
					{&width,std::make_shared<QString>(u"The width (in pixels) of the application window's contents (the part seen by OBS)"_s)},
					{&height,std::make_shared<QString>(u"The height (in pixels) of the application window's contents (the part seen by OBS)"_s)}
				};

				connect(&backgroundColor,&ColorEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&backgroundColor,&ColorEdit::textChanged,&previewBackgroundColor,&ColorPreview::Set);
				connect(&selectBackgroundColor,&QPushButton::clicked,&backgroundColor,&ColorEdit::PickColor);

				backgroundColor.setObjectName(u"Window background color");
				backgroundColor.setText(settings.backgroundColor);
				QRect desktop=QGuiApplication::primaryScreen()->availableVirtualGeometry();
				width.setRange(1,desktop.width());
				width.setValue(static_cast<QSize>(settings.dimensions).width());
				height.setRange(1,desktop.height());
				height.setValue(static_cast<QSize>(settings.dimensions).height());

				Rows({
					{Label(u"Background Color"_s),&backgroundColor,&previewBackgroundColor,&selectBackgroundColor},
					{Label(u"Width"_s),&width},
					{Label(u"Height"_s),&height}
				});
			}

			void Window::Save()
			{
				settings.backgroundColor.Set(backgroundColor.text());
				settings.dimensions.Set(QSize{width.value(),height.value()});
			}

			Status::Status(Settings settings,Feedback::Error &errorReport) : Category(u"Status"_s,errorReport),
				fontSize(this),
				font(&fontSize,this),
				selectFont(Text::CHOOSE,this),
				foregroundColor(this),
				previewForegroundColor(this,settings.foregroundColor),
				selectForegroundColor(Text::CHOOSE,this),
				backgroundColor(this),
				previewBackgroundColor(this,settings.backgroundColor),
				selectBackgroundColor(Text::CHOOSE,this),
				settings(settings)
			{
				auto fontHelpText=std::make_shared<QString>(u"The font that will be used in the initialization screen when the bot is first launched and connecting to Twitch"_s);
				auto foregroundColorHelpText=std::make_shared<QString>(u"The color of text in the initialization screen that is shown when the bot is first launched and connecting to Twitch"_s);
				auto backgroundColorHelpText=std::make_shared<QString>(u"The color of the background in the initialization screen that is shown when the bot is first launched and connecting to Twitch"_s);
				helpTexts={
					{&font,fontHelpText},{&fontSize,fontHelpText},{&selectFont,fontHelpText},
					{&foregroundColor,foregroundColorHelpText},{&selectForegroundColor,foregroundColorHelpText},
					{&backgroundColor,backgroundColorHelpText},{&selectBackgroundColor,backgroundColorHelpText}
				};

				connect(&font,&FontEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectFont,&QPushButton::clicked,&font,&FontEdit::PickFont);
				connect(&foregroundColor,&ColorEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&foregroundColor,&ColorEdit::textChanged,&previewForegroundColor,&ColorPreview::Set);
				connect(&selectForegroundColor,&QPushButton::clicked,&foregroundColor,&ColorEdit::PickColor);
				connect(&backgroundColor,&ColorEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&backgroundColor,&ColorEdit::textChanged,&previewBackgroundColor,&ColorPreview::Set);
				connect(&selectBackgroundColor,&QPushButton::clicked,&backgroundColor,&ColorEdit::PickColor);

				font.setObjectName(u"Status pane font"_s);
				font.setText(settings.font);
				fontSize.setRange(1,std::numeric_limits<short>::max());
				fontSize.setValue(settings.fontSize);
				foregroundColor.setObjectName(u"Status pane foreground color"_s);
				foregroundColor.setText(settings.foregroundColor);
				backgroundColor.setObjectName(u"Status pane background color"_s);
				backgroundColor.setText(settings.backgroundColor);

				Rows({
					{Label(u"Font"_s),&font,Label(u"Size"_s),&fontSize,&selectFont},
					{Label(u"Text Color"_s),&foregroundColor,&previewForegroundColor,&selectForegroundColor},
					{Label(u"Background Color"_s),&backgroundColor,&previewBackgroundColor,&selectBackgroundColor},
				});
			}

			void Status::Save()
			{
				settings.font.Set(font.text());
				settings.fontSize.Set(fontSize.value());
				settings.foregroundColor.Set(foregroundColor.text());
				settings.backgroundColor.Set(backgroundColor.text());
			}

			Chat::Chat(Settings settings,Feedback::Error &errorReport) : Category(u"Chat"_s,errorReport),
				fontSize(this),
				font(&fontSize,this),
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
				auto fontHelpText=std::make_shared<QString>(u"The font that will be used to display chat messages"_s);
				auto foregroundColorHelpText=std::make_shared<QString>(u"The color of chat message text"_s);
				auto backgroundColorHelpText=std::make_shared<QString>(u"The color of the background behind chat messages"_s);
				helpTexts={
					{&font,fontHelpText},{&fontSize,fontHelpText},{&selectFont,fontHelpText},
					{&foregroundColor,foregroundColorHelpText},{&selectForegroundColor,foregroundColorHelpText},
					{&backgroundColor,backgroundColorHelpText},{&selectBackgroundColor,backgroundColorHelpText},
					{&statusInterval,std::make_shared<QString>(u"How long (in milliseconds) updates and error messages should display at the bottom of the chat pane"_s)}
				};

				connect(&font,&FontEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectFont,&QPushButton::clicked,&font,&FontEdit::PickFont);
				connect(&foregroundColor,&ColorEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&foregroundColor,&ColorEdit::textChanged,&previewForegroundColor,&ColorPreview::Set);
				connect(&selectForegroundColor,&QPushButton::clicked,&foregroundColor,&ColorEdit::PickColor);
				connect(&backgroundColor,&ColorEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&backgroundColor,&ColorEdit::textChanged,&previewBackgroundColor,&ColorPreview::Set);
				connect(&selectBackgroundColor,&QPushButton::clicked,&backgroundColor,&ColorEdit::PickColor);

				font.setObjectName(u"Chat pane font"_s);
				font.setText(settings.font);
				fontSize.setRange(1,std::numeric_limits<short>::max());
				fontSize.setValue(settings.fontSize);
				foregroundColor.setObjectName(u"Chat pane foreground color"_s);
				foregroundColor.setText(settings.foregroundColor);
				backgroundColor.setObjectName(u"Chat pane background color"_s);
				backgroundColor.setText(settings.backgroundColor);

				statusInterval.setRange(TimeConvert::ONE_SECOND_IN_MILLISECONDS.count(),std::numeric_limits<int>::max());

				Rows({
					{Label(u"Font"_s),&font,Label(u"Size"_s),&fontSize,&selectFont},
					{Label(u"Text Color"_s),&foregroundColor,&previewForegroundColor,&selectForegroundColor},
					{Label(u"Background Color"_s),&backgroundColor,&previewBackgroundColor,&selectBackgroundColor},
					{Label(u"Status Duration"_s),&statusInterval}
				});
			}

			void Chat::Save()
			{
				settings.font.Set(font.text());
				settings.fontSize.Set(fontSize.value());
				settings.foregroundColor.Set(foregroundColor.text());
				settings.backgroundColor.Set(backgroundColor.text());
				settings.statusInterval.Set(statusInterval.value());
			}

			Pane::Pane(Settings settings,Feedback::Error &errorReport) : Category(u"Panes"_s,errorReport),
				fontSize(this),
				font(&fontSize,this),
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
				auto fontHelpText=std::make_shared<QString>(u"The font that will be used in event panes (such as raid and subscription announcements)"_s);
				auto foregroundColorHelpText=std::make_shared<QString>(u"The color of text in event panes (such as raid and subscription announcements)"_s);
				auto backgroundColorHelpText=std::make_shared<QString>(u"The color of the background in event panes (such as raid and subscription announcements)"_s);
				auto accentColorHelpText=std::make_shared<QString>(u"The color of text effects, such as drop shadows"_s);
				helpTexts={
					{&font,fontHelpText},{&fontSize,fontHelpText},{&selectFont,fontHelpText},
					{&foregroundColor,foregroundColorHelpText},{&selectForegroundColor,foregroundColorHelpText},
					{&backgroundColor,backgroundColorHelpText},{&selectBackgroundColor,backgroundColorHelpText},
					{&accentColor,accentColorHelpText},{&selectAccentColor,accentColorHelpText},
					{&duration,std::make_shared<QString>(u"The amount of time (in milliseconds) that an announcement will display. This only affects announcements that don't have an associated audio or video file, otherwise the duration will be the duration of the associated audio or video."_s)}
				};

				connect(&font,&FontEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectFont,&QPushButton::clicked,&font,&FontEdit::PickFont);
				connect(&foregroundColor,&ColorEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&foregroundColor,&ColorEdit::textChanged,&previewForegroundColor,&ColorPreview::Set);
				connect(&selectForegroundColor,&QPushButton::clicked,&foregroundColor,&ColorEdit::PickColor);
				connect(&backgroundColor,&ColorEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&backgroundColor,&ColorEdit::textChanged,&previewBackgroundColor,&ColorPreview::Set);
				connect(&selectBackgroundColor,&QPushButton::clicked,&backgroundColor,&ColorEdit::PickColor);
				connect(&accentColor,&ColorEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&accentColor,&ColorEdit::textChanged,&previewAccentColor,&ColorPreview::Set);
				connect(&selectAccentColor,&QPushButton::clicked,&accentColor,&ColorEdit::PickColor);

				font.setObjectName(u"Announcements font"_s);
				font.setText(settings.font);
				fontSize.setRange(1,std::numeric_limits<short>::max());
				fontSize.setValue(settings.fontSize);
				foregroundColor.setObjectName(u"Announcements text color"_s);
				foregroundColor.setText(settings.foregroundColor);
				backgroundColor.setObjectName(u"Announcements background color"_s);
				backgroundColor.setText(settings.backgroundColor);
				accentColor.setObjectName(u"Announcements accent color"_s);
				accentColor.setText(settings.accentColor);
				duration.setRange(TimeConvert::ONE_SECOND_IN_MILLISECONDS.count(),std::numeric_limits<int>::max());
				duration.setValue(settings.duration);

				Rows({
					{Label(u"Font"_s),&font,Label(u"Size"_s),&fontSize,&selectFont},
					{Label(u"Text Color"_s),&foregroundColor,&previewForegroundColor,&selectForegroundColor},
					{Label(u"Background Color"_s),&backgroundColor,&previewBackgroundColor,&selectBackgroundColor},
					{Label(u"Accent Color"_s),&accentColor,&previewAccentColor,&selectAccentColor},
					{Label(u"Duration"_s),&duration}
				});
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

			Music::Music(Settings settings,Feedback::Error &errorReport) : Category(u"Music"_s,errorReport),
				suppressedVolume(this),
				settings(settings)
			{
				helpTexts={{&suppressedVolume,std::make_shared<QString>(u"The volume the music should duck to when another pane is playing audio."_s)}};

				suppressedVolume.setRange(0,100);
				suppressedVolume.setSuffix("%");
				suppressedVolume.setValue(settings.suppressedVolume);

				Rows({
					{Label(u"Suppressed Volume"_s),&suppressedVolume}
				});
			}

			void Music::Save()
			{
				settings.suppressedVolume.Set(suppressedVolume.value());
			}

			Bot::Bot(Settings::Bot &settings,Feedback::Error &errorReport) : Category(u"Bot Core"_s,errorReport),
				settings(settings),
				arrivalSound({Text::FILE_TYPE_AUDIO},this,true),
				selectArrivalSound(Text::BROWSE,this),
				previewArrivalSound(Text::PREVIEW,this),
				portraitVideo({Text::FILE_TYPE_VIDEO},this),
				selectPortraitVideo(Text::BROWSE,this),
				previewPortraitVideo(Text::PREVIEW,this),
				cheerVideo({Text::FILE_TYPE_VIDEO},this,true),
				selectCheerVideo(Text::BROWSE,this),
				previewCheerVideo(Text::PREVIEW,this),
				subscriptionSound({Text::FILE_TYPE_AUDIO},this),
				selectSubscriptionSound(Text::BROWSE,this),
				previewSubscriptionSound(Text::PREVIEW,this),
				raidSound({Text::FILE_TYPE_AUDIO},this),
				postRaidEventDelay(this),
				postRaidEventDelayThreshold(this),
				selectRaidSound(Text::BROWSE,this),
				previewRaidSound(Text::PREVIEW,this),
				inactivityCooldown(this),
				helpCooldown(this),
				textWallSound({Text::FILE_TYPE_AUDIO},this),
				selectTextWallSound(Text::BROWSE,this),
				previewTextWallSound(Text::PREVIEW,this),
				textWallThreshold(this),
				adBreakWarningVideo({Text::FILE_TYPE_VIDEO},this),
				selectAdBreakWarningVideo(Text::BROWSE,this),
				previewAdBreakWarningVideo(Text::PREVIEW,this),
				adBreakWarningLeadTime(this),
				adBreakFinishedVideo({Text::FILE_TYPE_VIDEO},this),
				selectAdBreakFinishedVideo(Text::BROWSE,this),
				previewAdBreakFinishedVideo(Text::PREVIEW,this),
				adScheduleRefreshInterval(this),
				monkeyKeyboardBleepRootFrequency(this),
				monkeyKeyboardBleepLength(this),
				monkeyKeyboardBloopRootFrequency(this),
				monkeyKeyboardBloopLength(this),
				monkeyKeyboardWaveTypeGroup(this),
				monkeyKeyboardWaveTypeSine("Sine",this),
				monkeyKeyboardWaveTypeTriangle("Triangle",this),
				monkeyKeyboardWaveTypeSawtooth("Sawtooth",this),
				monkeyKeyboardWaveTypeSquare("Square",this),
				monkeyKeyboardNoteA("A",this),
				monkeyKeyboardNoteB("B",this),
				monkeyKeyboardNoteC("C",this),
				monkeyKeyboardNoteD("D",this),
				monkeyKeyboardNoteE("E",this),
				monkeyKeyboardNoteF("F",this),
				monkeyKeyboardNoteG("G",this),
				monkeyKeyboardVolume(Qt::Horizontal,this),
				monkeyKeyboardVolumeValue(this),
				monkeyKeyboardPreviewTypeGroup(this),
				monkeyKeyboardPreviewTypeBleep("Bleep",this),
				monkeyKeyboardPreviewTypeBloop("Bloop",this)
			{
				auto arrivalSoundHelpText=std::make_shared<QString>(u"This is the sound that plays each time someone s in chat for the first time. This can be a single audio file (mp3), or a folder of audio files. If it's a folder, a random audio file will be chosen from that folder each time."_s);
				auto cheerVideoHelpText=std::make_shared<QString>(u"A video (mp4) that plays when a chatter cheers bits."_s);
				auto portraitVideoHelpText=std::make_shared<QString>(u"Every so often, Twitch will send a request to the bot asking if it's still connected (ping). This is a video that can play each time that happens."_s);
				auto subscriptionSoundHelpText=std::make_shared<QString>(u"This is the sound that plays when a chatter subscribes to the channel."_s);
				auto raidSoundHelpText=std::make_shared<QString>(u"This is the sound that plays when another streamer raids the channel."_s);
				auto textWallSoundHelpText=std::make_shared<QString>(u"This is the sound that plays when a user spams chat with a super long message. The threshold is the number of characters the message needs to be to trigger the sound."_s);
				auto adBreakWarningVideoHelpText=std::make_shared<QString>(u"A video (mp4) that plays when an ad break is about to begin"_s);
				auto adBreakFinishedVideoHelpText=std::make_shared<QString>(u"A video (mp4) that plays when an break has finished"_s);
				helpTexts={
					{&arrivalSound,arrivalSoundHelpText},{&selectArrivalSound,arrivalSoundHelpText},{&previewArrivalSound,arrivalSoundHelpText},
					{&cheerVideo,cheerVideoHelpText},{&selectCheerVideo,cheerVideoHelpText},{&previewCheerVideo,cheerVideoHelpText},
					{&portraitVideo,portraitVideoHelpText},{&selectPortraitVideo,portraitVideoHelpText},{&previewPortraitVideo,portraitVideoHelpText},
					{&subscriptionSound,subscriptionSoundHelpText},{&selectSubscriptionSound,subscriptionSoundHelpText},{&previewSubscriptionSound,subscriptionSoundHelpText},
					{&raidSound,raidSoundHelpText},{&selectRaidSound,raidSoundHelpText},{&previewRaidSound,raidSoundHelpText},
					{&postRaidEventDelay,std::make_shared<QString>(u"How long (in milliseconds) to wait after a raid before allowing additional media to be triggered"_s)},
					{&postRaidEventDelayThreshold,std::make_shared<QString>(u"Under this many viewers, there will not be a delay following the raid"_s)},
					{&inactivityCooldown,std::make_shared<QString>(uR"(This is the amount of time (in milliseconds) that must pass without any chat messages before Celeste plays a "roast" video)"_s)},
					{&helpCooldown,std::make_shared<QString>(uR"(This is the amount of time (in milliseconds) between "help" message. A help message is an explanation of a single, randomly chosen command.)"_s)},
					{&textWallThreshold,std::make_shared<QString>(u"This is the sound that plays when a user spams chat with a super long message. The threshold is the number of characters the message needs to be to trigger the sound."_s)},
					{&textWallSound,textWallSoundHelpText},{&selectTextWallSound,textWallSoundHelpText},{&previewTextWallSound,textWallSoundHelpText},
					{&adBreakWarningVideo,adBreakWarningVideoHelpText},{&selectAdBreakWarningVideo,adBreakWarningVideoHelpText},{&previewAdBreakWarningVideo,adBreakWarningVideoHelpText},
					{&adBreakWarningLeadTime,std::make_shared<QString>(u"How many seconds to warn in advance that an ad break is about to begin"_s)},
					{&adBreakFinishedVideo,adBreakFinishedVideoHelpText},{&selectAdBreakFinishedVideo,adBreakFinishedVideoHelpText},{&previewAdBreakFinishedVideo,adBreakFinishedVideoHelpText},
					{&adScheduleRefreshInterval,std::make_shared<QString>(u"How often (in seconds) to ask Twitch for the ad manager schedule"_s)},
					{&monkeyKeyboardBleepRootFrequency,std::make_shared<QString>(u"This will be the frequency that will be treated as A. The notes B through G will be calculated based off of this one. It is common convention for A4 to be 440Hz. Halve this to drop an octave (A3 = 220Hz), double it to raise an octave (A5 = 880Hz). While this is the international standard, you are not required to follow it. You can set this to any value between 20Hz and 20kHz. Also note that bleep is intended to be the higher note, but this is also not a requirement."_s)},
					{&monkeyKeyboardBleepLength,std::make_shared<QString>(u"The length, in milliseconds, of the note. Bleep is intended to be the short note, but this is not required or enforced. Set it to any length you like between 1 millisecond and 1 full second."_s)},
					{&monkeyKeyboardBloopRootFrequency,std::make_shared<QString>(u"This will be the frequency that will be treated as A. The notes B through G will be calculated based off of this one. It is common convention for A4 to be 440Hz. Halve this to drop an octave (A3 = 220Hz), double it to raise an octave (A5 = 880Hz). While this is the international standard, you are not required to follow it. You can set this to any value between 20Hz and 20kHz. Also note that bloop is intended to be the lower note, but this is also not a requirement."_s)},
					{&monkeyKeyboardBloopLength,std::make_shared<QString>(u"The length, in milliseconds, of the note. Bloop is intended to be the long note, but this is not required or enforced. Set it to any length you like between 1 millisecond and 1 full second."_s)},
					{&monkeyKeyboardVolume,std::make_shared<QString>(u"How loud a monkey keyboard note is."_s)}
				};

				connect(&arrivalSound,&PathEdit::Valid,&previewArrivalSound,&QPushButton::setEnabled);
				connect(&arrivalSound,&PathEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectArrivalSound,&QPushButton::clicked,this,&Bot::OpenArrivalSound);
				connect(&previewArrivalSound,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayArrivalSound));
				connect(&portraitVideo,&PathEdit::Valid,&previewPortraitVideo,&QPushButton::setEnabled);
				connect(&portraitVideo,&PathEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectPortraitVideo,&QPushButton::clicked,this,&Bot::OpenPortraitVideo);
				connect(&previewPortraitVideo,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayPortraitVideo));
				connect(&cheerVideo,&PathEdit::Valid,&previewCheerVideo,&QPushButton::setEnabled);
				connect(&cheerVideo,&PathEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectCheerVideo,&QPushButton::clicked,this,&Bot::OpenCheerVideo);
				connect(&previewCheerVideo,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayCheerVideo));
				connect(&subscriptionSound,&PathEdit::Valid,&previewSubscriptionSound,&QPushButton::setEnabled);
				connect(&subscriptionSound,&PathEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectSubscriptionSound,&QPushButton::clicked,this,&Bot::OpenSubscriptionSound);
				connect(&previewSubscriptionSound,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlaySubscriptionSound));
				connect(&raidSound,&PathEdit::Valid,&previewRaidSound,&QPushButton::setEnabled);
				connect(&raidSound,&PathEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectRaidSound,&QPushButton::clicked,this,&Bot::OpenRaidSound);
				connect(&previewRaidSound,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayRaidSound));
				connect(&textWallSound,&PathEdit::Valid,&previewTextWallSound,&QPushButton::setEnabled);
				connect(&textWallSound,&PathEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectTextWallSound,&QPushButton::clicked,this,&Bot::OpenTextWallSound);
				connect(&previewTextWallSound,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayTextWallSound));
				connect(&adBreakWarningVideo,&PathEdit::Valid,&previewAdBreakWarningVideo,&QPushButton::setEnabled);
				connect(&adBreakWarningVideo,&PathEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectAdBreakWarningVideo,&QPushButton::clicked,this,&Bot::OpenAdBreakWarningVideo);
				connect(&previewAdBreakWarningVideo,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayAdBreakWarningVideo));
				connect(&adBreakFinishedVideo,&PathEdit::Valid,&previewAdBreakFinishedVideo,&QPushButton::setEnabled);
				connect(&adBreakFinishedVideo,&PathEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectAdBreakFinishedVideo,&QPushButton::clicked,this,&Bot::OpenAdBreakFinishedVideo);
				connect(&previewAdBreakFinishedVideo,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayAdBreakFinishedVideo));
				connect(&monkeyKeyboardVolume,&QSlider::valueChanged,this,&Bot::MonkeyKeyboardVolumeChanged);
				connect(&monkeyKeyboardNoteA,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayMonkeyKeyboardNote));
				connect(&monkeyKeyboardNoteB,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayMonkeyKeyboardNote));
				connect(&monkeyKeyboardNoteC,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayMonkeyKeyboardNote));
				connect(&monkeyKeyboardNoteD,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayMonkeyKeyboardNote));
				connect(&monkeyKeyboardNoteE,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayMonkeyKeyboardNote));
				connect(&monkeyKeyboardNoteF,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayMonkeyKeyboardNote));
				connect(&monkeyKeyboardNoteG,&QPushButton::clicked,this,QOverload<>::of(&Bot::PlayMonkeyKeyboardNote));

				arrivalSound.setObjectName(u"Arrival Announcement Audio"_s);
				arrivalSound.setText(settings.arrivalSound);
				portraitVideo.setObjectName(u"Portrait (Ping) Video"_s);
				portraitVideo.setText(settings.portraitVideo);
				cheerVideo.setObjectName(u"Cheer (Bits) Video"_s);
				cheerVideo.setText(settings.cheerVideo);
				subscriptionSound.setObjectName(u"Subscription Announcement"_s);
				subscriptionSound.setText(settings.subscriptionSound);
				raidSound.setObjectName(u"Raid Announcement"_s);
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
				textWallSound.setObjectName(u"Wall-of-Text Sound"_s);
				textWallSound.setText(settings.textWallSound);
				adBreakWarningVideo.setObjectName(u"Ad Break Warning Video"_s);
				adBreakWarningVideo.setText(settings.adWarningVideo);
				adBreakWarningLeadTime.setRange(1,std::numeric_limits<int>::max());
				adBreakWarningLeadTime.setValue(settings.adWarningLeadTime);
				adBreakFinishedVideo.setObjectName(u"Ad Break Finished Video"_s);
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
				monkeyKeyboardWaveTypeGroup.addButton(&monkeyKeyboardWaveTypeSine);
				monkeyKeyboardWaveTypeGroup.addButton(&monkeyKeyboardWaveTypeTriangle);
				monkeyKeyboardWaveTypeGroup.addButton(&monkeyKeyboardWaveTypeSawtooth);
				monkeyKeyboardWaveTypeGroup.addButton(&monkeyKeyboardWaveTypeSquare);
				monkeyKeyboardWaveTypeSine.setProperty(STYLE_NO_FILL,true);
				auto waveType=static_cast<QString>(settings.monkeyKeyboardWaveType).toLower();
				for (auto button : monkeyKeyboardWaveTypeGroup.buttons())
				{
					if (waveType == button->text().toLower()) button->setChecked(true);
				}
				monkeyKeyboardPreviewTypeGroup.addButton(&monkeyKeyboardPreviewTypeBleep);
				monkeyKeyboardPreviewTypeGroup.addButton(&monkeyKeyboardPreviewTypeBloop);
				monkeyKeyboardPreviewTypeBleep.setChecked(true);
				monkeyKeyboardPreviewTypeBleep.setProperty(STYLE_NO_FILL,true);
				monkeyKeyboardVolume.setRange(0,100);
				monkeyKeyboardVolume.setValue(settings.monkeyKeyboardVolume);
				monkeyKeyboardVolumeValue.setText(QString::number(monkeyKeyboardVolume.value())+"%");

				Rows({
					{Label(arrivalSound.objectName()),&arrivalSound,&selectArrivalSound,&previewArrivalSound},
					{Label(portraitVideo.objectName()),&portraitVideo,&selectPortraitVideo,&previewPortraitVideo},
					{Label(cheerVideo.objectName()),&cheerVideo,&selectCheerVideo,&previewCheerVideo},
					{Label(subscriptionSound.objectName()),&subscriptionSound,&selectSubscriptionSound,&previewSubscriptionSound},
					{Label(raidSound.objectName()),&raidSound,&selectRaidSound,&previewRaidSound},
					{Label(u"Post-Raid Greeting Delay"_s),&postRaidEventDelay,Label(u"Threshold"_s),&postRaidEventDelayThreshold},
					{Label(u"Inactivity Cooldown"_s),&inactivityCooldown},
					{Label(u"Help Cooldown"_s),&helpCooldown},
					{Label(textWallSound.objectName()),&textWallSound,&selectTextWallSound,&previewTextWallSound,Label(u"Threshold"_s),&textWallThreshold},
					{Subheading(u"Ads"_s)},
					{Label(adBreakWarningVideo.objectName()),&adBreakWarningVideo,&selectAdBreakWarningVideo,&previewAdBreakWarningVideo,Label(u"Lead Time"_s),&adBreakWarningLeadTime},
					{Label(adBreakFinishedVideo.objectName()),&adBreakFinishedVideo,&selectAdBreakFinishedVideo,&previewAdBreakFinishedVideo,Label(u"Refresh Interval"_s),&adScheduleRefreshInterval},
					{Subheading(u"Monkey Keyboard"_s)},
					{Label(u"Bleep Root Frequency"_s),&monkeyKeyboardBleepRootFrequency,Label(u"Bleep Length (ms)"_s),&monkeyKeyboardBleepLength,Label(u"Bloop Root Frequency"_s),&monkeyKeyboardBloopRootFrequency,Label(u"Bloop Length (ms)"_s),&monkeyKeyboardBloopLength},
					{Label(u"Volume"_s),&monkeyKeyboardVolume,&monkeyKeyboardVolumeValue},
					{Label(u"Wave Type"_s),&monkeyKeyboardWaveTypeSine,&monkeyKeyboardWaveTypeTriangle,&monkeyKeyboardWaveTypeSquare,&monkeyKeyboardWaveTypeSawtooth},
					{Label(u"Preview Type"_s),&monkeyKeyboardPreviewTypeBleep,&monkeyKeyboardPreviewTypeBloop},
					{Label(u"Preview Note"_s),&monkeyKeyboardNoteA,&monkeyKeyboardNoteB,&monkeyKeyboardNoteC,&monkeyKeyboardNoteD,&monkeyKeyboardNoteE,&monkeyKeyboardNoteF,&monkeyKeyboardNoteG}
				});
			}

			void Bot::OpenArrivalSound()
			{
				auto selection=OpenAudio(this,arrivalSound.text());
				if (!selection) return;
				arrivalSound.setText(*selection);
			}

			void Bot::OpenArrivalSound()
			{
				auto selection=OpenAudio(this,arrivalSound.text());
				if (!selection) return;
				arrivalSound.setText(*selection);
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
				auto selection=OpenVideo(this,portraitVideo.text());
				if (!selection) return;
				portraitVideo.setText(*selection);
			}

			void Bot::PlayPortraitVideo()
			{
				emit PlayPortraitVideo(portraitVideo.text());
			}

			void Bot::OpenCheerVideo()
			{
				auto selection=OpenVideo(this,cheerVideo.text());
				if (!selection) return;
				cheerVideo.setText(*selection);
			}

			void Bot::PlayCheerVideo()
			{
				emit PlayCheerVideo(qApp->applicationName(),100,"Hype!",cheerVideo.text());
			}

			void Bot::OpenSubscriptionSound()
			{
				auto selection=OpenAudio(this,subscriptionSound.text());
				if (!selection) return;
				subscriptionSound.setText(*selection);
			}

			void Bot::PlaySubscriptionSound()
			{
				emit PlaySubscriptionSound(qApp->applicationName(),subscriptionSound.text());
			}

			void Bot::OpenRaidSound()
			{
				auto selection=OpenAudio(this,raidSound.text());
				if (!selection) return;
				raidSound.setText(*selection);
			}

			void Bot::PlayRaidSound()
			{
				emit PlayRaidSound(qApp->applicationName(),100,raidSound.text());
			}

			void Bot::OpenTextWallSound()
			{
				auto selection=OpenAudio(this,textWallSound.text());
				if (!selection) return;
				textWallSound.setText(*selection);
			}

			void Bot::PlayTextWallSound()
			{
				QString message("Celeste ");
				message=message.repeated(textWallThreshold.value()/message.size());
				emit PlayTextWallSound(message,textWallSound.text());
			}

			void Bot::OpenAdBreakWarningVideo()
			{
				auto selection=OpenVideo(this,adBreakWarningVideo.text());
				if (!selection) return;
				adBreakWarningVideo.setText(*selection);
			}

			void Bot::PlayAdBreakWarningVideo()
			{
				emit PlayAdBreakWarningVideo(adBreakWarningVideo.text());
			}

			void Bot::OpenAdBreakFinishedVideo()
			{
				auto selection=OpenVideo(this,adBreakFinishedVideo.text());
				if (!selection) return;
				adBreakFinishedVideo.setText(*selection);
			}

			void Bot::PlayAdBreakFinishedVideo()
			{
				emit PlayAdBreakFinishedVideo(adBreakFinishedVideo.text());
			}

			void Bot::MonkeyKeyboardVolumeChanged(int value)
			{
				monkeyKeyboardVolumeValue.setText(QString::number(value)+"%");
			}

			void Bot::PlayMonkeyKeyboardNote()
			{
				auto note=qobject_cast<QPushButton*>(sender());
				if (monkeyKeyboardPreviewTypeBleep.isChecked())
				{
					emit PlayMonkeyKeyboardNote(std::chrono::milliseconds(settings.monkeyKeyboardBleepLength),settings.monkeyKeyboardBleepRootFrequency,note->text());

				}
				else
				{
					emit PlayMonkeyKeyboardNote(std::chrono::milliseconds(settings.monkeyKeyboardBloopLength),settings.monkeyKeyboardBloopRootFrequency,note->text());
				}
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
				settings.monkeyKeyboardWaveType.Set(qobject_cast<QRadioButton*>(monkeyKeyboardWaveTypeGroup.checkedButton())->text());

			}

			Pulsar::Pulsar(Settings::Pulsar &settings,Feedback::Error &errorReport) : Category(u"Pulsar"_s,errorReport),
				settings(settings),
				subsystemEnabled(this),
				reconnectDelay(this)
			{
				helpTexts={
					{&subsystemEnabled,std::make_shared<QString>(u"Check this to enable the Pulsar plugin which allows the bot to communicate with OBS Studio. This will only work if the Pulsar plugin was installed when the bot was installed."_s)},
					{&reconnectDelay,std::make_shared<QString>(u"How long (in milliseconds) to wait until attempting to reconnect to the OBS plugin after a failed attempt"_s)}
				};

				subsystemEnabled.setChecked(settings.enabled);
				reconnectDelay.setRange(1,std::numeric_limits<int>::max());
				reconnectDelay.setValue(settings.reconnectDelay);

				Rows({
					{Label(u"Use Pulsar"_s),&subsystemEnabled},
					{Label(u"Reconnect Delay"_s),&reconnectDelay}
				});
			}

			void Pulsar::Save()
			{
				settings.enabled.Set(subsystemEnabled.isChecked());
				settings.reconnectDelay.Set(reconnectDelay.value());
			}

			Log::Log(Settings settings,Feedback::Error &errorReport) : Category(u"Logging"_s,errorReport),
				directory(this),
				selectDirectory(Text::BROWSE,this),
				settings(settings)
			{
				auto directoryHelpText=std::make_shared<QString>(u"The folder where the bot will store log files, one log file per day. The bot logs to the file for the day the bot was launched."_s);
				helpTexts={
					{&directory,directoryHelpText},{&selectDirectory,directoryHelpText}
				};

				connect(&directory,&DirectoryEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectDirectory,&QPushButton::clicked,&directory,&DirectoryEdit::Browse);

				directory.setObjectName("Log file directory");
				directory.setText(settings.directory);

				Rows({
					{Label(u"Folder"_s),&directory,&selectDirectory}
				});
			}

			void Log::Save()
			{
				settings.directory.Set(directory.text());
			}

			Security::Security(::Security &settings,Feedback::Error &errorReport) : Category(u"Security"_s,errorReport),
				administrator(this),
				clientID(this),
				token(this),
				callbackURL(this),
				permissions(this),
				selectPermissions(Text::CHOOSE,this),
				settings(settings)
			{
				details->setVisible(false);

				auto permissionsHelpText=std::make_shared<QString>(uR"(The list of permissions (Twitch refers to as "scopes") the bot will require.)"_s);
				helpTexts={
					{&administrator,std::make_shared<QString>(u"Twitch user name of the broadcaster."_s)},
					{&clientID,std::make_shared<QString>(u"Client ID from Twitch developer console."_s)},
					{&token,std::make_shared<QString>(uR"(OAuth token obtained from Twitch authorization process (usually automatic, but can be manually obtained and entered). This is for "Authorization code grant flow" for Celeste's main API calls.)"_s)},
					{&callbackURL,std::make_shared<QString>(u"The URL Twitch will contact with an OAuth token (or error message)."_s)},
					{&permissions,permissionsHelpText},{&selectPermissions,permissionsHelpText}
				};

				connect(&administrator,&RequiredEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&clientID,&RequiredEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&token,&RequiredEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&permissions,&RequiredEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);
				connect(&selectPermissions,&QPushButton::clicked,this,&Security::SelectPermissions);
				connect(&callbackURL,&WebAddressEdit::NeedValidatedStyle,&errorReport,&Feedback::Error::StyleSource);

				administrator.setObjectName(u"Administrator account name missing");
				administrator.setText(settings.Administrator());
				clientID.setObjectName(u"Client ID is missing");
				clientID.setText(settings.ClientID());
				clientID.setEchoMode(QLineEdit::Password);
				token.setObjectName(u"Authorization token is missing");
				token.setText(settings.OAuthToken());
				token.setEchoMode(QLineEdit::Password);
				permissions.setObjectName(u"Twitch scope list is missing");
				permissions.setText(settings.Scope());
				callbackURL.setObjectName("Security callback URL");
				callbackURL.setText(settings.CallbackURL());
				callbackURL.setInputMethodHints(Qt::ImhUrlCharactersOnly);

				Rows({
					{Label(u"Administrator (Broascaster)"_s),&administrator},
					{Label(u"Client ID"_s),&clientID},
					{Label(u"OAuth Token"_s),&token},
					{Label(u"Callback URL"_s),&callbackURL},
					{Label(u"Permissions"_s),&permissions,&selectPermissions}
				});
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
		}

		Dialog::Dialog(std::vector<Categories::Category*> categories,std::unique_ptr<Feedback::Error> errorReport,QWidget *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint),
			entriesFrame(this),
			help(this),
			buttons(this),
			discard(Text::BUTTON_DISCARD,this),
			save(Text::BUTTON_SAVE,this),
			apply(Text::BUTTON_APPLY,this),
			scrollLayout(nullptr),
			errorBox("Problems",this),
			errorMessages(&errorBox),
			errorReport(std::move(errorReport)),
			categories(std::move(categories))
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
			for (auto category : this->categories)
			{
				scrollLayout->addWidget(category);
				connect(category,&Categories::Category::Help,&help,&Feedback::Help::Message);
				categories.push_back(category);
			}

			QWidget *rightPane=new QWidget(this);
			QGridLayout *rightLayout=new QGridLayout(rightPane);
			rightPane->setLayout(rightLayout);
			rightLayout->addWidget(&help,0,0,1,2);
			QVBoxLayout *errorBoxLayout=new QVBoxLayout(&errorBox);
			errorBox.setLayout(errorBoxLayout);
			errorBox.setVisible(false);
			connect(this->errorReport.get(),&Feedback::Error::Clear,&save,&QPushButton::setEnabled);
			connect(this->errorReport.get(),&Feedback::Error::Clear,&apply,&QPushButton::setEnabled);
			connect(this->errorReport.get(),&Feedback::Error::Count,&errorBox,&QGroupBox::setVisible);
			connect(this->errorReport.get(),&Feedback::Error::ReportProblem,&errorMessages,&QLabel::setText);
			errorBoxLayout->addWidget(&errorMessages);
			rightLayout->addWidget(&errorBox,1,0,1,2);
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

		void Dialog::Save()
		{
			for (Categories::Category *category : categories) category->Save();
			emit Refresh();
		}
	}
}
