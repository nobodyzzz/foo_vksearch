#include "StdAfx.h"
#include "vkApp.h"


vkApp::vkApp( searchOptions& opts )
{
	m_opts = opts;
}

bool vkApp::OnInit()
{
	if( wxApp::OnInit()){
		vkSearchWindow *vkSearch = new vkSearchWindow(TEXT("vk.com audio search"), m_opts);
		
		vkSearch->Show(true);
		return true;
	}
	return false;
}