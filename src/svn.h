/**
 * NodeJS SVN bindings
 *
 * This bindings allow filesystem-level access to Subversion
 * reposotories. This will allow you to interact with Subversion
 * without requiring a local working copy.
 *
 * This module will be expanded to be a full and functioning SVN
 * library for NodeJS
 *
 * @author Daniel Enman
 */
#ifndef SVN_H
#define SVN_H

// I'm not really sure why the compiler doesn't define __DARWIN__ on Lion
#ifdef __APPLE__
#define DARWIN
#endif

// Node/V8
#include <node.h>
#include <v8.h>

// SVN Includes
#include <svn_repos.h>
#include <svn_ra.h>
#include <svn_fs.h>
#include <svn_wc.h>
#include <svn_pools.h>
#include <svn_path.h>
#include <svn_utf.h>
#include <svn_auth.h>
#include <svn_client.h>

//APR Inludes
#include <apr_tables.h>

// Other
#include <stdlib.h>
#include <sys/stat.h>

using namespace node;
using namespace v8;

#define EXCEPTION(message) \
    return ThrowException(Exception::Error(String::New(message)))

#define ERROR(message) \
    result = Object::New(); \
    result->Set(status_symbol, Integer::New(0)); \
    result->Set(message_symbol, String::New(message)); \
    svn_pool_destroy(subpool); \
    subpool = NULL; \
    return result;

#define OPTIONS_EXCEPTION(message) \
    *has_err = true; \
    *err_msg = message; \
    return options; 

typedef struct options_t {
   svn_opt_revision_t revision_start;
   svn_opt_revision_t revision_end;
   svn_depth_t depth;
   svn_depth_t set_depth;
   svn_boolean_t ignore_externals;
   svn_boolean_t force; //aka allow_unver_obstructions
   svn_boolean_t parents;
} options_t;

typedef struct notify_item {
    const char *path;
    svn_wc_notify_state_t action;
    notify_item *next;

} notify_item;

typedef struct notify_list {
    notify_item *first;
    notify_item *last;
} notify_list;

enum options_interest {
    kRevision = 1,
    kRecursive = 1 << 1,
    kDepth = 1 << 2,
    kSetDepth = 1 << 3,
    kForce = 1 << 4,
    kIgnoreExternals = 1 << 5,
    kParents = 1 << 6,
    kCheckout = kRevision | kRecursive | kDepth | kForce | kIgnoreExternals,
    kUpdate = kCheckout | kSetDepth | kParents
};

class SVN : public ObjectWrap
{
public:
	static Persistent<FunctionTemplate> ct;
    static Persistent<String> revision_symbol;
    static Persistent<String> recursive_symbol;
    static Persistent<String> depth_symbol;
    static Persistent<String> set_depth_symbol;
    static Persistent<String> force_symbol;
    static Persistent<String> externals_symbol;
    static Persistent<String> parents_symbol;

    static Persistent<String> status_symbol;
    static Persistent<String> data_symbol;

    static Persistent<String> description_symbol;
    static Persistent<String> message_symbol;
    static Persistent<String> error_symbol;

	SVN(const char *config);
	static void InitModule(Handle<Object> target); // V8/Node initializer
	static Handle<Value> New(const Arguments &args); // 'new' construct
	
	void init_auth();
	void simple_authentication(const char *username, const char *password);

	~SVN()
	{
		svn_pool_destroy(this->pool);
		this->pool = NULL;
		apr_terminate();
	}

protected:
	// SVN-defined functions, that are available to NodeJS
	static Handle<Value> __checkout(const Arguments &args);
	static Handle<Value> __import(const Arguments &args);
	static Handle<Value> __blame(const Arguments &args);
	static Handle<Value> __cat(const Arguments &args);
	static Handle<Value> __copy(const Arguments &args);
	static Handle<Value> __del(const Arguments &args);
	static Handle<Value> __exp(const Arguments &args);
	static Handle<Value> __info(const Arguments &args);
	static Handle<Value> __list(const Arguments &args);
	static Handle<Value> __log(const Arguments &args);
	static Handle<Value> __mkdir(const Arguments &args);
	static Handle<Value> __move(const Arguments &args);
	static Handle<Value> __add(const Arguments &args);
	static Handle<Value> __cleanup(const Arguments &args);
	static Handle<Value> __commit(const Arguments &args);
	static Handle<Value> __revert(const Arguments &args);
	static Handle<Value> __status(const Arguments &args);
	static Handle<Value> __update(const Arguments &args);
    static Handle<Value> __upgrade(const Arguments &args);

	Handle<Value> do_cat(const Handle<String> url, svn_opt_revision_t revision);

	// SVN baton authentication
	static Handle<Value> __authenticate(const Arguments &args);

	// Class methods

	// Accessors

	// Inceptors
    
    // Helpers
    options_t default_options();
    options_t parse_options(const Handle<Value> opt_arg, options_interest opt_intrst, apr_pool_t *pool, bool *has_err, const char **err_msg);
private:
	apr_pool_t *pool;
	svn_client_ctx_t *ctx;

	const char* config; // Path to config	
	Handle<Object> error(svn_error_t *error);
};

#endif
