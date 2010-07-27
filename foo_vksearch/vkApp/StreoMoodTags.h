#pragma once
#include "wx/wx.h"
#include "wx/html/htmlpars.h"
#include <map>

#define EMPTY {}


class StreoMoodTags : public wxHtmlParser
{
	std::map<wxString, wxString> m_stereomoodLinksMap;
	wxString m_src;
public:
	StreoMoodTags(wxString src);
	void AddString(wxString txt) EMPTY
	void AddText(const wxChar* txt) EMPTY
	void AddTagAndLink(wxString tag, wxString link);
	wxObject *GetProduct(void) { return NULL;}
	std::map<wxString, wxString>& GetTagLinksMap();
};

class LinkTagHandler : public wxHtmlTagHandler
{
	StreoMoodTags* m_parser;
public:
	LinkTagHandler(StreoMoodTags* parser = NULL);
	wxString GetSupportedTags();
	bool HandleTag(const wxHtmlTag& tag);
};