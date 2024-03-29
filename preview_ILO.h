#pragma once
#include "libPPUI\CListControlOwnerData.h"
#include "foo_discogs.h"
#include "tag_writer.h"

class CPreviewList;

struct preview_stats {
	int m_totalwrites = 0;
	int m_totalupdates = 0;
	int totalsubwrites = 0;
	int totalsubupdates = 0;
	int totalskips = 0;
	int totalequal = 0;
};

class ILOD_preview : public IListControlOwnerDataSource {

public:

	typedef CPreviewList TParent;

	ILOD_preview() : m_preview_mode(PreView::Undef) {
		//..
	}

	void Enabled(bool enable);
	void SetResults(TagWriter_ptr tag_writer, PreView preview_mode, std::vector<preview_stats> m_vstats);
	pfc::string8 GetListRow(size_t row, PreView mode);
	bool is_result_editable(size_t item);

private:

	// pure virtual

	virtual CListControlOwnerData* ilo_get_uilist() = 0;

	// IListControlOwnerDataSource overrides

	size_t listGetItemCount(ctx_t ctx) override;
	pfc::string8 listGetSubItemText(ctx_t, size_t item, size_t subItem) override;

	void listSubItemClicked(ctx_t, size_t item, size_t subItem) override;
	void listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) override;
	pfc::string8 listGetEditField(ctx_t ctx, size_t item, size_t subItem, size_t& lineCount) override;
	uint32_t listGetEditFlags(ctx_t ctx, size_t item, size_t subItem) override;
	bool listIsColumnEditable(ctx_t, size_t subItem) override;
	void listSelChanged(ctx_t) override;
	void listItemAction(ctx_t, size_t) override;
	bool listIsSubItemGrayed(ctx_t, size_t item, size_t subItem) override;

	bool listEditCanAdvanceHere(ctx_t, size_t item, size_t subItem, uint32_t whatHappened) override;

	TagWriter_ptr m_tag_writer;
	PreView m_preview_mode = PreView::Undef;
	std::vector<preview_stats> m_v_stats;

};
