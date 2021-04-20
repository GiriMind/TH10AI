#include "Base/Windows/StackTrace.h"

#include <DbgHelp.h>

#pragma comment(lib, "DbgHelp.lib")

namespace base
{
	namespace win
	{
		StackTrace::StackTrace() :
			ThreadLocal(this)
		{
#ifdef _DEBUG
			m_size = CaptureStackBackTrace(1, BUFFER_SIZE, m_frames, nullptr);
#else
			m_size = CaptureStackBackTrace(0, BUFFER_SIZE, m_frames, nullptr);
#endif
		}

		void StackTrace::print(std::ostream& os) const
		{
			//todo �߳�ͬ���������ʽ��
			HANDLE process = GetCurrentProcess();

			SymSetOptions(SYMOPT_LOAD_LINES);
			SymInitialize(process, nullptr, TRUE);

			BYTE buffer[sizeof(SYMBOL_INFO) + sizeof(CHAR) * (MAX_SYM_NAME + 1)] = {};
			PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(buffer);
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = MAX_SYM_NAME;

			IMAGEHLP_LINE64 line = {};
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

			DWORD64 symDisplacement = 0;
			DWORD lineDisplacement = 0;

			os << "StackTrace:\n";
			for (WORD i = 0; i < m_size; ++i)
			{
				DWORD64 address = reinterpret_cast<DWORD64>(m_frames[i]);

				SymFromAddr(process, address, &symDisplacement, symbol);
				SymGetLineFromAddr64(process, address, &lineDisplacement, &line);

				os << i + 1
					<< " in " << symbol->Name
					<< " at " << line.FileName
					<< " : " << line.LineNumber << '\n';
			}

			SymCleanup(process);
		}
	}
}