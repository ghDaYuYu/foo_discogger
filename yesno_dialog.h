#pragma once

class CYesNoApiDialog {

public:

	int query(HWND wndParent, std::vector<pfc::string8> values) {
		
		int res = ~0;

		completion_notify::ptr reply;
		fb2k::completionNotifyFunc_t comp_func = [&res](unsigned op) {
			res = op;
		};
		completion_notify::ptr comp_notify = fb2k::makeCompletionNotify(comp_func);

		popup_message_v3::query_t query = { 0 };
		query.wndParent = wndParent;
		query.title = values[0];
		query.msg = values[1];
		query.icon = popup_message_v3::iconWarning;
		query.buttons = popup_message_v3::buttonYes | popup_message_v3::buttonNo;
		query.reply = comp_notify;

		popup_message_v3::get()->show_query_modal(query);

		if (res == popup_message_v3::buttonYes) {
			return 1;
		}
		else {
			return 0;
		}
	}
};