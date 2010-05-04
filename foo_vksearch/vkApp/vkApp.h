#pragma once
#include "vkSearchWindow.h"

class vkApp : 	public wxApp
{
private:
	searchOptions m_opts;
public:
	vkApp(searchOptions& opts);
	virtual bool OnInit();

};
