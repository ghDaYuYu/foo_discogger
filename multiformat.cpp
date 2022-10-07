#include "stdafx.h"

#include "multiformat.h"
#include "string_encoded_array.h"
#include "prompt_dialog.h"
bool titleformat_hook_impl_multiformat::process_field(titleformat_text_out * p_out, const char * p_name, size_t p_name_length, bool & p_found_flag) {
	size_t multi_depth = 0;
	while (p_name[0] == '<' && p_name[p_name_length - 1] == '>') {
		p_name += 1;
		p_name_length -= 2;
		multi_depth++;
	}
	abort_callback_impl p_abort; 
	pfc::string8 tag_name;
	string_encoded_array result;

	int tries = 0;
	while (1) {
		try {
			pfc::string8 temp(p_name, p_name_length);
			auto it = custom_map.find(temp);
			if (it != custom_map.cend()) {
				result = it->second;
			}
			else if (release != nullptr && STR_EQUALN(p_name, "RELEASE_", 8)) {
				tag_name = substr(p_name, 8, p_name_length - 8);
				result = (*release)->get_data(tag_name, p_status, p_abort);
			}
			else if ((release_track != nullptr || release != nullptr) && STR_EQUALN(p_name, "ARTISTS_", 8)) {
				tag_name = pfc::string8(p_name, p_name_length);
				if (release_track != nullptr && (*release_track)->artists.get_size()) {
					result = (*release_track)->get_data(tag_name, p_status, p_abort);
				}
				else if (release != nullptr) {
					result = (*release)->get_data(tag_name, p_status, p_abort);
				}
			}
			else if (artist != nullptr && STR_EQUALN(p_name, "ARTIST_", 7)) {
				tag_name = substr(p_name, 7, p_name_length - 7);
				result = (*artist)->get_data(tag_name, p_status, p_abort);
			}
			else if (release_track != nullptr && STR_EQUALN(p_name, "TRACK_", 6)) {
				tag_name = substr(p_name, 6, p_name_length - 6);
				result = (*release_track)->get_data(tag_name, p_status, p_abort);
			}
			else if (release_disc != nullptr && STR_EQUALN(p_name, "DISC_", 5)) {
				tag_name = substr(p_name, 5, p_name_length - 5);
				result = (*release_disc)->get_data(tag_name, p_status, p_abort);
			}
			else if (master_release != nullptr && STR_EQUALN(p_name, "MASTER_RELEASE_", 15)) {
				tag_name = substr(p_name, 15, p_name_length - 15);
				result = (*master_release)->get_data(tag_name, p_status, p_abort);
			}
			else if (image != nullptr && STR_EQUALN(p_name, "IMAGE_", 6)) {
				tag_name = substr(p_name, 6, p_name_length - 6);
				result = (*image)->get_data(tag_name, p_status, p_abort);
			}
			else {
				p_found_flag = false;
				return false;
			}
			break;
		}
		catch (missing_data_exception) {
			p_found_flag = false;
			return false;
		}
		catch (foo_discogs_exception &e) {
			foo_discogs_exception ex;
			ex << "Error processing field " << pfc::string8(p_name, p_name_length) << " : " << e.what();
			log_msg(ex.what());
			if (++tries > 2) {
				throw ex;
			}
		}
	}
	result.limit_depth(multi_depth);
	result.expand_depth(multi_depth);
	result.encode();
	p_out->write(titleformat_inputtypes::unknown, result.get_cvalue(), pfc::infinite_size);
	
	p_found_flag = result.get_cvalue().get_length() != 0;
	return true;
}

bool titleformat_hook_impl_multiformat::process_function(titleformat_text_out * p_out, const char * p_name,
		size_t p_name_length, titleformat_hook_function_params * p_params, bool & p_found_flag) {

	try {

		pfc::array_t<string_encoded_array> params;
		const size_t param_count = p_params->get_param_count();
		bool wrong_param_count = false;
		params.set_count(param_count);
		const char *field_name = nullptr;
		size_t field_name_length = 0;
		for (size_t i = 0; i < param_count; i++) {
			p_params->get_param(i, field_name, field_name_length);
			pfc::string8 str = pfc::string8(field_name, field_name_length);
			params[i] = string_encoded_array(str);
		}

		string_encoded_array *result = nullptr;

		if (pfc::strcmp_ex(p_name, p_name_length, "join", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				params[0].join();
			}
			else if (param_count == 2) {
				params[0].join(params[1]);
			}
			else {
				wrong_param_count = true;
			}
			result = &params[0];
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "joinnames", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].joinnames(params[1]);
			}
			else {
				wrong_param_count = true;
			}
			result = &params[0];
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "zip", pfc::infinite_size) == 0) {
			if (param_count) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].zip(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "zip2", pfc::infinite_size) == 0) {
			if (param_count) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].zip2(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_wrap", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].wrap(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "split", pfc::infinite_size) == 0) {
			if (param_count == 2 && !params[1].has_array()) {
				params[0].split(params[1].get_pure_cvalue());
			}
			else if (param_count == 1) {
				params[0].split();
			}
			else {
				wrong_param_count = true;
			}
			result = &params[0];
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "array", pfc::infinite_size) == 0) {
			if (param_count) {
				string_encoded_array temp = params[0];
				params[0].force_reset();
				params[0].append_item_val(temp);
				for (size_t i = 1; i < param_count; i++) {
					params[0].append_item_val(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "arrayn", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				string_encoded_array temp = params[0];
				params[0].force_reset();
				params[0].append_item_val(temp);
				result = &params[0];
			}
			else if (param_count == 2) {
				size_t length = params[1].get_numeric_value();
				params[1].force_reset();
				params[1].append_item_val(params[0]);
				params[1].increase_width(length);
				result = &params[1];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "unique", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				params[0].unique();
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "first", pfc::infinite_size) == 0 ) {
			if (param_count == 1) {
				if (params[0].has_array()) {
					result = &params[0].get_item_unsafe(0);
				}
				else {
					result = &params[0];
				}
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "last", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				if (params[0].has_array()) {
					result = &params[0].get_item_unsafe(params[0].get_width() - 1);
				}
				else {
					result = &params[0];
				}
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "element", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				size_t pos = params[1].get_numeric_value();
				if (params[0].has_array() && params[0].get_width() > pos) {
					result = &params[0].get_item_unsafe(pos);
				}
				else {
					params[0].set_value("");
					result = &params[0];
				}
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "length", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				pfc::string8 width;
				width << params[0].get_width();
				params[0].set_value(width);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_replace", pfc::infinite_size) == 0) {
			if (param_count == 3) {
				params[0].replace(params[1], params[2]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_replace_exp", pfc::infinite_size) == 0) {
			if (param_count == 3) {
				params[0].replace_exp(params[1], params[2]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_search_exp", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].search_exp(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_trim", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				params[0].trim();
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_strcmp", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].strcmp(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_istrcmp", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].istrcmp(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_strstr", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].strstr(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_len", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				params[0].multi_len();
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_add", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_add(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_sub", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_sub(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_mul", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_mul(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}
		
		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_div", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_div(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_mod", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].multi_mod(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_min", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_min(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_max", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_max(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_longest", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_longest(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_cut", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].multi_cut(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_left", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].multi_left(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_right", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].multi_right(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_greater", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].multi_greater(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_longer", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].multi_longer(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_pad", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].pad(params[1]);
				result = &params[0];
			}
			else if (param_count == 3) {
				params[0].pad(params[1], params[2]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_pad_right", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].pad_right(params[1]);
				result = &params[0];
			}
			else if (param_count == 3) {
				params[0].pad_right(params[1], params[2]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "any", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				params[0].any();
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "all", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				params[0].all();
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_if", pfc::infinite_size) == 0) {
			if (param_count == 3) {
				params[0].multi_if(params[1], params[2]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_if2", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].multi_if(params[0], params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_or", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_or(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_and", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_and(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_not", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				params[0].multi_not();
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "flatten", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				params[0].flatten();
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}
		else if (pfc::strcmp_ex(p_name, p_name_length, "extend", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].extend(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "append", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].force_array();
					params[0].append(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "sextend", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].shallow_extend(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "sappend", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].shallow_append(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "contains", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].contains(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "filter", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].filter(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "prompt", pfc::infinite_size) == 0) {
			if (param_count == 1 ) {
				try {
					params[0].set_value(prompt_store->get(params[0].get_cvalue()));
				}
				catch (foo_discogs_exception) {
					pfc::string8 answer;
					CPromptDialog dlg(params[0].get_cvalue(), &answer);
					dlg.DoModal();
					prompt_store->put(params[0].get_cvalue(), answer);
					params[0].set_value(answer);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (finfo != nullptr && pfc::strcmp_ex(p_name, p_name_length, "multi_meta", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				pfc::string8 name = params[0].get_pure_cvalue();
				const size_t count = finfo->meta_get_count_by_name(name);
				result = &params[0];
				result->force_reset();
				result->force_array();
				for (size_t i = 0; i < count; i++) {
					const char *val = finfo->meta_get(name, i);
					result->append_item_val(val);
				}
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (files != nullptr && pfc::strcmp_ex(p_name, p_name_length, "multi_meta_files", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				pfc::string8 name = params[0].get_pure_cvalue();
				result = &params[0];
				result->force_reset();
				result->force_array(2);
				for (size_t f = 0; f < files->get_item_count(); f++) {
					string_encoded_array temp;
					temp.force_array();
					metadb_handle_ptr item = files->get_item_handle(f);
					file_info &finfo = files->get_item(f);
					const size_t count = finfo.meta_get_count_by_name(name);
					for (size_t i = 0; i < count; i++) {
						const char *val = finfo.meta_get(name, i);
						temp.append_item_val(val);
					}
					result->append_item_val(temp);
				}
			}
			else {
				wrong_param_count = true;
			}
		}
		
		else if (store != nullptr && pfc::strcmp_ex(p_name, p_name_length, "pput", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				store->put(params[0].get_cvalue(), params[1].get_cvalue());
				result = &params[1];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (store != nullptr && pfc::strcmp_ex(p_name, p_name_length, "pputs", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				store->put(params[0].get_cvalue(), params[1].get_cvalue());
				params[0].force_reset();
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (store != nullptr && pfc::strcmp_ex(p_name, p_name_length, "pget", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				params[0].set_value(store->get(params[0].get_cvalue()));
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "count", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				params[0].count();
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "sum", pfc::infinite_size) == 0) {
			if (param_count == 1) {
				params[0].sum();
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_add", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_add(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_sub", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_sub(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_mul", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_mul(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_div", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_div(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_divd", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_divd(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_round", pfc::infinite_size) == 0) {
			if (param_count == 2) {
				params[0].multi_round(params[1]);
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else if (pfc::strcmp_ex(p_name, p_name_length, "multi_mod", pfc::infinite_size) == 0) {
			if (param_count > 1) {
				for (size_t i = 1; i < param_count; i++) {
					params[0].multi_mod(params[i]);
				}
				result = &params[0];
			}
			else {
				wrong_param_count = true;
			}
		}

		else {
			p_found_flag = false;
			return false;
		}

		if (wrong_param_count) {
			foo_discogs_exception ex;
			ex << "Function $" << pfc::string8(p_name, p_name_length) << " invalid number of arguments.";
			throw ex;
		}

		result->encode();
		p_out->write(titleformat_inputtypes::unknown, result->get_cvalue());
		p_found_flag = result->get_cvalue().get_length() != 0;
		return true;
	}
	catch (foo_discogs_exception &e) {
		foo_discogs_exception ex;
		ex << "Error processing function $" << pfc::string8(p_name, p_name_length) << " : " << e.what();
		throw ex;
	}
}
