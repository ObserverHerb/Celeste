#pragma once

#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QTimer>
#include <queue>
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

const QString SETTINGS_CATEGORY_CHAT_PANE="ChatPane";

class ChatPane : public Pane
{
	Q_OBJECT
public:
	ChatPane(QWidget *parent);
protected:
	QLabel *agenda;
	QTextEdit *chat;
	QLabel *status;
	std::queue<QString> statuses;
	QTimer statusClock;
	Setting settingStatusInterval;
public slots:
	void Print(const QString text) override;
	void Alert(const QString &text);
protected slots:
	void DismissAlert();
};

class EphemeralPane : public Pane
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
	VideoPane(const QString path,QWidget *parent=nullptr);
	void Show() override;
protected:
	QMediaPlayer *mediaPlayer;
	QVideoWidget *viewport;
public slots:
	void Print(const QString text) override { } // right now this does nothing but might be useful for giving context to the video
};
