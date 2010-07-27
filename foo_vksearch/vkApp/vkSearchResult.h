#pragma once
#include "wx/listctrl.h"

class vkSearchResult : public wxListView
{
public:
	vkSearchResult( wxWindow *parent,
		wxWindowID winid = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxLC_REPORT,
		const wxValidator& validator = wxDefaultValidator,
		const wxString &name = wxListCtrlNameStr) : wxListView(parent, winid, pos, size, style, validator, name)
	{
		
	}

	long GetListLastSelectedItem( long geometry );
	void DeselectAll();
	void SelectAll();

};
