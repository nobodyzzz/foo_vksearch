#include "StdAfx.h"
#include "vkSearchWindow.h"

vkSearchWindow::vkSearchWindow(const wxString& title, searchOptions options) : wxFrame(NULL, wxID_ANY, title){
	wxArrayString searchVariants;

	searchVariants.Add(wxT("artist"));
	searchVariants.Add(wxT("radio"));
	searchVariants.Add(wxT("tag"));
	searchVariants.Add(wxT("recommended"));
	searchVariants.Add(wxT("user"));
	searchVariants.Add(wxT("group"));
	searchVariants.Add(wxT("vk.com"));
	searchVariants.Add(wxT("stereomood.com"));
	m_tracks = NULL;
	m_popupMenu = NULL;
	m_apiId = wxString::FromAscii(options.api_id);
	m_secret = wxString::FromAscii(options.secret);
	m_viewerId = wxString::FromAscii(options.viewer_id);
	m_lastFmApiKey = wxT("15ce8760abe155ebab4a6365ec6c9915");
	m_lastFmSecret = wxT("06beee64c545f5fe65accf485492dfe9");
	m_lastFmSession = wxString::FromAscii(options.lastfm_session);
	m_radioMaxArtists = options.radioMaxArtists;
	m_radioMaxTracks = options.radioMaxTraks;
	m_closeAfterAdd = options.closeAfterAdd;
	m_addTracksFn = options.addTracksFn;
	m_saveLastFmSessionFn = options.saveLastFmSessionFn;
	m_searchThread = 0;
	m_noLog = new wxLogNull();
	m_vbox = new wxBoxSizer(wxVERTICAL);
	m_hbox1 = new wxBoxSizer(wxHORIZONTAL);
	m_hbox2 = new wxBoxSizer(wxHORIZONTAL);
	m_searchPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxTHICK_FRAME, wxT("search panel"));
	m_queryTextBox = new wxTextCtrl(m_searchPanel, QUERY_EDIT);
	m_searchButton = new wxButton(m_searchPanel, SEARCH_BUTTON, wxT("search"));
	m_searchResult = new vkSearchResult(m_searchPanel);	
	m_addAll = new wxButton(m_searchPanel, ADDALL_BUTTON, wxT("Add all"));
	m_addSelected = new wxButton(m_searchPanel, ADDSELECTED_BUTTON, wxT("Add selected"));
	m_deleteSelected = new wxButton(m_searchPanel, DELETESELECTED_BUTTON, wxT("Delete selected"));
	m_keepPrevious = new wxCheckBox(m_searchPanel, wxID_ANY, wxT("Keep previous search result"));
	m_searchVariants = new wxComboBox(m_searchPanel, wxID_ANY, wxT("artist"), wxDefaultPosition, wxDefaultSize, searchVariants, wxCB_DROPDOWN | wxCB_READONLY);
	m_stereomoodTag = new wxComboBox();

	m_searchResult->InsertColumn(0, wxT("Artist"));
	m_searchResult->InsertColumn(1, wxT("Title"));
	m_searchResult->InsertColumn(2, wxT("Duration"), wxLIST_FORMAT_RIGHT);

	m_stereomoodTag->Hide();

	m_hbox1->Add(m_searchVariants, 0);
	m_hbox1->Add(m_queryTextBox, 1, wxRIGHT | wxLEFT, 4);
	m_hbox1->Add(m_searchButton, 0);
	m_hbox2->Add(m_addAll, 1, wxRIGHT);
	m_hbox2->Add(m_addSelected, 1, wxRIGHT);
	m_hbox2->Add(m_deleteSelected, 1, wxRIGHT);	
	m_vbox->Add(m_hbox1, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);
	m_vbox->Add(m_keepPrevious, 0, wxLEFT, 10);
	m_vbox->Add(-1, 10);
	m_vbox->Add(m_searchResult, 5, wxEXPAND | wxALL);
	m_vbox->Add(m_hbox2, 0, wxEXPAND, 1);
	m_searchPanel->SetSizer(m_vbox);

	int width = GetSize().GetWidth();
	int point = width / 8;

	m_searchResult->SetColumnWidth(0, point*2);
	m_searchResult->SetColumnWidth(1, point*4 - 15);
	m_searchResult->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
	SetSize(384, 472);

	CreateStatusBar();
	SetStatusText(wxT("vk.com audio search"));

	Connect(SEARCH_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(vkSearchWindow::OnSearchButtonClick));
	Connect(ADDALL_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(vkSearchWindow::OnAddAllClick));
	Connect(ADDSELECTED_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(vkSearchWindow::OnAddSelectedClick));
	Connect(DELETESELECTED_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(vkSearchWindow::OnDeleteSelectedClick));
	Connect(wxEVT_CONTEXT_MENU, wxContextMenuEventHandler(vkSearchWindow::OnContextMenu));
	m_queryTextBox->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(vkSearchWindow::OnKeyDown));
	m_searchResult->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(vkSearchWindow::OnSearchResultKeyDown));
	m_searchResult->Connect(wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler(vkSearchWindow::OnSearchItemActivate));
	m_searchVariants->Connect(wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler(vkSearchWindow::OnSearchVariantChange));
	m_searchPanel->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(vkSearchWindow::OnEscKeyDown));
		
	wxSocketBase::Initialize();
}

vkSearchWindow::~vkSearchWindow(void)
{
	if(m_tracks){
		CleanUpSearchResult();
	}
	if(m_searchThread){
		TerminateThread(m_searchThread, 0);
	}
	wxSocketBase::Initialize();
	delete m_noLog;
}

wxString* vkSearchWindow::StringHash( const char* string )
{
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	BYTE rgbHash[MD5LEN] = {0};
	DWORD cbHash = MD5LEN;
	wxString* result = NULL;

	if(string){
		if(CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
			if(CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)){
				if(CryptHashData(hHash, (const BYTE*)string, strlen(string), 0))
				{
					if(CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)){
						result = new wxString();
						result->Printf(	wxT("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"), 
							rgbHash[ 0], rgbHash[1], rgbHash[ 2], rgbHash[ 3], rgbHash[ 4], rgbHash[ 5], rgbHash[ 6], 
							rgbHash[ 7], rgbHash[8], rgbHash[ 9], rgbHash[10], rgbHash[11], rgbHash[12], rgbHash[13], 
							rgbHash[14], rgbHash[15]);

					}
				}
				CryptDestroyHash(hHash);
			}
			CryptReleaseContext(hProv, 0);
		}
	}
	return result;
}
bool vkSearchWindow::BuildTrackList( char *queryString )
{
	bool success = false;

	if(queryString){
		wxURL url(wxString::FromAscii(queryString));

		if(url.IsOk()){
			wxInputStream* result = url.GetInputStream();

			if(result){
				try{
					wxXmlDocument xml(*result);

					if(xml.GetRoot()->GetName() != wxT("error")){
						wxXmlNode *node = xml.GetRoot()->GetChildren();

						while(node){
							if(node->GetName() == wxT("audio")){
								wxXmlNode *track = node->GetChildren();
								wxString artist; 
								wxString title; 
								wxString url; 
								long duration = 0;

								while(track){
									wxString name = track->GetName();
									if(name == wxT("artist")){
										artist = Unescape(track->GetChildren()->GetContent());
									} else if(name == wxT("title")){
										title = Unescape(track->GetChildren()->GetContent());
									} else if(name == wxT("url")){
										url = track->GetChildren()->GetContent();
									} else if(name == wxT("duration")){
										track->GetChildren()->GetContent().ToLong(&duration);
									}
									track = track->GetNext();
								}
								Audio* audio = new Audio();
								audio->artist = artist; audio->title = title; audio->url = url;  audio->duration = (int)duration;  m_tracks->push_back(audio);
							}
							node = node->GetNext();
						}
						success = true;
					} else {
						SetStatusText(wxT("vk.com request error: ") + xml.GetRoot()->GetChildren()->GetContent());
						success = false;
					}
				}catch(...){}
				wxDELETE(result);

			}
		}
	}

	return success;
}	

DWORD __stdcall vkSearchWindow::SearchThread(){
	wxString query = m_queryTextBox->GetValue();
	char *queryString = NULL;
	bool success = false;
	std::map<wxString, wxString> params;

	m_searchButton->SetLabel(wxT("stop"));
	SetStatusText(wxT("working..."));
	if(m_tracks && !m_keepPrevious->IsChecked()){
		CleanUpSearchResult();

	}
	if(!m_tracks){
		m_tracks = new std::list<Audio*>();
	}
	m_vkRequestError = false;
	if(m_searchVariants->GetValue() == wxT("artist")){
		success = LastFmArtistSearch(query);
	} else if(m_searchVariants->GetValue() == wxT("radio")){
		success = LastFmArtistRadio(query);
	} else if(m_searchVariants->GetValue() == wxT("tag")){
		success = LastFmTagSearch(query);
	} else if(m_searchVariants->GetValue() == wxT("recommended")){
		if(m_lastFmSession.Len() == 0){
			wxString token = GetLastFmToken()->c_str();
			LastFmRequestAuthorisation(token);
			wxMessageBox(wxT("Allow access to your last.fm profile and press ok"), wxT("vk.com audio search"));
			m_lastFmSession = GetAuthSession(token)->c_str();
			if(m_saveLastFmSessionFn){
				m_saveLastFmSessionFn(m_lastFmSession.ToAscii());
			}
		}
		success = LastFmGetRecomendations();
	} else if(m_searchVariants->GetValue() == wxT("user")){
		params[wxT("method")] = wxT("audio.get");
		params[wxT("uid")] = query;
		success = VkRequest(params);
	} else if(m_searchVariants->GetValue() == wxT("group")){
		params[wxT("method")] = wxT("audio.get");
		params[wxT("gid")] = query;
		success = VkRequest(params);
	} else if(m_searchVariants->GetValue() == wxT("vk.com")){
		params[wxT("count")] = wxT("200");
		params[wxT("method")] = wxT("audio.search");
		params[wxT("q")] = query;
		success = VkRequest(params);
	} else if(m_searchVariants->GetValue() == wxT("stereomood.com")){
		success = StereoMoodRequest(m_stereomoodTag->GetValue());
	}
	if(!m_vkRequestError && success){
		SetStatusText(wxT("done"));
	}
	m_searchButton->SetLabel(wxT("search"));
	m_searchThread = 0;
	return TRUE;
}
void vkSearchWindow::DoSearch()
{
	union {
		DWORD (__stdcall vkSearchWindow::*method)();
		DWORD (__stdcall *fn)(void*);
	} ptr = { &vkSearchWindow::SearchThread};

	m_searchThread = CreateThread(NULL, 0, ptr.fn, this, 0, NULL);
}
Audio* vkSearchWindow::VkSearchTrack(wxString artistName, wxString trackName){
	wxString query;
	char* queryString = NULL;
	Audio* audio = NULL;

	if(artistName.Len() && trackName.Len()){
		std::map<wxString, wxString> params;

		query.Printf(wxT("%s - %s"), artistName, trackName);
		params[wxT("count")] = wxT("200");
		params[wxT("method")] = wxT("audio.search");
		params[wxT("q")] = query;
		queryString = VkMethodUrl(params);
		if(queryString){
			wxURL url(wxString::FromAscii(queryString));

			if(url.IsOk()){
				wxInputStream* result = url.GetInputStream();

				if(result){
					try{
						wxXmlDocument xml;

						if( xml.Load(*result) && xml.GetRoot() && xml.GetRoot()->GetName() != wxT("error")){
							wxXmlNode *node = xml.GetRoot()->GetChildren();

							while(node){
								if(node->GetName() == wxT("audio")){
									wxXmlNode *track = node->GetChildren();
									wxString artist;
									wxString title;
									wxString url;
									long duration = 0;

									while(track){
										wxString name = track->GetName();
										if(track->GetChildren()){
											if(name == wxT("artist")){
												artist = Unescape(track->GetChildren()->GetContent());
											} else if(name == wxT("title")){
												title = Unescape(track->GetChildren()->GetContent());
											} else if(name == wxT("url")){
												url = track->GetChildren()->GetContent();
											} else if(name == wxT("duration")){
												track->GetChildren()->GetContent().ToLong(&duration);
											}
										}
										track = track->GetNext();
									}
									if(artist.IsSameAs(artistName, false) && title.IsSameAs(trackName, false)){
										audio = new Audio();

										audio->artist = artistName;
										audio->title = trackName;
										audio->url = url;
										audio->duration = (int)duration;
									}
								}
								node = node->GetNext();
							}
						} else {
							if(xml.GetRoot()){
								SetStatusText(wxT("vk.com request error: ") + xml.GetRoot()->GetChildren()->GetContent());
							} else {
								SetStatusText(wxT("vk.com request error"));
							}
							m_vkRequestError = true;
						}
					} catch(...){}

					wxDELETE(result);
				}
			}
		}
	}
	return audio;
}
void vkSearchWindow::AddTrackToResultList(Audio *track){
	wxString duration;
	long index = 0;
	wxMBConvUTF8 conv;
	wxListItem newTrack;

	newTrack.SetText(wxString(track->artist.c_str()));
	newTrack.SetId(m_searchResult->GetItemCount());
	newTrack.SetData((void*)track->url.c_str());
	duration.Printf(wxT("%2d:%02d"), track->duration / 60, track->duration % 60);						
	index = m_searchResult->InsertItem(newTrack);
	m_searchResult->SetItem(index, 1, wxString(track->title.c_str()));
	m_searchResult->SetItem(index, 2, duration);

}
bool vkSearchWindow::LastFmArtistSearch( wxString artist, int maxCount /*= 50*/ )
{
	wxString requestUrl;
	wxURI requestUri;
	std::auto_ptr<TCHAR> escapedArtist(new TCHAR[INTERNET_MAX_URL_LENGTH + 1]);
	DWORD chEscaped = 0;
	bool success = false;

	AtlEscapeUrl(artist.c_str(), escapedArtist.get(), &chEscaped, INTERNET_MAX_URL_LENGTH);
	requestUrl.Printf(wxT("http://ws.audioscrobbler.com/1.0/artist/%s/toptracks.xml"),  escapedArtist);
	requestUri.Create(requestUrl);
	wxURL url(requestUri);
	if(url.IsOk()){
		wxInputStream *toptracks = url.GetInputStream();
		int trackCount = 0;

		if(toptracks){
			wxXmlDocument xml;

			if( xml.Load(*toptracks) ){
				wxXmlNode *node = xml.GetRoot()->GetChildren();

				while(node && !m_vkRequestError){
					if(node->GetName() == wxT("track")){
						wxXmlNode *track = node->GetChildren();

						if(track->GetName() == wxT("name")){
							wxString name = track->GetChildren()->GetContent();
							Audio* track = VkSearchTrack(artist, name);

							if(track && std::find_if(m_tracks->begin(), m_tracks->end(), std::bind2nd(equal_tracks(), track)) == m_tracks->end()){
								m_tracks->push_back(track);
								AddTrackToResultList(track);
								trackCount++;
								if(trackCount == maxCount){
									break;
								}
							}
						}
					}
					wxMilliSleep(333);
					node = node->GetNext();
				}
				success = !m_vkRequestError;
			}
			wxDELETE(toptracks);
		} else {
			SetStatusText(wxT("last.fm request failed. try vk.com search"));
		}
	}
	return success;
}
bool vkSearchWindow::LastFmTagSearch( wxString tag )
{
	wxString requestUrl;
	wxURI requestUri;
	std::auto_ptr<TCHAR> escapedTag(new TCHAR[INTERNET_MAX_URL_LENGTH + 1]);
	DWORD chEscaped = 0;
	bool success = false;

	AtlEscapeUrl(tag.c_str(), escapedTag.get(), &chEscaped, INTERNET_MAX_URL_LENGTH);
	requestUrl.Printf(wxT("http://ws.audioscrobbler.com/1.0/tag/%s/toptracks.xml"), escapedTag);
	requestUri.Create(requestUrl);
	wxURL url(requestUri);
	if(url.IsOk()){
		wxInputStream *toptracks = url.GetInputStream();
		if(toptracks){
			wxXmlDocument xml;

			if( xml.Load(*toptracks) ){
				wxXmlNode *track = xml.GetRoot()->GetChildren();

				while(track && !m_vkRequestError){
					if(track->GetName() == wxT("track")){
						wxXmlNode *artist = track->GetChildren();
						wxString trackName = track->GetAttribute(wxT("name"), wxEmptyString);
						wxString artistName = artist->GetAttribute(wxT("name"), wxEmptyString);
						Audio* audio = VkSearchTrack(artistName, trackName);

						if(audio){						
							m_tracks->push_back(audio);
							AddTrackToResultList(audio);
						}

					}
					wxMilliSleep(333);
					track = track->GetNext();
				}
				success = !m_vkRequestError;
			}
			wxDELETE(toptracks);
			
		} else {
			SetStatusText(wxT("last.fm request failed..."));
		}
	}
	return success;
}
bool vkSearchWindow::LastFmArtistRadio( wxString artist )
{
	wxString requestUrl;
	wxURI requestUri;
	std::auto_ptr<TCHAR> escapedArtist(new TCHAR[INTERNET_MAX_URL_LENGTH + 1]);
	DWORD chEscaped = 0;
	bool success = false;

	LastFmArtistSearch(artist, m_radioMaxTracks);
	AtlEscapeUrl(artist.c_str(), escapedArtist.get(), &chEscaped, INTERNET_MAX_URL_LENGTH);
	requestUrl.Printf(wxT("http://ws.audioscrobbler.com/1.0/artist/%s/similar.xml"),  escapedArtist);
	requestUri.Create(requestUrl);
	wxURL url(requestUri);
	if(url.IsOk()){
		wxInputStream *similar = url.GetInputStream();
		int count = 0;

		if(similar){
			wxXmlDocument xml;
			
			if( xml.Load(*similar) ){
				wxXmlNode *node = xml.GetRoot()->GetChildren();

				while(node && !m_vkRequestError){
					if(node->GetName() == wxT("artist")){
						wxXmlNode *similarArtist = node->GetChildren();

						if(similarArtist->GetName() == wxT("name")){
							wxString name = similarArtist->GetChildren()->GetContent();

							LastFmArtistSearch(name, m_radioMaxTracks);
							count++;
							if(count == m_radioMaxArtists){
								break;
							}
						}
					}
					node = node->GetNext();
				}
				success = !m_vkRequestError;
			}
			wxDELETE(similar);
		} else {
			SetStatusText(wxT("last.fm request failed..."));
		}
	}
	return success;
}
void vkSearchWindow::DeleteSelectedTracks(){
	long item = -1;

	while((item = m_searchResult->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND)
		m_searchResult->DeleteItem(item--);
}
void vkSearchWindow::AddSelected(){
	if(m_addTracksFn){
		long item = -1;
		std::vector<std::string> tracks;

		while((item = m_searchResult->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND){
			wxString url((wxChar*)m_searchResult->GetItemData(item));
			tracks.push_back(std::string(url.ToAscii()));
		}
		m_addTracksFn(tracks);
		if(m_closeAfterAdd)
		{
			Close();
		}
	}
}
void vkSearchWindow::CopyUrlsToClipboard(){
	long itemIdx = -1;
	wxString urls;
	wxTextDataObject* url = new wxTextDataObject();

	while((itemIdx = m_searchResult->GetNextItem(itemIdx, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND){
		urls.Append((wxChar*)m_searchResult->GetItemData(itemIdx));
		urls.Append(wxT("\n"));
	}
	urls = urls.Mid(0, urls.Len() - 1);
	if(wxTheClipboard->Open()){
		url->SetText(urls);
		wxTheClipboard->SetData(url);
		wxTheClipboard->Close();
	}
}
void vkSearchWindow::CleanUpSearchResult()
{
	m_searchResult->DeleteAllItems();
	std::for_each(m_tracks->begin(), m_tracks->end(), delete_object());
	delete m_tracks;				
	m_tracks = NULL;
}
wxString vkSearchWindow::Unescape( wxString string )
{
	string.Replace(wxT("&lt;"), wxT("<"));
	string.Replace(wxT("&gt;"), wxT(">"));
	string.Replace(wxT("&apos;"), wxT("'"));
	string.Replace(wxT("&quot;"), wxT("\""));
	string.Replace(wxT("&amp;"), wxT("&"));
	string.Replace(wxT("&#39;"), wxT("'"));
	string.Replace(wxT("&#62;"), wxT(">"));
	return string;
}

wxString* vkSearchWindow::LastFmCallSig( std::map<wxString, wxString> params )
{
	wxString* result = NULL;
	if(!params.empty()){
		wxString signature = wxT("api_key") + m_lastFmApiKey;
		std::map<wxString, wxString>::iterator it = params.begin();

		while(it != params.end()){
			signature += it->first + it->second; 
			it++;
		}
		signature += m_lastFmSecret;
		result = StringHash(signature.ToUTF8().data());
	}
	return result;
}

wxString* vkSearchWindow::GetLastFmToken()
{
	std::map<wxString, wxString> params;
	wxString* api_sig = NULL;
	wxString* result = NULL;
	wxString* methodUrl = NULL;

	params[wxT("method")] = wxT("auth.getToken");
	methodUrl = BuildLastFmMethodUrl(params);
	if(methodUrl){		
		wxURL url(methodUrl->c_str());

		if(url.IsOk()){
			wxInputStream *response = url.GetInputStream();

			if(response){
				wxXmlDocument token;

				if(token.Load(*response)){
					wxXmlNode* node = token.GetRoot()->GetChildren();

					if(node->GetName() == wxT("token")){
						result = new wxString(node->GetChildren()->GetContent());
					}
				}
				wxDELETE(response);
			}
		}		
	}
	return result;
}

wxString* vkSearchWindow::GetAuthSession(wxString token)
{
	wxString* result = NULL;

	if(token.Len()){
		std::map<wxString, wxString> params;
		wxString* methodUrl;

		params[wxT("method")] = wxT("auth.getSession");
		params[wxT("token")] = token;
		methodUrl = BuildLastFmMethodUrl(params);
		if(methodUrl){
			wxURL url(methodUrl->c_str());

			if(url.IsOk()){
				wxInputStream *response = url.GetInputStream();

				if(response){
					wxXmlDocument session;

					if(session.Load(*response)){
						wxXmlNode* node = session.GetRoot()->GetChildren();

						if(node->GetName() == wxT("session")){
							node = node->GetChildren();
							while(node){
								if(node->GetName() == wxT("key")){
									result = new wxString(node->GetChildren()->GetContent());
									break;
								}
								node = node->GetNext();
							}
						}
					}
					wxDELETE(response);
				}
			}			
		}		
	}
	return result;
}

void vkSearchWindow::LastFmRequestAuthorisation( wxString token )
{
	wxString authUrl;

	authUrl.Printf(wxT("http://www.last.fm/api/auth/?api_key=%s&token=%s"), m_lastFmApiKey, token);
	wxLaunchDefaultBrowser(authUrl);
}

bool vkSearchWindow::LastFmGetRecomendations(){
	bool result = false; 
	if(m_lastFmSession.Len() != 0){
		std::map<wxString, wxString> params;
		wxString* methodUrl = NULL; 
		
		params[wxT("method")] = wxT("user.getRecommendedArtists"); 
		params[wxT("sk")] = m_lastFmSession;
		methodUrl = BuildLastFmMethodUrl(params);
		if(methodUrl){
			wxURL url(methodUrl->c_str());
			if(url.IsOk()){
				wxInputStream *response = url.GetInputStream();

				if(response){
					wxXmlDocument recomendations;

					if(recomendations.Load(*response)){
						wxXmlNode *node = recomendations.GetRoot()->GetChildren()->GetChildren();

						while (node && !m_vkRequestError)
						{
							if(node->GetName() == wxT("artist")){
								wxXmlNode *artist = node->GetChildren();
								if(artist->GetName() == wxT("name")){
									wxString name = artist->GetChildren()->GetContent();

									LastFmArtistSearch(name, m_radioMaxTracks);
								}
							}
							node = node->GetNext();
						}
					} 
					result = !m_vkRequestError; 
					wxDELETE(response);
				} else {
					SetStatusText(wxT("last.fm request failed..."));
				}
			}
		}
	}
	return result;
}



wxString* vkSearchWindow::BuildLastFmMethodUrl( std::map<wxString, wxString> params )
{
	wxString* result = NULL;
	wxString* api_sig = LastFmCallSig(params);
	if(!params.empty() && api_sig){
		result = new wxString(wxT("http://ws.audioscrobbler.com/2.0/?"));
		std::map<wxString, wxString>::iterator it;

		it = params.begin();
		while(it != params.end()){
			result->Append(wxString::Format(wxT("%s=%s&"), it->first, it->second));
			it++;
		}
		result->Append(wxString::Format(wxT("api_key=%s&api_sig=%s"), m_lastFmApiKey, api_sig->c_str()));
		wxDELETE(api_sig);
	}
	return result;
}
wxString* vkSearchWindow::VkCallSig( std::map<wxString, wxString> params )
{
	wxString* result = NULL; 
	if(!params.empty()){
		char signature[INTERNET_MAX_URL_LENGTH + 1] = {0};
		char paramPair[INTERNET_MAX_URL_LENGTH + 1] = {0};
		std::map<wxString, wxString>::iterator it = params.begin();

		strcat_s(signature, INTERNET_MAX_URL_LENGTH, m_viewerId.ToUTF8().data());
		sprintf_s(paramPair, INTERNET_MAX_URL_LENGTH,"api_id=%s", m_apiId.ToUTF8().data());
		strcat_s(signature, INTERNET_MAX_URL_LENGTH, paramPair);
		params[wxT("test_mode")] = wxT("1");
		while(it != params.end()){
			sprintf_s(paramPair, INTERNET_MAX_URL_LENGTH, "%s=%s", it->first.ToUTF8().data(), it->second.ToUTF8().data());
			strcat_s(signature, INTERNET_MAX_URL_LENGTH, paramPair);
			it++;
		}
		strcat_s(signature, INTERNET_MAX_URL_LENGTH, m_secret.ToUTF8().data());
		result = StringHash(signature);
	}
	return result;
}

char* vkSearchWindow::VkMethodUrl( std::map<wxString, wxString> params )
{
	char* result = NULL;

	if(!params.empty()){
		wxString *sig = VkCallSig(params);
		if(sig){
			char paramPair[INTERNET_MAX_URL_LENGTH + 1] = {0};
			std::map<wxString, wxString>::iterator it = params.begin();

			result = new char[INTERNET_MAX_URL_LENGTH + 1];
			sprintf_s(result, INTERNET_MAX_URL_LENGTH, "http://api.vkontakte.ru/api.php?api_id=%s", m_apiId.ToUTF8().data());
			if(params.find(wxT("q")) != params.end()){
				//escape reserved url characters
				params[wxT("q")].Replace(wxT("&"), wxT("%26"));
				params[wxT("q")].Replace(wxT("+"), wxT("%2B"));
				params[wxT("q")].Replace(wxT("$"), wxT("%24"));
				params[wxT("q")].Replace(wxT(","), wxT("%2C"));
				params[wxT("q")].Replace(wxT("/"), wxT("%2F"));
				params[wxT("q")].Replace(wxT(":"), wxT("%3A"));
				params[wxT("q")].Replace(wxT(";"), wxT("%3B"));
				params[wxT("q")].Replace(wxT("="), wxT("%3D"));
				params[wxT("q")].Replace(wxT("?"), wxT("%3F"));
				params[wxT("q")].Replace(wxT("@"), wxT("%40"));
				params[wxT("q")].Replace(wxT("#"), wxT("%23"));
			}
			params[wxT("sig")] = sig->c_str();
			params[wxT("test_mode")] = wxT("1");
			while(it != params.end()){
				sprintf_s(paramPair, INTERNET_MAX_URL_LENGTH, "&%s=%s", it->first.ToUTF8().data(), it->second.ToUTF8().data());
				strcat_s(result, INTERNET_MAX_URL_LENGTH, paramPair);
				it++;
			}
			wxDELETE(sig);
		}
	}
	return result;
}
void vkSearchWindow::OnSearchButtonClick(wxCommandEvent& evt){
	if(!m_searchThread){
		DoSearch();
	} else {
		m_searchButton->SetLabel(wxT("search"));
		TerminateThread(m_searchThread, 0);
		m_searchThread = 0;
		SetStatusText(wxT("stopped..."));
	}
}
void vkSearchWindow::OnAddAllClick(wxCommandEvent& evt){
	if(m_addTracksFn){
		std::vector<std::string> tracks;
		for(long i = 0; i < m_searchResult->GetItemCount(); i++){
			wxString url((wxChar*)m_searchResult->GetItemData(i));
			tracks.push_back(std::string(url.ToAscii()));
		}
		m_addTracksFn(tracks);
		if(m_closeAfterAdd)
		{
			Close();
		}
	}
}
void vkSearchWindow::OnAddSelectedClick(wxCommandEvent& evt){
	AddSelected();
}	
void vkSearchWindow::OnDeleteSelectedClick(wxCommandEvent& evt){
	DeleteSelectedTracks();
}
void vkSearchWindow::OnPopupClick(wxCommandEvent& evt){
	switch(evt.GetId()){
			case COPY_URL_MENU:
				CopyUrlsToClipboard();
				break;
			case ADDSELECTED_MENU:
				AddSelected();
				break;
			case DELETESELECTED_MENU:
				DeleteSelectedTracks();
				break;
	}
}
void vkSearchWindow::OnContextMenu(wxContextMenuEvent& evt){		
	if(!m_popupMenu){
		m_popupMenu = new wxMenu();

		m_popupMenu->Append(ADDSELECTED_MENU, wxT("Add selected tracks to playlist"));
		m_popupMenu->Append(DELETESELECTED_MENU, wxT("Delete selected tracks"));
		m_popupMenu->Append(COPY_URL_MENU, wxT("Copy url(s) to clipboard"));
		Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(vkSearchWindow::OnPopupClick));
	}
	if(m_searchResult->IsMouseInWindow() && m_searchResult->GetItemCount() != 0){
		PopupMenu(m_popupMenu);
	}
}
void vkSearchWindow::OnKeyDown(wxKeyEvent& event){
	if(event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER){
		vkSearchWindow* _this = (vkSearchWindow*)GetParent()->GetParent();
		_this->DoSearch();			
	} else {
		event.Skip();
	}
}
void vkSearchWindow::OnEscKeyDown( wxKeyEvent& event )
{
	if(event.GetKeyCode() == WXK_ESCAPE){
		Close();
	}
}
void vkSearchWindow::OnSearchResultKeyDown( wxKeyEvent& evt )
{
	vkSearchWindow* _this = (vkSearchWindow*)GetParent()->GetParent();
	vkSearchResult* list = _this->m_searchResult;
	int keyCode = evt.GetKeyCode();
	long item = -1;
	long geometry = 0;

	switch(keyCode)
	{
	case 'A':
		if(evt.ControlDown()){
			list->SelectAll();
		}
		break;
	case 'C':
		if(evt.ControlDown()){
			_this->CopyUrlsToClipboard();
		}
		break;
	case WXK_DELETE:
		_this->DeleteSelectedTracks();	
		break;
	case WXK_RETURN:
	case WXK_NUMPAD_ENTER:
		_this->AddSelected();
		break;
	case WXK_UP:
	case WXK_DOWN:		
		geometry = keyCode == WXK_DOWN ? wxLIST_NEXT_BELOW : wxLIST_NEXT_ABOVE;	
		item = list->GetFocusedItem();

		if(wxNOT_FOUND == item){
			item = list->GetListLastSelectedItem(geometry);
		}
		if(wxNOT_FOUND != item){
			if(evt.ControlDown())
			{		
				item = list->GetNextItem(item, geometry);
				list->Focus(item);			
			} else {
				if(!evt.ShiftDown()){
					list->DeselectAll();
				}
				item = list->GetNextItem(item, geometry);
				list->SetItemState(item, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
				list->EnsureVisible(item);
			}
		}
		break;
	case WXK_SPACE:
		item = list->GetFocusedItem();
		if(wxNOT_FOUND != item){
			list->Select(item, !list->IsSelected(item));
		}
		break;
	}
}

void vkSearchWindow::OnSearchItemActivate( wxListEvent& evt )
{
	vkSearchWindow* _this = (vkSearchWindow*)GetParent()->GetParent();
	_this->AddSelected();
}

void vkSearchWindow::OnSearchVariantChange( wxCommandEvent& evt )
{
	vkSearchWindow* _this = (vkSearchWindow*)GetParent()->GetParent();

	_this->m_stereomoodTag->Hide();
	_this->m_queryTextBox->Show();
	_this->m_searchVariants->SetSize(-1, -1, wxDefaultCoord, wxDefaultCoord, wxSIZE_AUTO);

	if(_this->m_searchVariants->GetValue() == wxT("recommended")){
		int width = _this->m_searchVariants->GetSize().GetWidth() + _this->m_queryTextBox->GetSize().GetWidth() + 4;
		_this->m_queryTextBox->Hide();
		_this->m_searchVariants->SetSize(width, wxDefaultCoord);
	} else if(_this->m_searchVariants->GetValue() == wxT("stereomood.com")) {
		if(_this->m_stereomoodTags.Count() == 0){
			union {
				DWORD (__stdcall vkSearchWindow::*method)();
				DWORD (__stdcall *fn)(void*);
			} ptr = { &vkSearchWindow::FillStereomoodTagsThread};
			wxArrayString choises;

			choises.Add(wxT("Loading..."));
			_this->m_stereomoodTag->Create(	_this->m_searchPanel,
											wxID_ANY,
											choises[0],
											_this->m_queryTextBox->GetPosition(),
											_this->m_queryTextBox->GetSize(),
											choises);
			_this->m_hbox1->Insert(1, _this->m_stereomoodTag);
			CreateThread(NULL, 0, ptr.fn, _this, 0, NULL);			
		} 
		_this->m_queryTextBox->Hide();
		_this->m_stereomoodTag->Show();
		
	}


}

bool vkSearchWindow::VkRequest( std::map<wxString, wxString> params )
{
	char * queryString = VkMethodUrl(params);
	bool success = false;

	if(queryString){
		if(BuildTrackList(queryString)){
			m_tracks->sort(compare_tracks());
			m_tracks->unique(equal_tracks());
			std::list<Audio*>::iterator track = m_tracks->begin();

			while(track != m_tracks->end()){
				AddTrackToResultList(*track);
				track++;
			}
			success = true;
		}
	}	
	return success;
}

void vkSearchWindow::GetStereomoodTags()
{
	StreoMoodTags main(ReadUrlToString(wxT("http://stereomood.com/")));
	StreoMoodTags more(ReadUrlToString(wxT("http://stereomood.com/tools/ajax_more_tags.php")));
	std::map<wxString, wxString>& mainTags = main.GetTagLinksMap();
	std::map<wxString, wxString>& moreTags = more.GetTagLinksMap();

	m_stereomoodLinksMap.insert(mainTags.begin(), mainTags.end());
	m_stereomoodLinksMap.insert(moreTags.begin(), moreTags.end());
	std::for_each(mainTags.begin(), mainTags.end(), AddToWxStringArray(m_stereomoodTags));
	std::for_each(moreTags.begin(), moreTags.end(), AddToWxStringArray(m_stereomoodTags));
	return;
}

bool vkSearchWindow::StereoMoodRequest( wxString tag )
{
	bool success = false;
	wxURL url(m_stereomoodLinksMap[tag]);

	if(url.IsOk()){
		wxInputStream* result = url.GetInputStream();

		if(result){
			try{
				wxXmlDocument xml(*result);
				wxXmlNode *node = xml.GetRoot()->GetChildren();
				long total = 0;
				long indexes = 0;

				if(node->GetName() == wxT("tracksTotal")){
					node->GetChildren()->GetContent().ToLong(&total);
					indexes = total / 20;
					indexes += (total % 20) ? 1 : 0;
					for(long i = 0; i < indexes; i++){
						StereoMoodGetTracks(url.GetURL(), i + 1);
					}					
				}
				success = true;
			} catch(...){}
		}
	}

	return success;
}

void vkSearchWindow::StereoMoodGetTracks( wxString url, long index)
{
	wxURL tagurl(url + wxString::Format(wxT("?index=%d"), index));

	if(tagurl.IsOk()){
		wxInputStream	*xspfStream = tagurl.GetInputStream();

		if(xspfStream){
			wxXmlDocument xml(*xspfStream);
			wxXmlNode *node = xml.GetRoot()->GetChildren();

			while (node)
			{
				if(node->GetName() == wxT("trackList")){
					wxXmlNode *tracks = node->GetChildren();

					while(tracks){
						if(tracks->GetName() == wxT("track")){
							wxXmlNode *track = tracks->GetChildren();
							wxString artist; 
							wxString title; 
							wxString url; 

							while(track){
								wxString name = track->GetName();
								if(name == wxT("creator")){
									artist = Unescape(track->GetChildren()->GetContent());
								} else if(name == wxT("title")){
									title = Unescape(track->GetChildren()->GetContent());
								} else if(name == wxT("location")){
									url = track->GetChildren()->GetContent();
								}
								track = track->GetNext();
							}
							Audio* audio = new Audio();
							audio->artist = artist; 
							audio->title = title;
							audio->url = url;  
							audio->duration = 0;  
							m_tracks->push_back(audio);
							AddTrackToResultList(audio);
						}
						tracks = tracks->GetNext();
					}
				}
				node = node->GetNext();
			}
		}
		wxDELETE(xspfStream);
	}
	
}

DWORD __stdcall vkSearchWindow::FillStereomoodTagsThread()
{
	m_stereomoodTag->Disable();
	GetStereomoodTags();
	m_stereomoodTag->Clear();
	m_stereomoodTag->Append(m_stereomoodTags);
	m_stereomoodTag->SetValue(m_stereomoodTags[0]);
	m_stereomoodTag->Enable();
	return TRUE;
}

wxString vkSearchWindow::ReadUrlToString( wxString address )
{
	wxURL url(address);
	wxString str;

	if(url.IsOk()){
		wxInputStream *stream = url.GetInputStream();
		char* data = new char[0x1000];

		while(!stream->Eof()){
			stream->Read(data, 0x1000);
			str += wxString::FromUTF8(data, 0x1000);
		}
		delete[] data;
		wxDELETE(stream);
	}
	return str;
}