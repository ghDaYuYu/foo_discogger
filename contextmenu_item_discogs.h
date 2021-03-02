#pragma once


#include "../SDK/foobar2000.h"
#include "tasks.h"
#include "find_release_dialog.h"
#include "tag_mappings_dialog.h"
#include "configuration_dialog.h"
#include "update_art_dialog.h"
#include "update_tags_dialog.h"
#include "tags.h"

// {2C6B4148-365D-4585-BA74-724F06192E4A}
static const GUID guid_WriteTags =
{0x2c6b4148, 0x365d, 0x4585, {0xba, 0x74, 0x72, 0x4f, 0x6, 0x19, 0x2e, 0x4c}};

// {99649CA2-0759-4E29-8CBA-FA5D4F496CAC}
static const GUID guid_WriteTagsDropId =
{0x99649ca2, 0x759, 0x4e29, {0x8c, 0xba, 0xfa, 0x5d, 0x4f, 0x49, 0x6c, 0xac}};

// {8A4C8171-0FA6-4137-BC12-C00C451693D5}
static const GUID guid_UpdateTags =
{0x8a4c8171, 0xfa6, 0x4137, {0xbc, 0x12, 0xc0, 0xc, 0x45, 0x16, 0x93, 0xd7}};

// {974E3297-A1BC-4e0e-906F-9DCC61AC8026}
static const GUID guid_UpdateArt =
{0x974e3297, 0xa1bc, 0x4e0e, {0x90, 0x6f, 0x9d, 0xcc, 0x61, 0xac, 0x80, 0x28}};

// {7F9E5387-D8A2-4b88-B2E8-6C47D57F825F}
static const GUID guid_DisplayReleasePage =
{0x7f9e5387, 0xd8a2, 0x4b88, {0xb2, 0xe8, 0x6c, 0x47, 0xd5, 0x7f, 0x82, 0x70}};

// {DFAA6920-5B19-450B-B956-84EDDD1E4E0B}
static const GUID guid_DisplayMasterReleasePage =
{0xdfaa6920, 0x5b19, 0x450b, {0xb9, 0x56, 0x84, 0xed, 0xdd, 0x1e, 0x4e, 0xb}};

// {D3CD1BC2-BA8A-4e03-A979-22F03C73E33C}
static const GUID guid_DisplayArtistPage =
{0xd3cd1bc2, 0xba8a, 0x4e03, {0xa9, 0x79, 0x22, 0xf0, 0x3c, 0x73, 0xe3, 0x3e}};

// {397E8445-3A39-49bb-8FA3-04A1A6365127}
static const GUID guid_DisplayArtistArtPage =
{0x397e8445, 0x3a39, 0x49bb, {0x8e, 0xa4, 0x4, 0xa1, 0xa6, 0x36, 0x51, 0x27}};

// {56F50DF1-E7BF-4cf4-B8AD-FAF2C3021F0E}
static const GUID guid_DisplayLabelPage =
{0x56f50df1, 0xe7bf, 0x4cf4, {0xb8, 0xad, 0xfa, 0xf2, 0xc3, 0x2, 0x1f, 0xA}};

// {F8E2EEFA-390E-43ca-A415-93F1C781F65D}
static const GUID guid_DisplayAlbumArtPage =
{0xf8e2eefa, 0x390e, 0x43ca, {0xa4, 0x15, 0x93, 0xf1, 0xc7, 0x81, 0xf6, 0x5f}};

// {E3BC0088-1B17-4b89-A9A4-6D9446E313E2}
static const GUID guid_EditTagMappings =
{0xe3bc0088, 0x1b17, 0x4b89, {0xa9, 0xa4, 0x6d, 0x94, 0x46, 0xe3, 0x13, 0xe4}};

// {62B3BD51-EDE9-4141-B06B-F1BB7377F578}
static const GUID guid_Configuration =
{0x62b3bd51, 0xede9, 0x4141, {0xb0, 0x6b, 0xf1, 0xbb, 0x73, 0x77, 0xf5, 0x80}};

// {DBA80FEA-8345-474e-A3B9-60D14E30ADD0}
static const GUID guid_FindDeletedReleases =
{0xdba80fea, 0x8345, 0x474e, {0xa3, 0xb9, 0x60, 0xd1, 0x4e, 0x30, 0xad, 0xd0}};

// {CFFA3E68-AAD8-4CAF-9AD8-F6944E8EE094}
static const GUID guid_FindReleasesNotInCollection =
{ 0xcffa3e68, 0xaad8, 0x4caf, { 0x9a, 0xd8, 0xf6, 0x94, 0x4e, 0x8e, 0xe0, 0x94 } };


static const GUID contextmenu_group_discogs_id = {0xfe6795d9, 0xbd08, 0x470e, {0x89, 0x45, 0x74, 0xc3, 0xe0, 0x1f, 0x42, 0xb4}};
static service_factory_single_t<contextmenu_group_popup_impl> mygroup(contextmenu_group_discogs_id, contextmenu_groups::tagging, "Discogs", 0);

static const GUID contextmenu_group_discogs_utilities_id = {0x7d00e1e6, 0xedee, 0x4611, { 0x9d, 0x91, 0x31, 0x5, 0xf2, 0x30, 0xc7, 0x5}};
static service_factory_single_t<contextmenu_group_popup_impl> mygroup2(contextmenu_group_discogs_utilities_id, contextmenu_group_discogs_id, "Utilities", 0);


class contextmenu_discogs : public contextmenu_item_simple
{
private:
	enum MenuIndex
	{
		WriteTags,
		WriteTagsDropId,
		UpdateTags,
		UpdateArt,
		DisplayReleasePage,
		DisplayMasterReleasePage,
		DisplayArtistPage,
		DisplayArtistArtPage,
		DisplayLabelPage,
		DisplayAlbumArtPage,
		EditTagMappings,
		Configuration,
	};

public:

	GUID get_parent() override {
		return contextmenu_group_discogs_id;
	}

	unsigned get_num_items() override {
		return 12;
	}

	void get_item_name(unsigned p_index, pfc::string_base & p_out) override {
		switch (p_index) {
			case WriteTags:
				p_out = "Write Tags...";
				break;
			case UpdateTags:
				p_out = "Update Tags...";
				break;
			case UpdateArt:
				p_out = "Update Album/Artist Art...";
				break;
			case DisplayReleasePage:
				p_out = "View Release Page";
				break;
			case DisplayMasterReleasePage:
				p_out = "View Master Release Page";
				break;
			case DisplayArtistPage:
				p_out = "View Artist Page";
				break;
			case DisplayArtistArtPage:
				p_out = "View Artist Art Page";
				break;
			case DisplayLabelPage:
				p_out = "View Label Page";
				break;
			case DisplayAlbumArtPage:
				p_out = "View Album Art Page";
				break;
			case EditTagMappings:
				p_out = "Edit Tag Mappings...";
				break;
			case Configuration:
				p_out = "Configuration...";
				break;
			case WriteTagsDropId: {
				p_out = "Write Tags (Update Release Id)...";
				break;
			}

		}
	}

	void get_item_default_path(unsigned p_index, pfc::string_base & p_out) override {
		p_out = "Discogs";
	}

	void context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller) override {
		unsigned displayflags;
		pfc::string8 out;
		context_get_display(p_index, p_data, out, displayflags, p_caller);
		if (displayflags & FLAG_GRAYED) {
			return;
		}

		switch (p_index) {
			case WriteTags:
				g_discogs->find_release_dialog = new CFindReleaseDialog(core_api::get_main_window(), p_data, false);
				break;

			case UpdateTags:
				g_discogs->update_tags_dialog = new CUpdateTagsDialog(core_api::get_main_window(), p_data);
				break;

			case UpdateArt:
				g_discogs->update_art_dialog = new CUpdateArtDialog(core_api::get_main_window(), p_data);
				break;

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

			case EditTagMappings:
				g_discogs->tag_mappings_dialog = new CNewTagMappingsDialog(core_api::get_main_window());
				break;

			case Configuration:
				static_api_ptr_t<ui_control>()->show_preferences(guid_pref_page);
				break;

			case WriteTagsDropId:
				g_discogs->find_release_dialog = new CFindReleaseDialog(core_api::get_main_window(), p_data, true);
				break;
		}
	}

	bool context_get_display(unsigned p_index, metadb_handle_list_cref p_data, pfc::string_base & p_out, unsigned & p_displayflags, const GUID & p_caller) override {
		PFC_ASSERT(p_index >= 0 && p_index < get_num_items());
		get_item_name(p_index, p_out);
		switch (p_index) {
			case WriteTags:
				p_displayflags = !g_discogs->locked_operation && !g_discogs->find_release_dialog && !g_discogs->track_matching_dialog && !g_discogs->preview_tags_dialog ? 0 : FLAG_GRAYED;
				break;
			case WriteTagsDropId: {
				p_displayflags = !g_discogs->locked_operation && !g_discogs->find_release_dialog && !g_discogs->track_matching_dialog && !g_discogs->preview_tags_dialog ? 0 : FLAG_GRAYED;
				break;
			}
			case UpdateTags:
				p_displayflags = !g_discogs->locked_operation && !g_discogs->update_tags_dialog && !g_discogs->find_release_dialog && !g_discogs->track_matching_dialog && !g_discogs->preview_tags_dialog ? 0 : FLAG_GRAYED;
				break;
			case UpdateArt:
				p_displayflags = !g_discogs->locked_operation && !g_discogs->update_art_dialog && g_discogs->some_item_has_tag(p_data, TAG_RELEASE_ID) ? 0 : FLAG_GRAYED;
				break;
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
			case WriteTagsDropId:
				return guid_WriteTagsDropId;
			case UpdateTags:
				return guid_UpdateTags;
			case UpdateArt:
				return guid_UpdateArt;
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
			case EditTagMappings:
				return guid_EditTagMappings;
			case Configuration:
				return guid_Configuration;
		}
		return GUID_NULL;
	}

	bool get_item_description(unsigned p_index, pfc::string_base & p_out) override {
		switch (p_index) {
			case WriteTags:
				p_out = "Write Tags";
				break;
			case WriteTagsDropId:
				p_out = "Write Tags (Update Release Id)";
				break;
			case UpdateTags:
				p_out = "Update Tags";
				break;
			case UpdateArt:
				p_out = "Update Album/Artist Art";
				break;
			case DisplayReleasePage:
				p_out = "View Release Page";
				break;
			case DisplayMasterReleasePage:
				p_out = "View Master Release Page";
				break;
			case DisplayArtistPage:
				p_out = "View Artist Page";
				break;
			case DisplayArtistArtPage:
				p_out = "View Artist Art Page";
				break;
			case DisplayLabelPage:
				p_out = "View Label Page";
				break;
			case EditTagMappings:
				p_out = "Edit Tag Mappings";
				break;
			case Configuration:
				p_out = "Configuration";
				break;
		}
		return true;
	}

	bool item_is_mappable_shortcut(unsigned p_index)
	{
		if (p_index == WriteTagsDropId)
			return false;
		else
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

	void get_item_name(unsigned p_index, pfc::string_base & p_out) override {
		switch (p_index) {
		case FindDeletedReleases:
			p_out = "Find Deleted Releases...";
			break;
		case FindReleasesNotInCollect:
			p_out = "Find Releases not in Collection...";
			break;
		}
	}

	void get_item_default_path(unsigned p_index, pfc::string_base & p_out) override {
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
									  service_ptr_t<find_deleted_releases_task> task = new service_impl_t<find_deleted_releases_task>(p_data);
									  task->start();
		}
			break;

		case FindReleasesNotInCollect: {
										   service_ptr_t<find_releases_not_in_collection_task> task = new service_impl_t<find_releases_not_in_collection_task>(p_data);
										   task->start();
		}
			break;
		}
	}

	bool context_get_display(unsigned p_index, metadb_handle_list_cref p_data, pfc::string_base & p_out, unsigned & p_displayflags, const GUID & p_caller) override {
		PFC_ASSERT(p_index >= 0 && p_index < get_num_items());
		get_item_name(p_index, p_out);
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

	bool get_item_description(unsigned p_index, pfc::string_base & p_out) override {
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
static contextmenu_item_factory_t<contextmenu_discogs_utilities> g_contextmenu_discogs_utilities_factory;
