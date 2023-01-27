#pragma once

class CYesNoApiDialog {

public:

	int query(HWND wndParent, std::vector<pfc::string8> values, bool bcancel = false, bool bwarning = true) {
		
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

		query.icon = bwarning ? popup_message_v3::iconWarning : popup_message_v3::iconInformation;
		query.buttons = popup_message_v3::buttonYes | popup_message_v3::buttonNo;
		if (bcancel) {
			query.buttons |= popup_message_v3::buttonCancel;
		}
		query.reply = comp_notify;

		popup_message_v3::get()->show_query_modal(query);

		if (res == popup_message_v3::buttonYes) {
			return 1;
		}
		else if (bcancel) {

				return (res == popup_message_v3::buttonNo ? 2 : 0);
		}
		else {
			return 0;
		}
	}
};