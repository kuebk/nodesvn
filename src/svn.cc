#include "svn.h"

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

options_t SVN::default_options()
{
    options_t options;

    options.revision_start.kind = svn_opt_revision_unspecified;
    options.revision_end.kind = svn_opt_revision_unspecified;
    options.depth = svn_depth_unknown;
    options.set_depth = svn_depth_unknown;
    options.ignore_externals = false;
    options.force = false;
    options.parents = false;

    return options;
};

options_t SVN::parse_options(const Handle<Value> opt_arg, options_interest opt_intrst, apr_pool_t *pool, bool *has_err, const char **err_msg)
{
    options_t options = this->default_options();
    Local<Object> opts;

    if (!opt_arg->IsObject()) {
        OPTIONS_EXCEPTION("options has to be an object");
    }

    opts = opt_arg->ToObject();

    if (opt_intrst & kRevision && opts->Has(this->revision_symbol)) {
        Local<Value> revision = opts->Get(this->revision_symbol);

        if (!revision->IsNumber() && !revision->IsString()) {
            OPTIONS_EXCEPTION("revision has to be a string or a number");
        }

        String::Utf8Value jsRevision (revision->ToString());

        if (svn_opt_parse_revision(&options.revision_start, &options.revision_end, *jsRevision, pool) != 0) {
            OPTIONS_EXCEPTION("syntax error in given revision");
        }
    }

    if (opt_intrst & kForce && opts->Has(this->force_symbol)) {
        Local<Value> force = opts->Get(this->force_symbol);

        if (!force->IsBoolean()) {
            OPTIONS_EXCEPTION("force has to be a boolean");
        }

        options.force = force->BooleanValue();
    }

    if (opt_intrst & kRecursive && opts->Has(this->recursive_symbol)) {
        Local<Value> recursive = opts->Get(this->recursive_symbol);

        if (!recursive->IsBoolean()) {
            OPTIONS_EXCEPTION("recursive has to be a boolean");
        }

        if (recursive->BooleanValue()) {
            options.depth = svn_depth_files;
        }
    }

    if (opt_intrst & kDepth && opts->Has(this->depth_symbol)) {
        Local<Value> depth = opts->Get(this->depth_symbol);

        if (!depth->IsNumber() || !depth->IsString()) {
            OPTIONS_EXCEPTION("depth has to be a string or a number");
        }

        //veryfication needed
        if (depth->IsNumber()) {
            options.depth = static_cast<svn_depth_t>((int) depth->IntegerValue());
        }

        if (depth->IsString()) {
            String::Utf8Value s_depth (depth->ToString());
            options.depth = svn_depth_from_word(*s_depth);
        }

        if (options.depth == svn_depth_unknown || options.depth == svn_depth_exclude) {
            OPTIONS_EXCEPTION("depth can be one of: empty, files, immediates or infinity");
        }
    }

    if (opt_intrst & kSetDepth && opts->Has(this->set_depth_symbol)) {
        Local<Value> set_depth = opts->Get(this->set_depth_symbol);

        if (!set_depth->IsNumber() || !set_depth->IsString()) {
            OPTIONS_EXCEPTION("depth has to be a string or a number");
        }

        //veryfication needed
        if (set_depth->IsNumber()) {
            options.set_depth = static_cast<svn_depth_t>((int) set_depth->IntegerValue());
        }

        if (set_depth->IsString()) {
            String::Utf8Value s_set_depth (set_depth->ToString());
            options.set_depth = svn_depth_from_word(*s_set_depth);
        }

        if (options.set_depth == svn_depth_unknown) {
            OPTIONS_EXCEPTION("depth can be one of: exclude, empty, files, immediates or infinity");
        }
    }
    if (opt_intrst & kIgnoreExternals && opts->Has(this->externals_symbol)) {
        Local<Value> externals = opts->Get(this->externals_symbol);

        if (!externals->IsBoolean()) {
           OPTIONS_EXCEPTION("ignore_externals has to be a boolean");
        }

        options.ignore_externals = externals->BooleanValue();
    }

    return options;
}

Handle<Value> SVN::__checkout(const Arguments &args)
{
	HandleScope scope;

    //system
    SVN *svn = ObjectWrap::Unwrap<SVN>(args.This());
    apr_pool_t *subpool = svn_pool_create(svn->pool);
    svn_error_t *err;

    //svn
    svn_revnum_t result_rev;
    const char *url; //holds link@rev
    const char *tmp_url; //holds only the link to repo
    const char *path = 0;
    svn_opt_revision_t peg_revision;
    svn_opt_revision_t revision;

    //additional
    options_t options;
    bool options_err;
    const char *options_err_msg;
    Local<Object> result;

    if (args.Length() == 3) {
        options = svn->parse_options(args[2], kCheckout, subpool, &options_err, &options_err_msg);
    } else {
        options = svn->default_options();
    }

    switch (args.Length()) {
        case 3:
        case 2:
        {
            if (!args[1]->IsString()) {
                ERROR("path has to be a string");
            }
            String::Utf8Value jsPath (args[1]->ToString());
            path = apr_pstrdup(subpool, *jsPath);
        }
        case 1:
        {
            if (!args[0]->IsString()) {
                ERROR("url has to be a string");
            }
            String::Utf8Value jsUrl (args[0]->ToString());
            url = apr_pstrdup(subpool, *jsUrl);

            if (!svn_path_is_url(url)) {
                ERROR("given url is not valid");
            }
        }
        break;
        default:
            ERROR("Expected: url[,path,options]");
    }

    if ( (err = svn_opt_parse_path(&peg_revision, &tmp_url, url, subpool)) ) {
        ERROR("error parsing given url");
    } 

    revision = options.revision_start;

    if (revision.kind == svn_opt_revision_unspecified) {
        if (peg_revision.kind != svn_opt_revision_unspecified) {
            revision = peg_revision;
        } else {
            revision.kind = svn_opt_revision_head;
        }
    }
    //if no target path is specified we creating it (basing on given url)
    if (!path) {
        path = svn_uri_basename(tmp_url, subpool);
    }

    if ( (err = svn_client_checkout3(&result_rev, tmp_url, path, &peg_revision, &revision, options.depth, options.ignore_externals, options.force, svn->ctx, subpool)) )
    {
		result = Local<Object>::New(svn->error(err));
    } else {
        result = Object::New();
        result->Set(status_symbol, Integer::New(0));
        result->Set(revision_symbol, Integer::New((int) result_rev));
    }
		
    svn_pool_destroy(subpool);
	subpool = NULL;

    return scope.Close(result);
}

void __notify_callback(void *baton, const svn_wc_notify_t *notify, apr_pool_t *pool)
{
    notify_item *item = (notify_item *) malloc(sizeof(notify_item));
    item->path = notify->path;
    item->action = notify->content_state;
    item->next = NULL;

    notify_list *list = (notify_list *) baton;

    if (!list->first && !list->last) {
        list->first = item;
    } else {
        list->last->next = item;
    }

//    fprintf(stderr, "%s\n", item->path);
//    fprintf(stderr, "%d\n", item->action);

    list->last = item;
}

Handle<Value> SVN::__update(const Arguments &args)
{
    HandleScope scope;

    //system
    SVN *svn = ObjectWrap::Unwrap<SVN>(args.This());
    apr_pool_t *subpool = svn_pool_create(svn->pool);
    svn_error_t *err;

    //svn
    apr_array_header_t *result_revs;
    apr_array_header_t *targets = apr_array_make(subpool, 1, sizeof(const char*));
    svn_depth_t depth;
    svn_boolean_t depth_is_sticky;

    //additional
    options_t options;
    bool options_err;
    const char *options_err_msg;
    Local<Object> result;

    if (args.Length() == 2) {
        options = svn->parse_options(args[1], kUpdate, subpool, &options_err, &options_err_msg);
    } else {
        options = svn->default_options();
    }

    switch (args.Length()) {
        case 2:
        case 1:
            if (args[0]->IsString()) {
                String::Utf8Value path (args[0]->ToString());
                *(const char **)apr_array_push(targets) = apr_pstrdup(subpool, *path);
            }
            //add support for array of strings
        break;
    }

    svn_opt_push_implicit_dot_target(targets, subpool);

    if (options.set_depth != svn_depth_unknown) {
        depth = options.set_depth;
        depth_is_sticky =  true;
    } else {
        depth = options.depth;
        depth_is_sticky = false;
    }

    notify_list *list = (notify_list *) malloc(sizeof(notify_list));
    memset(list, 0, sizeof(notify_list));

    svn->ctx->notify_func2 = __notify_callback;
    svn->ctx->notify_baton2 = list;

    if ( (err = svn_client_update4(&result_revs, targets, &(options.revision_start), depth, depth_is_sticky, options.ignore_externals, options.force, true, options.parents, svn->ctx, subpool)) )
    {
		result = Local<Object>::New(svn->error(err));
    } else {
        result = Object::New();
        result->Set(status_symbol, Integer::New(0));
        Local<Object> data = Object::New();

        notify_item *item = list->first;

        while (item) {
            Local<Array> arr;
            Local<String> key = String::New(item->path);

            if (!data->Has(key)) {
                arr = Array::New();
                data->Set(key, arr);
            } else {
                arr = data->Get(key).As<Array>();
            }

            arr->Set(arr->Length(), Integer::New(item->action));

            item = item->next;
        }

        result->Set(data_symbol, data);
    }

    return scope.Close(result);
}


svn_error_t* __commit_log(const char **log_msg, const char **tmp_file, const apr_array_header_t *commit_items, void *baton, apr_pool_t *pool)
{
    *log_msg = (const char *) baton;
    return SVN_NO_ERROR;
    //return svn_utf_cstring_to_utf8(log_msg, message, pool);
}

svn_error_t* __commit_callback(const svn_commit_info_t *commit_info, void *baton, apr_pool_t *pool)
{
//    fprintf(stderr, "%d", (int)commit_info->revision);
    return SVN_NO_ERROR;
}

Handle<Value> SVN::__commit(const Arguments &args)
{
    HandleScope scope;

    SVN *svn = ObjectWrap::Unwrap<SVN>(args.This());
    apr_pool_t *subpool = svn_pool_create(svn->pool);
    svn_error_t *err;

    String::Utf8Value path (args[0]->ToString());
    String::Utf8Value msg (args[1]->ToString());
    apr_array_header_t *targets = apr_array_make(subpool, 1, sizeof(const char*));
    *(const char **)apr_array_push(targets) = *path;

    svn->ctx->log_msg_func3 = __commit_log;
    svn->ctx->log_msg_baton3 = *msg;

    if ( (err = svn_client_commit5(targets, svn_depth_infinity, 0, 0, 0, NULL, NULL, __commit_callback, NULL, svn->ctx, subpool)) )
    {
		svn_pool_destroy(subpool);
		subpool = NULL;
		return svn->error(err);
    }

    Local<Object> obj = Object::New();
    obj->Set(status_symbol, Integer::New(0));

    return scope.Close(obj);
}

Handle<Value> SVN::__cat(const Arguments &args)
{
	HandleScope scope;
	Local<String> url;
	SVN *svn = ObjectWrap::Unwrap<SVN>(args.This());
	svn_opt_revision_t revision;

	switch(args.Length())
	{
		case 2:
			if(args[1]->IsNumber())
			{
				revision.value.number = args[1]->ToNumber()->Value();
				revision.kind = svn_opt_revision_number;
			} else
			{
				revision.kind = svn_opt_revision_head;
			}
		
		case 1:
			if(args[0]->IsString())
				url = args[0]->ToString();
			else
				return ThrowException(Exception::Error(
					String::New("Your URL path must be a string")
				));
			revision.kind = svn_opt_revision_head;
		break;
		default:
			return ThrowException(Exception::Error(
				String::New("Expected: url[,revision]")
			));
	}

	return scope.Close(svn->do_cat(url, revision));
}

Handle<Value> SVN::do_cat(const Handle<String> url, svn_opt_revision_t revision)
{
	HandleScope scope;

	apr_pool_t *subpool = svn_pool_create(this->pool);

	String::Utf8Value url_utf(url);
	svn_error_t *err;
	svn_opt_revision_t peg_revision = { svn_opt_revision_unspecified };
	
	// pool and buffers setup
	svn_stringbuf_t *buf = svn_stringbuf_create("", subpool);
	svn_stream_t *out = svn_stream_from_stringbuf(buf, subpool);

	if( (err = svn_client_cat2(out, *url_utf, &peg_revision, &revision, this->ctx, subpool)) )
	{
		svn_pool_destroy(subpool);
		subpool = NULL;
		return this->error(err);
	}
	return scope.Close(String::New(buf->data, buf->len));
}

// We currently only push the "simple provider" for basic user/pass
// authentication. Others may follow.
Handle<Value> SVN::__authenticate(const Arguments &args)
{
	HandleScope scope;
	SVN *svn = ObjectWrap::Unwrap<SVN>(args.This());

	// If we have a username and password passed in, setting those
	// to the SVN_AUTH_PARAM_DEFAULT_USERNAME and SVN_AUTH_PARAM_DEFAULT_PASSWORD
	// will allow us to use the "simple providers" from Subversion.
	if(args.Length() == 2 && (args[0]->IsString() && args[1]->IsString()))
	{
		String::Utf8Value username(args[0]->ToString());
		String::Utf8Value password(args[1]->ToString());
		svn->simple_authentication(*username, *password);
		return scope.Close(Boolean::New(true));
	}

	return scope.Close(Boolean::New(false));
}

void SVN::init_auth()
{
	svn_auth_provider_object_t *_provider;
	svn_auth_baton_t *auth_baton;

	apr_array_header_t *providers = apr_array_make(this->pool, 10, sizeof(svn_auth_provider_object_t *));

#ifdef DARWIN
	svn_auth_get_keychain_simple_provider(&_provider, this->pool);
	*(svn_auth_provider_object_t **)apr_array_push(providers) = _provider;
#endif

	svn_auth_get_simple_provider2(&_provider, NULL, NULL, this->pool);
	*(svn_auth_provider_object_t **)apr_array_push(providers) = _provider;

	svn_auth_get_username_provider(&_provider, this->pool);
	*(svn_auth_provider_object_t **)apr_array_push(providers) = _provider;

	svn_auth_open(&auth_baton, providers, this->pool);
	
	svn_auth_set_parameter(auth_baton, SVN_AUTH_PARAM_NON_INTERACTIVE, "");
	this->ctx->auth_baton = auth_baton;
}


void SVN::simple_authentication(const char *username, const char *password)
{
	svn_auth_set_parameter(this->ctx->auth_baton, 
				apr_pstrdup(this->pool, SVN_AUTH_PARAM_DEFAULT_USERNAME),
				apr_pstrdup(this->pool, username));
	svn_auth_set_parameter(this->ctx->auth_baton,
				apr_pstrdup(this->pool, SVN_AUTH_PARAM_DEFAULT_PASSWORD),
				apr_pstrdup(this->pool, password));
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
