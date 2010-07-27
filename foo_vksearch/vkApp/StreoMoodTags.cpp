#include "StdAfx.h"
#include "StreoMoodTags.h"

LinkTagHandler::LinkTagHandler( StreoMoodTags* parser /*= NULL*/ )
{
	m_parser = parser;
}

wxString LinkTagHandler::GetSupportedTags()
{
	return wxT("A");
}

bool LinkTagHandler::HandleTag( const wxHtmlTag& element )
{
	if(element.HasParam(wxT("HREF")) && element.HasParam(wxT("CLASS")) && element.GetParam(wxT("CLASS")).Contains(wxT("tag-type"))){
		wxString tag = m_parser->GetSource()->SubString(element.GetBeginPos(), element.GetEndPos1() - 1);
		wxString link = wxString::Format(wxT("http://stereomood.com%s/playlist.xspf"),element.GetParam(wxT("HREF")));

		m_parser->AddTagAndLink(tag, link);
		ParseInner(element);

		return true;
	}
	return false;
}

void StreoMoodTags::AddTagAndLink(wxString tag, wxString link)
{
	m_stereomoodLinksMap[tag] = link;
}

std::map<wxString, wxString>& StreoMoodTags::GetTagLinksMap()
{
	Parse(m_src);
	return m_stereomoodLinksMap;
}

StreoMoodTags::StreoMoodTags(wxString src) : m_src(src)
{
	AddTagHandler(new LinkTagHandler(this));
}