#pragma once

#include <QVariant>

// can't put subsystem in a module because good ol' Qt not supporting QObjects in modules
namespace Subsystem
{
	namespace Interchange
	{
		class Transaction : public QObject
		{
			Q_OBJECT // can't make this general purpose without variants because good ol' QObject not supporting templates
		public:
			Transaction(const QVariant &subject) : QObject(nullptr), subject(subject) { }
			void Close();
			QVariant subject;
			QVariant context;
		signals:
			void AcknowledgeClosure(const QVariant &subject);
		};
	}
}
