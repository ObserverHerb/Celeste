#include "panes.h"

#include <QVBoxLayout>
#include <QStackedLayout>
#include <QLabel>

/*!
 * \class PersistentPane
 * \brief Abstract base class that forms the foundation of long lived panes in
 * the system
 *
 * A pane fills the primary viewing area with a group of related content,
 * similar to the way a fullscreen window does on a desktop system or an
 * activity does on Android.
 *
 * Panes are organized into stacks, the top most always being the one that
 * is visible. All panes must ultimately derives from this interface and
 * provide implementations for the pure virtual functions.
 */

/*!
 * \fn PersistentPane::PersistentPane
 * \brief Default constructor
 *
 * You won't be able to manually construct a pane. It serves solely as an
 * interface.
 *
 * \param parent This pane will be destroyed when the QWidget pointed to by
 * this pointer is destroyed.
 */

/*!
 * \fn PersistentPane::Print
 * \brief Default text output for a pane
 *
 * Most panes will use this to display text on screen in some manner.
 *
 * \param text The message to display
 */

/*!
 * \class StatusPane
 * \brief A terminal-like pane that displays scrolling text
 *
 * This pane is useful for displaying system or configuration information
 * in realtime.
 */

//! QSettings category for any settings pertaining to the ChatPane
const QString StatusPane::SETTINGS_CATEGORY="StatusPane";

/*!
 * \fn StatusPane::StatusPane
 * \brief Default constructor
 *
 * Sets some basic formatting, the Qt layout, and initializes the text box
 * control that will display the content.
 *
 * \param parent This pane will be destroyed when the QWidget pointed to by
 * this pointer is destroyed.
 */
StatusPane::StatusPane(QWidget *parent) : PersistentPane(parent),
	settingFont(SETTINGS_CATEGORY,"Font","Droid Sans Mono"),
	settingFontSize(SETTINGS_CATEGORY,"FontSize",10),
	settingForegroundColor(SETTINGS_CATEGORY,"ForegroundColor","#ffffffff"),
	settingBackgroundColor(SETTINGS_CATEGORY,"BackgroundColor","#ff000000")
{
	output=new QTextEdit(this);
	output->setStyleSheet(StyleSheet::Colors(settingForegroundColor,settingBackgroundColor));
	output->setFontFamily(settingFont);
	output->setFontPointSize(settingFontSize);
	output->document()->setDocumentMargin(16);
	output->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	output->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	output->setFrameStyle(QFrame::NoFrame);

	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->addWidget(output);
}

/*!
 * \fn StatusPane::Print
 * \brief Adds a string of text to the end of the text currently being displayed
 * \param text The text to add
 */
void StatusPane::Print(const QString &text)
{
	output->insertPlainText(text);
	output->insertPlainText("\n");
	output->ensureCursorVisible();
}

/*!
 * \class ChatPane
 * \brief Displays a scrolling chat history
 *
 * This is likely the main pane that will be visible most of the time the
 * bot is operational. It consists of a main chat message area, an agenda
 * banner at the top that is meant to communicate the current objective of the
 * stream, and a status area at the bottom that can communicate the type of
 * information that would normally be placed in a StatusPane
 */

//! QSettings category for any settings pertaining to the ChatPane
const QString ChatPane::SETTINGS_CATEGORY="ChatPane";

/*!
 * \fn ChatPane::ChatPane
 * \brief Default constructor
 *
 * Creates three control that display output in a vertical layout and set
 * initial formatting of those control. Also initializes the QTimer that
 * manages how long text in the bottom status control is visible.
 *
 * \param parent This pane will be destroyed when the QWidget pointed to by
 * this pointer is destroyed.
 */
ChatPane::ChatPane(QWidget *parent) : PersistentPane(parent),
	agenda(nullptr),
	chat(nullptr),
	status(nullptr),
	settingFont(SETTINGS_CATEGORY,"Font","Copperplate Gothic Bold"),
	settingFontSize(SETTINGS_CATEGORY,"FontSize",12),
	settingForegroundColor(SETTINGS_CATEGORY,"ForegroundColor","#ffffffff"),
	settingBackgroundColor(SETTINGS_CATEGORY,"BackgroundColor","#ff000000"),
	settingStatusInterval(SETTINGS_CATEGORY,"StatusInterval",5000)
{
	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->setSpacing(0);

	agenda=new QLabel(this);
	agenda->setStyleSheet(StyleSheet::Colors(settingForegroundColor,settingBackgroundColor));
	agenda->setFont(QFont(settingFont,static_cast<qreal>(settingFontSize)*1.666,QFont::Bold));
	agenda->setMargin(16);
	agenda->setAlignment(Qt::AlignCenter);
	agenda->setWordWrap(true);
	agenda->hide();
	layout()->addWidget(agenda);

	chat=new ScrollingTextEdit(this);
	chat->setStyleSheet(StyleSheet::Colors(settingForegroundColor,settingBackgroundColor));
	chat->setFontFamily(settingFont);
	chat->setFontPointSize(settingFontSize);
	chat->document()->setDefaultStyleSheet(QString("div.user { font-family: '%1'; font-size: %2pt; } div.message, span.message { font-family: '%1'; font-size: %3pt; }").arg(static_cast<QString>(settingFont),StringConvert::Integer(static_cast<int>(settingFontSize)*1.333),StringConvert::Integer(static_cast<int>(settingFontSize))));
	chat->document()->setDocumentMargin(static_cast<qreal>(settingFontSize)*1.333);
	chat->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	chat->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	chat->setFrameStyle(QFrame::NoFrame);
	chat->setCursorWidth(0);
	layout()->addWidget(chat);

	status=new QLabel(this);
	status->setStyleSheet(StyleSheet::Colors(settingForegroundColor,settingBackgroundColor));
	status->setFont(QFont(settingFont,static_cast<qreal>(settingFontSize)*0.833)); // QLabel doesn't have setFontFamily()
	status->setMargin(16);
	status->setAlignment(Qt::AlignCenter);
	status->setWordWrap(true);
	status->setTextFormat(Qt::RichText);
	status->hide();
	layout()->addWidget(status);

	ResetStatusClock();
	connect(&statusClock,&QTimer::timeout,this,&ChatPane::DismissAlert);
}

void ChatPane::ResetStatusClock()
{
	statusClock.setInterval(TimeConvert::Interval(static_cast<std::chrono::milliseconds>(settingStatusInterval)));
}

/*!
 * \fn ChatPane::SetAgenda
 * \brief Sets an objective in the top section of the ChatPane
 * \param text Agenda to display
 */
void ChatPane::SetAgenda(const QString &text)
{
	if (text.isEmpty())
	{
		agenda->clear();
		agenda->hide();
	}
	else
	{
		agenda->setText(text);
		agenda->show();
	}
}

void ChatPane::Refresh()
{
	chat->viewport()->update();
}

/*!
 * \fn ChatPane::Print
 * \brief Adds a message to the end of the messages in the middle section of
 * the ChatPane
 * \param text The message to add
 */
void ChatPane::Print(const QString &text)
{
	chat->insertHtml(text);
	chat->insertPlainText("\n");
}

/*!
 * \fn ChatPane::Alert
 * \brief Adds a message to be displayed in the bottom section of the ChatPane
 *
 * The message will be displayed immediately unless another message is already
 * visible. If another message is visible, this message will be added to a
 * queue until the statusClock has triggered the removal of the message in
 * front of it in the queue. This way, all messages are displayed in order,
 * long enough for them to be read.
 *
 * \param text The message to be displayed
 * \return Relay::Status::Context* A pointer to a context for this message
 * which can be used to alter the message before it's removed.
 */
Relay::Status::Context* ChatPane::Alert(const QString &text)
{
	status->show();
	Relay::Status::Context *statusUpdate=new Relay::Status::Context(text,this);
	connect(statusUpdate,&Relay::Status::Context::ResetClock,this,&ChatPane::ResetStatusClock);
	statusUpdates.push(Relay::Status::Package(statusUpdate,&Relay::Status::Dismiss));
	if (statusUpdates.size() == 1)
	{
		connect(statusUpdate,&Relay::Status::Context::Updated,status,&QLabel::setText);
		statusClock.start();
		statusUpdate->Trigger();
	}
	return statusUpdate;
}

/*!
 * \fn ChatPane::DismissAlert
 * \brief Removes the visible status from the queue and displays the next one
 * if one exists.
 */
void ChatPane::DismissAlert()
{
	statusUpdates.pop();
	if (statusUpdates.empty())
	{
		status->clear();
		status->hide();
		statusClock.stop();
		return;
	}
	connect(statusUpdates.front().get(),&Relay::Status::Context::Updated,status,&QLabel::setText);
	statusUpdates.front()->Trigger();
}

/*!
 * \class EphemeralPane
 * \brief Abstract base class that forms the foundation for all panes which
 * are displayed for a finite period of time.
 *
 * Ephemeral panes the are only visible for a set period of time or until
 * another event is triggered. Any pane which is only temporarily visible
 * and does not constitute a context switch of some kind, like switching from
 * authorization to chatting, qualifies as an ephemeral pane.
 */

/*!
 * \fn EphemeralPane::Show
 * \brief Displays the pane
 *
 * Note that the pane may not immediately become visible if other panes are
 * ahead of it in the queue of panes to be displayed.
 */

/*!
 * \fn EhpemeralPane::Finished
 * \brief Provides a way to clean up after the pane when it has completed its
 * operation.
 *
 * This is primary used to deallocate the object when all of its messages have
 * been processed from Qt's message queue.
 */

/*!
 * \fn EphemeralPane::EphemeralPane
 * \brief Default constructor
 *
 * Creates a pane that is initially not visible and configures it to deallocate
 * itself once it is finished with its operation.
 *
 * \param parent This pane will be destroyed when the QWidget pointed to by
 * this pointer is destroyed.
 */
EphemeralPane::EphemeralPane(QWidget *parent) : QWidget(parent)
{
	setVisible(false);
	connect(this,&EphemeralPane::Finished,this,&EphemeralPane::deleteLater);
}

/*!
 * \class VideoPane
 * \brief Displays a video
 */

/*!
 * \fn VideoPane::VideoPane
 * \brief Primary constructor
 * \param path File path or URL to the video to be played
 * \param parent This pane will be destroyed when the QWidget pointed to by
 * this pointer is destroyed.
 */
VideoPane::VideoPane(const QString &path,QWidget *parent) : EphemeralPane(parent), videoPlayer(new QMediaPlayer(this)), viewport(new QVideoWidget(this))
{
	videoPlayer->setVideoOutput(viewport);
	videoPlayer->setMedia(QUrl::fromLocalFile(path));
	connect(videoPlayer,&QMediaPlayer::stateChanged,[this](QMediaPlayer::State state) {
		if (state == QMediaPlayer::StoppedState) emit Finished();
	});

	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->addWidget(viewport);
}

/*!
 * \fn VideoPane::Show
 * \brief Makes the pane visible and starts the assigned video.
 */
void VideoPane::Show()
{
	show();
	videoPlayer->play();
}


/*!
 * \class AnnouncePane
 * \brief Briefly displays a message that fills the pane
 *
 * Use this for significant notifications that are best served by large,
 * attention-grabbing text.
 */

//! QSettings category for any settings pertaining to the AnnouncePane
const QString AnnouncePane::SETTINGS_CATEGORY="AnnouncePane";

/*!
 * \fn AnnouncePane::AnnouncePane
 * \brief Primary constructor
 * \param text Message to be displayed
 * \param parent This pane will be destroyed when the QWidget pointed to by
 * this pointer is destroyed.
 */
AnnouncePane::AnnouncePane(const QString &text,QWidget *parent) : EphemeralPane(parent),
	output(new QLabel(text,this)),
	settingDuration(SETTINGS_CATEGORY,"Duration",5000),
	settingFont(SETTINGS_CATEGORY,"Font","Copperplate Gothic Bold"),
	settingFontSize(SETTINGS_CATEGORY,"FontSize",20),
	settingForegroundColor(SETTINGS_CATEGORY,"ForegroundColor","#ffffffff"),
	settingBackgroundColor(SETTINGS_CATEGORY,"BackgroundColor","#ff000000")
{
	setLayout(new QVBoxLayout(this));
	layout()->setContentsMargins(0,0,0,0);
	layout()->setSpacing(0);

	output->setStyleSheet(StyleSheet::Colors(settingForegroundColor,settingBackgroundColor));
	output->setFont(QFont(settingFont,settingFontSize,QFont::Bold));
	output->setMargin(16);
	output->setAlignment(Qt::AlignCenter);
	output->setWordWrap(true);
	output->setTextFormat(Qt::RichText);

	clock.setSingleShot(true);
	clock.setInterval(TimeConvert::Interval(static_cast<std::chrono::milliseconds>(settingDuration)));
	connect(&clock,&QTimer::timeout,this,&AnnouncePane::Finished);
}

AnnouncePane::AnnouncePane(const std::vector<std::pair<QString,double>> &lines,QWidget *parent) : AnnouncePane("",parent)
{
	output->setText(BuildParagraph(lines));
}


/*!
 * \fn AnnouncePane::Show
 * \brief Displays the pane
 *
 * This override of the EphemeralPane::Show() function starts a timer that
 * triggers the eventual removal of the pane from view, as well as allowing an
 * opportunity to do some last moment initialization.
 */
void AnnouncePane::Show()
{
	Polish();
	show();
	clock.start();
}

/*!
 * \fn AnnouncePane::Polish
 * \brief Performs and last moment initialization just before the pane is displayed.
 *
 * This is ideal for initialization that cannot be performed at construction of
 * the pane object, but must be completed before the pane is visible.
 */
void AnnouncePane::Polish()
{
	layout()->addWidget(output);
}

const QString AnnouncePane::BuildParagraph(const std::vector<std::pair<QString,double>> &lines)
{
	QString paragraph;
	for (const std::pair<QString,double> &line : lines)
	{
		if (line.second == 1)
			paragraph.append(QString("%1").arg(line.first));
		else
			paragraph.append(QString(R"(<span style="font-size: %2pt;">%1</span>)").arg(line.first,StringConvert::Integer(static_cast<int>(settingFontSize)*line.second)));
	}
	return QString("<div style='line-height: 1.25;'>%1</div>").arg(paragraph);
}

/*!
 * \class AudioAnnouncePane
 * \brief Similar to an AnnouncePane, but plays audio along with the message
 * displayed
 */

/*!
 * \fn AudioAnnouncePane::AudioAnnouncePane
 * \brief Primary constructor
 * \param text The message to be displayed
 * \param path Path or URL to an audio file that will accompany the message
 * that is displayed
 * \param parent This pane will be destroyed when the QWidget pointed to by
 * this pointer is destroyed.
 */
AudioAnnouncePane::AudioAnnouncePane(const QString &text,const QString &path,QWidget *parent) : AnnouncePane(text,parent), audioPlayer(new QMediaPlayer())
{
	audioPlayer->setMedia(QUrl::fromLocalFile(path));
	connect(audioPlayer,&QMediaPlayer::stateChanged,[this](QMediaPlayer::State state) {
		if (state == QMediaPlayer::StoppedState) emit Finished();
	}); // TODO: report loading failures to status section of chat pane
}

AudioAnnouncePane::AudioAnnouncePane(const std::vector<std::pair<QString,double>> &lines,const QString &path,QWidget *parent) : AudioAnnouncePane("",path,parent)
{
	output->setText(BuildParagraph(lines));
}

/*!
 * \fn AudioAnnouncePane::Show
 * \brief Displays the pane and starts playing the assigned audio
 */
void AudioAnnouncePane::Show()
{
	Polish();
	show();
	audioPlayer->play(); // FIXME: catch errors above and don't play if the file failed to load
}

/*!
 * \class ImageAnnouncePane
 * \brief Similar to an AnnouncePane, but displays an image behind the message
 */

/*!
 * \fn ImageAnnouncePane::ImageAnnouncePane
 * \brief Primary constructor
 * \param text The message to be displayed
 * \param image Reference to a QImage to be displayed behind the text
 * \param parent This pane will be destroyed when the QWidget pointed to by
 * this pointer is destroyed.
 */
ImageAnnouncePane::ImageAnnouncePane(const QString &text,const QImage &image,QWidget *parent) : AnnouncePane(text,parent), view(nullptr), stack(nullptr), shadow(nullptr), image(image)
{
	stack=new QStackedWidget(this);
	dynamic_cast<QStackedLayout*>(stack->layout())->setStackingMode(QStackedLayout::StackAll);

	scene=new QGraphicsScene(this);
	int coverSize=std::max(size().width(),size().height());
	scene->addPixmap(QPixmap::fromImage(QImage(image).scaled(QSize(coverSize,coverSize))));
	view=new QGraphicsView(scene);
	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	shadow=new QGraphicsDropShadowEffect();
	shadow->setBlurRadius(10);
	shadow->setOffset(0,0);
	output->setGraphicsEffect(shadow);
}

ImageAnnouncePane::ImageAnnouncePane(const std::vector<std::pair<QString,double>> &lines,const QImage &image,QWidget *parent) : ImageAnnouncePane("",image,parent)
{
	output->setText(BuildParagraph(lines));
}

/*!
 * \fn ImageAnnouncePane::Polish
 * \brief Performs and last moment initialization just before the pane is displayed.
 *
 * Sets some intricate details such as drop shadows and adds the initialized
 * text to a stacked layout so the text appears to be layered on top of the
 * image.
 */
void ImageAnnouncePane::Polish()
{
	shadow->setColor(accentColor);
	stack->addWidget(output);
	stack->addWidget(view);
	layout()->addWidget(stack);
}
