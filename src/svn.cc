#include "svn.h"

SVN::SVN(const char *config = NULL) 
{
	HandleScope scope;

	svn_error_t *err;

	apr_initialize();

	this->pool = svn_pool_create(NULL); // Create the APR memory pool
	
	// Create our client context
	if ( (err = svn_client_create_context(&this->ctx, this->pool)) )
	{
		this->error(err);
		svn_pool_destroy(this->pool);
		this->pool = NULL;
		return;
	}

	// If we have a config path defined, that will be used. Otherwise, we're set
	// without one
	if( (err = svn_config_get_config(&this->ctx->config, config, this->pool)) )
	{
		this->error(err);
		svn_pool_destroy(this->pool);
		this->pool = NULL;
		return;
	}

	// Add our providers for authentication and set up
	// our client for proper authentication
	this->init_auth();
}

void SVN::InitModule(Handle<Object> target)
{
	HandleScope scope;

	Local<FunctionTemplate> t = FunctionTemplate::New(New);
	ct = Persistent<FunctionTemplate>::New(t);
	ct->InstanceTemplate()->SetInternalFieldCount(1);
	ct->SetClassName(String::NewSymbol("client"));

	NODE_SET_PROTOTYPE_METHOD(ct, "authenticate", __authenticate);
	NODE_SET_PROTOTYPE_METHOD(ct, "cat", __cat);
	NODE_SET_PROTOTYPE_METHOD(ct, "checkout", __checkout);
	NODE_SET_PROTOTYPE_METHOD(ct, "update", __update);
	NODE_SET_PROTOTYPE_METHOD(ct, "commit", __commit);
    NODE_SET_PROTOTYPE_METHOD(ct, "upgrade", __upgrade);

    //enum svn_depth_t
    Local<Object> depth = Object::New();
    NODE_DEFINE_CONSTANT(depth, svn_depth_unknown);
    NODE_DEFINE_CONSTANT(depth, svn_depth_exclude);
    NODE_DEFINE_CONSTANT(depth, svn_depth_empty);
    NODE_DEFINE_CONSTANT(depth, svn_depth_files);
    NODE_DEFINE_CONSTANT(depth, svn_depth_immediates);
    NODE_DEFINE_CONSTANT(depth, svn_depth_infinity);


    //enum svn_wc_notify_state_t
    Local<Object> state = Object::New();
    NODE_DEFINE_CONSTANT(state, svn_wc_notify_state_inapplicable);
    NODE_DEFINE_CONSTANT(state, svn_wc_notify_state_unknown);
    NODE_DEFINE_CONSTANT(state, svn_wc_notify_state_unchanged);
    NODE_DEFINE_CONSTANT(state, svn_wc_notify_state_missing);
    NODE_DEFINE_CONSTANT(state, svn_wc_notify_state_obstructed);
    NODE_DEFINE_CONSTANT(state, svn_wc_notify_state_changed);
    NODE_DEFINE_CONSTANT(state, svn_wc_notify_state_merged);
    NODE_DEFINE_CONSTANT(state, svn_wc_notify_state_conflicted);
    NODE_DEFINE_CONSTANT(state, svn_wc_notify_state_source_missing);

	target->Set(String::NewSymbol("client"), ct->GetFunction());
    target->Set(String::NewSymbol("depth"), depth);
    target->Set(String::NewSymbol("state"), state);
}

Handle<Value> SVN::New(const Arguments &args)
{
	HandleScope scope;
	SVN *svn = new SVN();

	String::AsciiValue config(args[0]->ToString());
	svn = new SVN(*config);
	
	svn->Wrap(args.This());

	return args.This();
}

Handle<Object> SVN::error(svn_error_t *error)
{
	HandleScope scope;
	svn_error_t *err_it = error;

    Local<Object> obj = Object::New();
    Local<Object> obj_it = obj;

	while( err_it )
	{
		char buffer[256];
		svn_strerror(err_it->apr_err, buffer, sizeof(buffer));

        //obj_it->Set(status_symbol, (int) err_it->app_err);
        obj_it->Set(description_symbol, String::New(buffer));
        if (err_it->message) {
            obj_it->Set(message_symbol, String::New(err_it->message));
        }

		err_it = err_it->child;
		if(err_it != NULL)
		{
            Local<Object> obj_tmp = Object::New();
            obj_it->Set(error_symbol, obj_tmp);
            obj_it = obj_tmp;
		}
	}

	return scope.Close(obj);
}

//options symbol
Persistent<FunctionTemplate> SVN::ct;
Persistent<String> SVN::revision_symbol = NODE_PSYMBOL("revision");
Persistent<String> SVN::recursive_symbol = NODE_PSYMBOL("recursive");
Persistent<String> SVN::depth_symbol = NODE_PSYMBOL("depth");
Persistent<String> SVN::set_depth_symbol = NODE_PSYMBOL("set_depth");
Persistent<String> SVN::force_symbol = NODE_PSYMBOL("force");
Persistent<String> SVN::externals_symbol = NODE_PSYMBOL("ignore_externals");
Persistent<String> SVN::parents_symbol = NODE_PSYMBOL("parents");

//return object symbols
Persistent<String> SVN::status_symbol = NODE_PSYMBOL("status");
Persistent<String> SVN::data_symbol = NODE_PSYMBOL("data");

//errors symbol
Persistent<String> SVN::description_symbol = NODE_PSYMBOL("description");
Persistent<String> SVN::message_symbol = NODE_PSYMBOL("message");
Persistent<String> SVN::error_symbol = NODE_PSYMBOL("error");

extern "C" void init(Handle<Object> target)
{
	HandleScope scope;

	SVN::InitModule(target);
}
