#include "Base/Windows/WindowsError.h"

#include "Base/Windows/Apis.h"

namespace base
{
	namespace win
	{
		WindowsError::WindowsError(DWORD errorCode) :
			Exception(nullptr),
			m_errorCode(errorCode)
		{
		}

		void WindowsError::print(std::ostream& os) const
		{
			os << '[' << m_errorCode << ']' << Apis::GetErrorDesc(m_errorCode);

			m_sourceLocation.print(os);
			m_stackTrace.print(os);
		}
	}
}