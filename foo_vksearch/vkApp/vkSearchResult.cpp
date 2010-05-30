#include "StdAfx.h"
#include "vkSearchResult.h"


long vkSearchResult::GetListLastSelectedItem( long geometry )
{
	long item = GetFirstSelected();

	if(wxNOT_FOUND != item && geometry != wxLIST_NEXT_ABOVE)
	{
		while(GetNextSelected(item) != -1){
			item = GetNextSelected(item);
		}
	}
	return item;

}

void vkSearchResult::DeselectAllItems()
{
	long item = GetFirstSelected();

	while(wxNOT_FOUND != item){
		Select(item, false);
		item = GetNextSelected(item);		
	}
}