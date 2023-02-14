#include "stdafx.h"
#include "resource.h"
#include "guids_discogger.h"

#include "tasks.h"
#include "find_release_dialog.h"
#include "tag_mappings_dialog.h"
#include "configuration_dialog.h"
#include "tags.h"

static const GUID contextmenu_group_discogs_id = { 0x73505390, 0x5ff3, 0x4ca0, { 0x91, 0x1e, 0xc1, 0xf9, 0xfd, 0xaa, 0x88, 0xed } }; //mod
static service_factory_single_t<contextmenu_group_popup_impl> mygroup(contextmenu_group_discogs_id, contextmenu_groups::tagging, "Discogger", 0);
static const GUID contextmenu_group_discogs_web_id = { 0x51d6ec22, 0x3bed, 0x46de, { 0x94, 0x71, 0xeb, 0xc8, 0x98, 0xed, 0x75, 0x62 } };
static service_factory_single_t<contextmenu_group_popup_impl> mysep1(contextmenu_group_discogs_web_id, contextmenu_group_discogs_id, "Web", 0);
static const GUID contextmenu_group_discogs_utilities_id = { 0x2fa5ff96, 0xf204, 0x4922, { 0xa6, 0x28, 0x40, 0x1d, 0xf8, 0xe0, 0xe8, 0xef } }; //mod
static service_factory_single_t<contextmenu_group_popup_impl> mygroup2(contextmenu_group_discogs_utilities_id, contextmenu_group_discogs_id, "Utilities", 0);

// {9A2A116F-BEAA-43FA-889C-E3FD777BEDE1}
static const GUID guid_Configuration =
{ 0x9a2a116f, 0xbeaa, 0x43fa, { 0x88, 0x9c, 0xe3, 0xfd, 0x77, 0x7b, 0xed, 0xe1 } };
// {87014504-4FC5-4520-83CC-49663675FA51}
static const GUID guid_WriteTags =
{ 0x87014504, 0x4fc5, 0x4520, { 0x83, 0xcc, 0x49, 0x66, 0x36, 0x75, 0xfa, 0x51 } };
// {ADA08D51-4013-4127-8970-13D4B59BACFE}
static const GUID guid_WriteTagsAlt =
{ 0xada08d51, 0x4013, 0x4127, { 0x89, 0x70, 0x13, 0xd4, 0xb5, 0x9b, 0xac, 0xfe } };
// {D91BEF05-460B-43BB-A1CC-66BDD55DC177}
static const GUID guid_DisplayReleasePage =
{ 0xd91bef05, 0x460b, 0x43bb, { 0xa1, 0xcc, 0x66, 0xbd, 0xd5, 0x5d, 0xc1, 0x77 } };
// {422E9B29-8713-4334-99F3-5A8832B03DC2}
static const GUID guid_DisplayMasterReleasePage =
{ 0x422e9b29, 0x8713, 0x4334, { 0x99, 0xf3, 0x5a, 0x88, 0x32, 0xb0, 0x3d, 0xc2 } };
// {97328E47-ACB4-4669-8370-D410415A6856}
static const GUID guid_DisplayArtistPage =
{ 0x97328e47, 0xacb4, 0x4669, { 0x83, 0x70, 0xd4, 0x10, 0x41, 0x5a, 0x68, 0x56 } };
// {A2CA897B-CD8D-4048-AF59-DE9E2B790785}
static const GUID guid_DisplayArtistArtPage =
{ 0xa2ca897b, 0xcd8d, 0x4048, { 0xaf, 0x59, 0xde, 0x9e, 0x2b, 0x79, 0x7, 0x85 } };
// {71F1E9A5-9982-46C8-B429-4B920D19ED39}
static const GUID guid_DisplayLabelPage =
{ 0x71f1e9a5, 0x9982, 0x46c8, { 0xb4, 0x29, 0x4b, 0x92, 0xd, 0x19, 0xed, 0x39 } };
// {E6BCFA27-2CD1-4960-AE7D-934231940231}
static const GUID guid_DisplayAlbumArtPage =
{ 0xe6bcfa27, 0x2cd1, 0x4960, { 0xae, 0x7d, 0x93, 0x42, 0x31, 0x94, 0x2, 0x31 } };
// {18798216-62E4-46A0-964D-2C6B65D4BC3A}
static const GUID guid_EditTagMappings =
{ 0x18798216, 0x62e4, 0x46a0, { 0x96, 0x4d, 0x2c, 0x6b, 0x65, 0xd4, 0xbc, 0x3a } };
// {B9391FF5-9C91-4FF2-8001-0DAFDF972236}
static const GUID guid_FindDeletedReleases =
{ 0xb9391ff5, 0x9c91, 0x4ff2, { 0x80, 0x1, 0xd, 0xaf, 0xdf, 0x97, 0x22, 0x36 } };
// {4197518E-8CDF-49E9-BBFC-F96C8B216423}
static const GUID guid_FindReleasesNotInCollection =
{ 0x4197518e, 0x8cdf, 0x49e9, { 0xbb, 0xfc, 0xf9, 0x6c, 0x8b, 0x21, 0x64, 0x23 } };


class contextmenu_discogs_web : public contextmenu_item_simple {

private:
	enum MenuIndex
	{
		DisplayReleasePage,
		DisplayMasterReleasePage,
		DisplayArtistPage,
		DisplayArtistArtPage,
		DisplayLabelPage,
		DisplayAlbumArtPage,
	};

public:
	GUID get_parent() override {
		return contextmenu_group_discogs_web_id;
	}

	unsigned get_num_items() override {
		return 6;
	}

	void get_item_name(unsigned p_index, pfc::string_base& p_out) override {
		switch (p_index) {
		case DisplayReleasePage:
			p_out = "Web release page";
			break;
		case DisplayMasterReleasePage:
			p_out = "Web master release page";
			break;
		case DisplayArtistPage:
			p_out = "Web artist page";
			break;
		case DisplayArtistArtPage:
			p_out = "Web artist artwork page";
			break;
		case DisplayLabelPage:
			p_out = "Web label page";
			break;
		case DisplayAlbumArtPage:
			p_out = "Web album artwork page";
			break;
		}
		return;
	}

	bool context_get_display(unsigned p_index, metadb_handle_list_cref p_data, pfc::string_base& p_out, unsigned& p_displayflags, const GUID& p_caller) override {
		PFC_ASSERT(p_index >= 0 && p_index < get_num_items());
		get_item_name(p_index, p_out);

		if (!g_discogs) {
			//exit
			p_displayflags = FLAG_GRAYED;
			return true;
		}

		switch (p_index) {
		case DisplayReleasePage:
			p_displayflags = g_discogs->item_has_tag(p_data.get_item(0), TAG_RELEASE_ID) ? 0 : FLAG_GRAYED;
			break;
		case DisplayMasterReleasePage:
			p_displayflags = (g_discogs->item_has_tag(p_data.get_item(0), TAG_MASTER_RELEASE_ID) || g_discogs->item_has_tag(p_data.get_item(0), TAG_RELEASE_ID)) ? 0 : FLAG_GRAYED;
			break;
		case DisplayArtistPage:
			p_displayflags = g_discogs->item_has_tag(p_data.get_item(0), TAG_ARTIST_ID, "DISCOGS_ARTIST_LINK") ? 0 : FLAG_GRAYED;
			break;
		case DisplayArtistArtPage:
			p_displayflags = g_discogs->item_has_tag(p_data.get_item(0), TAG_ARTIST_ID, "DISCOGS_ARTIST_LINK") ? 0 : FLAG_GRAYED;
			break;
		case DisplayLabelPage:
			p_displayflags = g_discogs->item_has_tag(p_data.get_item(0), TAG_LABEL_ID, "DISCOGS_LABEL_LINK") ? 0 : FLAG_GRAYED;
			break;
		case DisplayAlbumArtPage:
			p_displayflags = g_discogs->item_has_tag(p_data.get_item(0), TAG_RELEASE_ID) ? 0 : FLAG_GRAYED;
			break;
		}
		return true;
	}

	GUID get_item_guid(unsigned p_index) override {

		switch (p_index) {
		case DisplayReleasePage:
			return guid_DisplayReleasePage;
		case DisplayMasterReleasePage:
			return guid_DisplayMasterReleasePage;
		case DisplayArtistPage:
			return guid_DisplayArtistPage;
		case DisplayArtistArtPage:
			return guid_DisplayArtistArtPage;
		case DisplayLabelPage:
			return guid_DisplayLabelPage;
		case DisplayAlbumArtPage:
			return guid_DisplayAlbumArtPage;
		}
		return GUID_NULL;
	}

	bool get_item_description(unsigned p_index, pfc::string_base& p_out) override {
		switch (p_index) {
		case DisplayReleasePage:
			p_out = "Web release page";
			break;
		case DisplayMasterReleasePage:
			p_out = "Web master release page";
			break;
		case DisplayArtistPage:
			p_out = "Web artist page";
			break;
		case DisplayArtistArtPage:
			p_out = "Web artist artwork page";
			break;
		case DisplayLabelPage:
			p_out = "Web label page";
			break;
		case DisplayAlbumArtPage:
			p_out = "Web album artwork page";
			break;
		}
		return true;
	}

	void context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller) override {
		unsigned displayflags;
		pfc::string8 out;
		context_get_display(p_index, p_data, out, displayflags, p_caller);

		if (displayflags & FLAG_GRAYED) {
			//exit
			return;
		}

		switch (p_index) {
		case DisplayReleasePage:
			g_discogs->item_display_web_page(p_data.get_item(0), foo_discogs::RELEASE_PAGE);
			break;

		case DisplayMasterReleasePage:
			g_discogs->item_display_web_page(p_data.get_item(0), foo_discogs::MASTER_RELEASE_PAGE);
			break;

		case DisplayArtistPage:
			g_discogs->item_display_web_page(p_data.get_item(0), foo_discogs::ARTIST_PAGE);
			break;

		case DisplayArtistArtPage:
			g_discogs->item_display_web_page(p_data.get_item(0), foo_discogs::ARTIST_ART_PAGE);
			break;

		case DisplayLabelPage:
			g_discogs->item_display_web_page(p_data.get_item(0), foo_discogs::LABEL_PAGE);
			break;

		case DisplayAlbumArtPage:
			g_discogs->item_display_web_page(p_data.get_item(0), foo_discogs::ALBUM_ART_PAGE);
			break;
		}
	}
};


class contextmenu_discogs : public contextmenu_item_simple
{
private:
	enum MenuIndex
	{
		WriteTags,
		WriteTagsAlt,
		EditTagMappings,
		Configuration,
	};

public:

	GUID get_parent() override {
		return contextmenu_group_discogs_id;
	}

	unsigned get_num_items() override {
		return 4;
	}

	void get_item_name(unsigned p_index, pfc::string_base& p_out) override {
		switch (p_index) {
		case WriteTags:
			p_out = "Write tags...";
			break;
		case WriteTagsAlt:
			p_out = "Plain write tags ...";
			break;
		case EditTagMappings:
			p_out = "Edit Tag Mappings...";
			break;
		case Configuration:
			p_out = "Configuration...";
			break;
		}
	}

	void context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller) override {
		unsigned displayflags;
		pfc::string8 out;
		context_get_display(p_index, p_data, out, displayflags, p_caller);
		
		if (displayflags & FLAG_GRAYED) {
			//exit
			return;
		}

		switch (p_index) {
		case WriteTags:
		case WriteTagsAlt:
			//lo former, hi current
			CONF.mode_write_alt = MAKELPARAM(HIWORD(CONF.mode_write_alt), p_index == WriteTagsAlt);
			g_discogs->find_release_dialog = fb2k::newDialog<CFindReleaseDialog>(core_api::get_main_window(), p_data, CONF);
			if (g_discogs->tag_mappings_dialog) {
				if (CONF.awt_mode_changing()) {
					if (g_discogs->tag_mappings_dialog) {
						CTagMappingDialog* dlg = g_discogs->tag_mappings_dialog;
						dlg->UpdateAltMode(true);						
					}
				}
			}
			break;

		case EditTagMappings:
			if (!g_discogs->tag_mappings_dialog) {
				g_discogs->tag_mappings_dialog = fb2k::newDialog<CTagMappingDialog>(core_api::get_main_window());
			}
			else {
				::SetFocus(g_discogs->tag_mappings_dialog->m_hWnd);
			}
			break;

		case Configuration:
			static_api_ptr_t<ui_control>()->show_preferences(guid_pref_page);
			break;
		}
	}

	bool context_get_display(unsigned p_index, metadb_handle_list_cref p_data, pfc::string_base& p_out, unsigned& p_displayflags, const GUID& p_caller) override {
		PFC_ASSERT(p_index >= 0 && p_index < get_num_items());
		get_item_name(p_index, p_out);

		if (!g_discogs) {
			//exit
			p_displayflags = FLAG_GRAYED;
			return true;
		}

		switch (p_index) {
		case WriteTags:
		case WriteTagsAlt:
			p_displayflags =
				!g_discogs->locked_operation &&
				!g_discogs->find_release_dialog &&
				!g_discogs->track_matching_dialog &&
				!g_discogs->preview_tags_dialog ? 0 : FLAG_GRAYED;
			break;
		case EditTagMappings:
			p_displayflags = g_discogs->tag_mappings_dialog ? FLAG_GRAYED : 0;
			break;
		case Configuration:
			p_displayflags = /*g_discogs->configuration_dialog ? FLAG_GRAYED :*/ 0;
			break;
		}
		return true;
	}

	GUID get_item_guid(unsigned p_index) override {

		switch (p_index) {
		case WriteTags:
			return guid_WriteTags;
		case WriteTagsAlt:
			return guid_WriteTagsAlt;
		case EditTagMappings:
			return guid_EditTagMappings;
		case Configuration:
			return guid_Configuration;
		}
		return GUID_NULL;
	}

	bool get_item_description(unsigned p_index, pfc::string_base& p_out) override {
		switch (p_index) {
		case WriteTags:
			p_out = "Write tags";
			break;
		case WriteTagsAlt:
			p_out = "Plain write tags";
			break;
		case EditTagMappings:
			p_out = "Edit tag mappings";
			break;
		case Configuration:
			p_out = "Configuration";
			break;
		}
		return true;
	}
};


class contextmenu_discogs_utilities : public contextmenu_item_simple
{
private:
	enum MenuIndex
	{
		FindDeletedReleases,
		FindReleasesNotInCollect,
	};

public:
	GUID get_parent() override {
		return contextmenu_group_discogs_utilities_id;
	}

	unsigned get_num_items() override {
		return 2;
	}

	void get_item_name(unsigned p_index, pfc::string_base& p_out) override {
		switch (p_index) {
		case FindDeletedReleases:
			p_out = "Find Deleted Releases...";
			break;
		case FindReleasesNotInCollect:
			p_out = "Find Releases not in Collection...";
			break;
		}
	}

	void get_item_default_path(unsigned p_index, pfc::string_base& p_out) override {
		p_out = "Utilities";
	}

	void context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller) override {
		unsigned displayflags;
		pfc::string8 out;
		context_get_display(p_index, p_data, out, displayflags, p_caller);
		if (displayflags & FLAG_GRAYED) {
			return;
		}

		switch (p_index) {
		case FindDeletedReleases: {
			try {
				service_ptr_t<find_deleted_releases_task> task = new service_impl_t<find_deleted_releases_task>(p_data);
				task->start();
			}
			catch (locked_task_exception e)
			{
				log_msg(e.what());
			}
		}
		break;

		case FindReleasesNotInCollect: {
			try {
				service_ptr_t<find_releases_not_in_collection_task> task = new service_impl_t<find_releases_not_in_collection_task>(p_data);
				task->start();
			}
			catch (locked_task_exception e)
			{
				log_msg(e.what());
			}
		}
		break;
		}
	}

	bool context_get_display(unsigned p_index, metadb_handle_list_cref p_data, pfc::string_base& p_out, unsigned& p_displayflags, const GUID& p_caller) override {
		PFC_ASSERT(p_index >= 0 && p_index < get_num_items());
		get_item_name(p_index, p_out);

		if (!g_discogs) {
			//exit
			p_displayflags = FLAG_GRAYED;
			return true;
		}

		switch (p_index) {
		case FindDeletedReleases:
			p_displayflags = !g_discogs->locked_operation ? 0 : FLAG_GRAYED;
			break;
		case FindReleasesNotInCollect:
			p_displayflags = !g_discogs->locked_operation ? 0 : FLAG_GRAYED;
			break;
		}
		return true;
	}

	GUID get_item_guid(unsigned p_index) override {
		switch (p_index) {
		case FindDeletedReleases:
			return guid_FindDeletedReleases;
		case FindReleasesNotInCollect:
			return guid_FindReleasesNotInCollection;
		}
		return GUID_NULL;
	}

	bool get_item_description(unsigned p_index, pfc::string_base& p_out) override {
		switch (p_index) {
		case FindDeletedReleases:
			p_out = "Find Deleted Release ids...";
			break;
		case FindReleasesNotInCollect:
			p_out = "Find Releases not in Collection...";
			break;
		}
		return true;
	}
};

static contextmenu_item_factory_t<contextmenu_discogs> g_contextmenu_discogs_factory;
static contextmenu_item_factory_t<contextmenu_discogs_web> g_contextmenu_discogs_separator;
static contextmenu_item_factory_t<contextmenu_discogs_utilities> g_contextmenu_discogs_utilities_factory;
