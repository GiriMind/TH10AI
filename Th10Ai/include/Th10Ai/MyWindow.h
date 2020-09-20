#pragma once

#include "Th10Ai/Common.h"

#include <wx/wx.h>

#include "Th10Ai/Th10Types.h"

namespace th
{
	class MyWindow :
		public wxWindow
	{
	public:
		MyWindow(wxWindow* parent);
		virtual ~MyWindow();

		void update(const StatusData& status);

	private:
		void onPaint(wxPaintEvent& event);
		void onEraseBackground(wxEraseEvent& event);

		wxSize m_size;
		wxBitmap m_buffer;

		StatusData m_status;

		wxDECLARE_EVENT_TABLE();
	};
}