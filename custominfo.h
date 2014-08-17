#ifndef _CUSTOMINFO_H_
#define _CUSTOMINFO_H_
#include <share.h>
#include "../SDK/foobar2000.h"
#include "../SDK/core_api.h"
#include "x_info.h"

class custominfo;
class custominfo_reader;
class custominfo_reader_writer;
class custominfo_handle;
typedef service_ptr_t<custominfo_handle> custominfo_handle_ptr;
class custominfo_storage;


/*
SDK for foo_custominfo 0.1
*/



/*
Provides static methods for getting an interface (custominfo_reader_writer/custominfo_reader) 
to custominfo data.
- You may request either normal or read-only access. Write access is exlusive, and
  read acces may be either shared or exclusive depending on implementation. Lock section is exited
  when the interface is destroyed.
- Return values are false if custominfo is disabled or not installed, otherwise true.
- Example of usage: getting interface and setting a field
	
	service_ptr_t<custominfo_reader_writer> cirw;
	if(custominfo::g_open(cirw)) {
		custominfo_handle_ptr entry;
		cirw->handle_create(entry,some_metadb_handle_ptr);
		cirw->set_field(entry,"RATING","5");
	}

- All strings are utf-8.
- Field names and file names are case insensitive.
*/
class NOVTABLE custominfo : public service_base {
public:
	static inline bool g_open(service_ptr_t<custominfo_reader_writer> &ptr);
	static inline bool g_open_read(service_ptr_t<custominfo_reader> &ptr);

	virtual bool open(service_ptr_t<custominfo_reader_writer> &ptr) = 0;
	virtual bool open_read(service_ptr_t<custominfo_reader> &ptr) = 0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(custominfo);
};

//Interface for reading custominfo data.
//All methods use exceptions to signal about failures. Thus, exception handling is needed.
class NOVTABLE custominfo_reader : public service_base {
public:
	//Creates a new custominfo handle from a metadb handle.
	virtual void handle_create(custominfo_handle_ptr &out, const metadb_handle_ptr & in)=0;
	//Retrieves a list of handles to all entries in the storage.
	virtual void get_all_entries(pfc::list_base_t<custominfo_handle_ptr>& entries) = 0;

	//Returns whether handle has any values associated with it. 
	virtual bool has_info(const custominfo_handle_ptr& entry) = 0;
	//Retrieves all values for specified entry in x_info structure. "info" will be empty if no values found.
	virtual void get_info(const custominfo_handle_ptr& entry, x_info& info) = 0;
	//Reads info fields for multiple items. "info_list" must be initialized by caller to contain correct
	//number of pointers to existing x_info structures.
	virtual void get_info_multi(const pfc::list_base_const_t<custominfo_handle_ptr> &entries,const pfc::list_base_const_t<x_info*> & info_list) = 0;
	//Retrieves a single (n+1:th) value of the field with given name. Returns whether field and value was found.
	virtual bool get_value(const custominfo_handle_ptr& entry, const char* name, t_size n, pfc::string_base& value) = 0;
	//Returns number of values for a field with given name.
	virtual t_size	get_value_count(const custominfo_handle_ptr& entry, const char* name) = 0;

	//For informative/statistical purposes only. Return -1 if not known.
	virtual t_ssize get_entry_count() {return -1;}
	virtual t_ssize get_total_value_count() {return -1;}

	FB2K_MAKE_SERVICE_INTERFACE(custominfo_reader,service_base)
};

//Interface for read and write access to custominfo data.
//All methods use exceptions to signal about failures. Thus, exception handling is needed.
class NOVTABLE custominfo_reader_writer : public custominfo_reader {
public:
	//Clears all storage contents.
	virtual void clear() = 0;

	//Sets info for the specified item.
	virtual void set_info(const custominfo_handle_ptr& entry, const x_info& info) = 0;
	//Sets info for multiple items.
	virtual void set_info_multi(const pfc::list_base_const_t<custominfo_handle_ptr> & entries,const pfc::list_base_const_t<x_info*> & info_list) = 0;
	//Copies all values from one entry to another (target entry is cleared first).
	virtual void copy_info(const custominfo_handle_ptr& source,const custominfo_handle_ptr& target) = 0;
	//Removes info (all values) associated with the specified handle.
	virtual void remove_info(const custominfo_handle_ptr& entry) = 0;
	//Creates a new value.
	virtual void add_value(const custominfo_handle_ptr& entry, const char* name, const char* value) = 0;
	//Removes a field( = all values) with given name.
	virtual void remove_field(const custominfo_handle_ptr& entry, const char* name) = 0;
	//Sets new value for a field, replacing all existing values.
	virtual void set_field(const custominfo_handle_ptr& entry, const char* name, const char* value) = 0;

	FB2K_MAKE_SERVICE_INTERFACE(custominfo_reader_writer,custominfo_reader)
};

/*
Handle to a custominfo storage item with certain filename and subsong index.
Use custominfo_reader::handle_create() to create a handle.
*/
class NOVTABLE custominfo_handle : public service_base {
public:
	virtual rcplayable_location get_location() = 0;
	inline const char* get_path() {return get_location().get_path();}
	inline t_uint32 get_subsong_index() {return get_location().get_subsong_index();}

	virtual void get_metadb_handle(metadb_handle_ptr& out) {static_api_ptr_t<metadb>()->handle_create(out,get_location());}

	FB2K_MAKE_SERVICE_INTERFACE(custominfo_handle,service_base)
};

typedef service_ptr_t<custominfo_handle> custominfo_handle_ptr;






/*
Custominfo storage service for implementing customized info storing method.
- storage instances and the readers/writers/handles returned by them should not be used directly
  by 3rd party components wishing to access custominfo data, but through custominfo service,
  which automatically wraps handles and reader/writer interfaces, refreshes info changes to UI,
  updates locations of files moved within fb2k, removes non-library entries, etc.
ABOUT IMPLEMENTING READER/WRITER/HANDLE CLASSES:
- Calls to methods of these classes are not synchronized, so correct synchronizing must be implemented. I.e. entering
  lock section when reader/writer instance is created, and exiting when instance is destroyed. Write access
  must be exclusive. Also, a thread shouldn't be able to block itself by opening several readers/writers.
- Avoid calling foobar2k api functions as much as possible, because we don't know anything
  about the calling context (may be inside callbacks, metadb lock sections etc..)
- Make sure handle_create()/get_info()/get_value()/get_value_count() execute fast, because they are called
  inside metadb display hook quite often.
- handles are required to be valid as long as the storage service is in activated state.  
*/
class NOVTABLE custominfo_storage : public service_base {
public:
	//Name or short description that is shown in the drop-down list of the preferences dialog.
	virtual const char* get_name() = 0;
	//Guid that is used to identify the service.
	virtual const GUID& get_guid() = 0;

	//Called when user selects/deselects this storage in preferences, or when foobar starts/exits while
	//this is the selected storage. Activate is guaranteed to be fully executed before any calls to open/open_read,
	//and, respectively, deactivate is not called until all custominfo_reader/writer instances are destroyed.
	virtual void activate(const char* params) {}
	virtual void deactivate() {}

	//Called by custominfo core when some 3rd party has requested access to the data. It's recommended to create a new
	//reader/writer instance which automatically handles synchronizing the data correctly.
	virtual void open(service_ptr_t<custominfo_reader_writer> &ptr) = 0;
	virtual void open_read(service_ptr_t<custominfo_reader> &ptr) {service_ptr_t<custominfo_reader_writer> ptr2;open(ptr2);ptr=ptr2;}

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(custominfo_storage);
};



bool custominfo::g_open(service_ptr_t<custominfo_reader_writer> &ptr){
	service_ptr_t<custominfo> ci;
	service_enum_t<custominfo> e;
	return e.first(ci) && ci->open(ptr);
}

bool custominfo::g_open_read(service_ptr_t<custominfo_reader> &ptr){
	service_ptr_t<custominfo> ci;
	service_enum_t<custominfo> e;
	return e.first(ci) && ci->open_read(ptr);
}

// {87AC8688-D80E-491a-B488-6C0EBFFEE465}
FOOGUIDDECL const GUID custominfo_handle::class_guid = 
{ 0x87ac8688, 0xd80e, 0x491a, { 0xb4, 0x88, 0x6c, 0xe, 0xbf, 0xfe, 0xe4, 0x65 } };
// {940875AB-8B2E-4010-9646-9C88B1DDDB5B}
FOOGUIDDECL const GUID custominfo_reader::class_guid = 
{ 0x940875ab, 0x8b2e, 0x4010, { 0x96, 0x46, 0x9c, 0x88, 0xb1, 0xdd, 0xdb, 0x5b } };
// {CCBD1106-46E3-4215-BD90-F8FFBD68FAC7}
FOOGUIDDECL const GUID custominfo_reader_writer::class_guid = 
{ 0xccbd1106, 0x46e3, 0x4215, { 0xbd, 0x90, 0xf8, 0xff, 0xbd, 0x68, 0xfa, 0xc7 } };
// {8B9E68E5-FEBE-4565-9F41-C1937F6B5A4E}
FOOGUIDDECL const GUID custominfo::class_guid = 
{ 0x8b9e68e5, 0xfebe, 0x4565, { 0x9f, 0x41, 0xc1, 0x93, 0x7f, 0x6b, 0x5a, 0x4e } };
// {D79B621D-F781-429d-AC00-80746FCDC269}
FOOGUIDDECL const GUID custominfo_storage::class_guid = 
{ 0xd79b621d, 0xf781, 0x429d, { 0xac, 0x0, 0x80, 0x74, 0x6f, 0xcd, 0xc2, 0x69 } };

#endif _CUSTOMINFO_H_
