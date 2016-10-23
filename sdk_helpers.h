#include "../SDK/foobar2000.h"

#pragma once


class fake_threaded_process_status : public threaded_process_status
{
	void set_progress(t_size p_state) {}
	void set_progress_secondary(t_size p_state) {}
	void set_item(const char * p_item, t_size p_item_len = ~0) {}
	void set_item_path(const char * p_item, t_size p_item_len = ~0) {}
	void set_title(const char * p_title, t_size p_title_len = ~0) {}
	void force_update() {}
	bool is_paused() { return false; }
	bool process_pause() { return false; }
};
