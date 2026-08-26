#include "subsystem.h"

namespace Subsystem
{
	namespace Interchange
	{
		void Transaction::Close()
		{
			emit AcknowledgeClosure(subject);
			deleteLater();
		}
	}
}
