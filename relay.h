#pragma once

#include <QString>
#include <QTimer>
#include <memory>
#include "globals.h"

namespace Relay
{
	namespace Status
	{
		class Context : public QObject
		{
			Q_OBJECT
		public:
			Context(const QString &text,QObject *parent=nullptr) : QObject(parent), text(text)
			{
				timer.setInterval(TimeConvert::Milliseconds(static_cast<std::chrono::seconds>(1)));
				connect(&timer,&QTimer::timeout,this,&Context::UpdateRequested);
			}
			void Trigger()
			{
				Update(text);
			}
			void Update(const QString &text)
			{
				emit Updated(text);
				timer.start();
			}
		protected:
			QTimer timer;
			QString text;
		signals:
			void Updated(const QString &text);
			void UpdateRequested();
		};

		using Package=std::unique_ptr<Context,void(*)(Context*)>;

		inline void Dismiss(Context *context)
		{
			context->disconnect();
			context->deleteLater();
		};
	}
}
