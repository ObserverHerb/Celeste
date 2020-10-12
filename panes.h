#pragma once

#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QStackedWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsDropShadowEffect>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QTimer>
#include <queue>
#include <unordered_map>
#include "relay.h"
#include "settings.h"

class Pane : public QWidget
{
	Q_OBJECT
public:
	Pane(QWidget *parent=nullptr) : QWidget(parent) { }
public slots:
	virtual void Print(const QString text)=0;
};

class StatusPane : public Pane
{
	Q_OBJECT
public:
	StatusPane(QWidget *parent);
protected:
	QTextEdit *output;
public slots:
	void Print(const QString text) override;
};

class ChatPane : public Pane
{
	Q_OBJECT
public:
	ChatPane(QWidget *parent);
	void SetAgenda(const QString &text);
protected:
	QLabel *agenda;
	QTextEdit *chat;
	QLabel *status;
	std::queue<Relay::Status::Package> statusUpdates;
	QTimer statusClock;
	Setting settingStatusInterval;
	static const QString SETTINGS_CATEGORY;
public slots:
	void Print(const QString text) override;
	Relay::Status::Context* Alert(const QString &text);
protected slots:
	void DismissAlert();
};

class EphemeralPane : public QWidget
{
	Q_OBJECT
public:
	EphemeralPane(QWidget *parent=nullptr);
	virtual void Show()=0;
signals:
	void Finished();
};

class VideoPane : public EphemeralPane
{
	Q_OBJECT
public:
	VideoPane(const QString &path,QWidget *parent=nullptr);
	void Show() override;
protected:
	QMediaPlayer *videoPlayer;
	QVideoWidget *viewport;
};

class AnnouncePane : public EphemeralPane
{
	Q_OBJECT
public:
	AnnouncePane(const QString &text,QWidget *parent=nullptr);
	void Show() override;
	virtual void Polish();
	void AccentColor(const QColor &color) { accentColor=color; }
protected:
	QColor accentColor;
	QLabel *output;
	QTimer clock;
	Setting settingDuration;
	static const QString SETTINGS_CATEGORY;
};

class AudioAnnouncePane : public AnnouncePane
{
	Q_OBJECT
public:
	AudioAnnouncePane(const QString &text,const QString &path,QWidget *parent=nullptr);
	void Show() override;
protected:
	QMediaPlayer *audioPlayer;
};

class ImageAnnouncePane : public AnnouncePane
{
	Q_OBJECT
public:
	ImageAnnouncePane(const QString &text,const QImage &image,QWidget *parent=nullptr);
	void Polish() override;
protected:
	QGraphicsScene *scene;
	QGraphicsView *view;
	QStackedWidget *stack;
	QGraphicsDropShadowEffect *shadow;
	QImage image;
};
