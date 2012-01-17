#include "svn.h"

#define EXCEPTION(message) \
    return ThrowException(Exception::Error(String::New(message)))

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
    NODE_DEFINE_CONSTANT(target, svn_depth_unknown);
    NODE_DEFINE_CONSTANT(target, svn_depth_exclude);
    NODE_DEFINE_CONSTANT(target, svn_depth_empty);
    NODE_DEFINE_CONSTANT(target, svn_depth_files);
    NODE_DEFINE_CONSTANT(target, svn_depth_immediates);
    NODE_DEFINE_CONSTANT(target, svn_depth_infinity);

	target->Set(String::NewSymbol("client"), ct->GetFunction());
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

options_t SVN::parse_options(Handle<Object> opts, apr_pool_t *pool)
{
    options_t options;

    return options;
}

Handle<Value> SVN::__checkout(const Arguments &args)
{
	HandleScope scope;

    SVN *svn = ObjectWrap::Unwrap<SVN>(args.This());
    apr_pool_t *subpool = svn_pool_create(svn->pool);
    svn_error_t *err;

    Local<Object> options;

    svn_revnum_t result_rev;
    const char *url; //holds link@rev
    const char *tmp_url; //holds only the link to repo
    const char *path;
    svn_opt_revision_t peg_revision;
    svn_opt_revision_t revision;
    svn_depth_t depth = svn_depth_unknown;
    svn_boolean_t ignore_externals = false;
    svn_boolean_t allow_unver_obstructions = false;

    svn_opt_revision_range_t *range = (svn_opt_revision_range_t *) apr_palloc(subpool, sizeof(*range));
    range->start.kind = svn_opt_revision_unspecified;
    range->end.kind = svn_opt_revision_unspecified;
    apr_array_header_t *opt_ranges = apr_array_make(subpool, 1, sizeof(*range));
    APR_ARRAY_PUSH(opt_ranges, svn_opt_revision_range_t*) = range;

    switch (args.Length()) {
        case 3:
        {
            if (!args[2]->IsObject()) {
                EXCEPTION("options has to be an object");
            }

            options = args[2]->ToObject();

            if (options->Has(svn->revision_symbol)) {
                Local<Value> revision = options->Get(svn->revision_symbol);

                if (!revision->IsNumber() || !revision->IsString()) {
                    EXCEPTION("revision has to be a string or a number");
                }

                String::Utf8Value jsRevision (revision->ToString());

                if (svn_opt_parse_revision_to_range(opt_ranges, *jsRevision, subpool) != 0) {
                    EXCEPTION("syntax error in given revision");
                }
            }

            if (options->Has(svn->force_symbol)) {
                Local<Value> force = options->Get(svn->force_symbol);

                if (!force->IsBoolean()) {
                    EXCEPTION("force has to be a boolean");
                }

                allow_unver_obstructions = force->BooleanValue();
            }

            if (options->Has(svn->recursive_symbol)) {
                Local<Value> recursive = options->Get(svn->recursive_symbol);

                if (!recursive->IsBoolean()) {
                    EXCEPTION("recursive has to be a boolean");
                }

                if (recursive->BooleanValue()) {
                    depth = svn_depth_files;
                }
            }

            if (options->Has(svn->depth_symbol)) {
                Local<Value> o_depth = options->Get(svn->depth_symbol);

                if (!o_depth->IsNumber() || !o_depth->IsString()) {
                    EXCEPTION("depth has to be a string or a number");
                }

                //veryfication needed
                if (o_depth->IsNumber()) {
                    int i_depth = (int) Local<Integer>::Cast(o_depth)->Int32Value();
                    depth = static_cast<svn_depth_t>(i_depth);
                }

                if (o_depth->IsString()) {
                    String::Utf8Value s_depth (o_depth->ToString());
                    depth = svn_depth_from_word(*s_depth);
                }

                if (depth == svn_depth_unknown || depth == svn_depth_exclude) {
                    EXCEPTION("depth can be one of: empty, files, immediates or infinity");
                }
            }

            if (options->Has(svn->externals_symbol)) {
                Local<Value> externals = options->Get(svn->externals_symbol);

                if (!externals->IsBoolean()) {
                    EXCEPTION("ignore_externals has to be a boolean");
                }

                ignore_externals = externals->BooleanValue();
            }
        }
        case 2:
        {
            if (!args[1]->IsString()) {
                EXCEPTION("path has to be a string");
            }
            String::Utf8Value jsPath (args[1]->ToString());
            path = strdup(*jsPath);
        }
        case 1:
        {
            if (!args[0]->IsString()) {
                EXCEPTION("url has to be a string");
            }
            String::Utf8Value jsUrl (args[0]->ToString());
            url = strdup(*jsUrl);

            if (!svn_path_is_url(url)) {
                EXCEPTION("given url is not valid");
            }
        }
        break;
        default:
            EXCEPTION("Expected: url[,path]");
    }

    if ( (err = svn_opt_parse_path(&peg_revision, &tmp_url, url, subpool)) ) {
        EXCEPTION("error parsing given url");
    } 

    revision = APR_ARRAY_IDX(opt_ranges, 0, svn_opt_revision_range_t *)->start;

    if (revision.kind == svn_opt_revision_unspecified) {
        if (peg_revision.kind != svn_opt_revision_unspecified) {
            revision = peg_revision;
        } else {
            revision.kind = svn_opt_revision_head;
        }
    }
    //if no target path is specified we creating it (basing on given url)
    if (strlen(path) == 0) {
        path = svn_uri_basename(tmp_url, subpool);
    }

    if ( (err = svn_client_checkout3(&result_rev, tmp_url, path, &peg_revision, &revision, depth, ignore_externals, allow_unver_obstructions, svn->ctx, subpool)) )
    {
		svn_pool_destroy(subpool);
		subpool = NULL;
		return ThrowException(Exception::Error(
		    svn->error(err)
		));
    }

    return scope.Close(Integer::New((int)result_rev));
}

Handle<Value> SVN::__update(const Arguments &args)
{
    HandleScope scope;

    SVN *svn = ObjectWrap::Unwrap<SVN>(args.This());
    apr_pool_t *subpool = svn_pool_create(svn->pool);
    svn_error_t *err;

    apr_array_header_t *result_revs;

    String::Utf8Value path (args[0]->ToString());
    apr_array_header_t *paths = apr_array_make(subpool, 1, sizeof(const char*));
    *(const char **)apr_array_push(paths) = *path;

    svn_opt_revision_t revision;
    revision.kind = svn_opt_revision_head;

    if ( (err = svn_client_update4(&result_revs, paths, &revision, svn_depth_infinity, 0, 0, 1, 0, 1, svn->ctx, subpool)) )
    {
		svn_pool_destroy(subpool);
		subpool = NULL;
		return ThrowException(Exception::Error(
		    svn->error(err)
		));
    }

    return scope.Close(Boolean::New(true));
}

svn_error_t* __commit_log(const char **log_msg, const char **tmp_file, const apr_array_header_t *commit_items, void *bato, apr_pool_t *pool)
{
    const char *message = "Taki se commit";
    return svn_utf_cstring_to_utf8(log_msg, message, pool);
}

svn_error_t* __commit_callback(const svn_commit_info_t *commit_info, void *baton, apr_pool_t *pool)
{
    fprintf(stderr, "%d", (int)commit_info->revision);
    return SVN_NO_ERROR;
}

Handle<Value> SVN::__commit(const Arguments &args)
{
    HandleScope scope;

    SVN *svn = ObjectWrap::Unwrap<SVN>(args.This());
    apr_pool_t *subpool = svn_pool_create(svn->pool);
    svn_error_t *err;

    String::Utf8Value path (args[0]->ToString());
    apr_array_header_t *targets = apr_array_make(subpool, 1, sizeof(const char*));
    *(const char **)apr_array_push(targets) = *path;

    svn->ctx->log_msg_func3 = __commit_log;

    if ( (err = svn_client_commit5(targets, svn_depth_infinity, 0, 0, 0, NULL, NULL, __commit_callback, NULL, svn->ctx, subpool)) )
    {
		svn_pool_destroy(subpool);
		subpool = NULL;
		return ThrowException(Exception::Error(
		    svn->error(err)
		));
    }

    return scope.Close(Boolean::New(true));
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
		return ThrowException(Exception::Error(
			this->error(err)
		));
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

Handle<String> SVN::error(svn_error_t *error)
{
	HandleScope scope;
	svn_error_t *err_it = error;
	Local<String> strerror = String::New("SVN Error Occured: ");

	while( err_it )
	{
		char buffer[256];
		svn_strerror(err_it->apr_err, buffer, sizeof(buffer));
		strerror = String::Concat(strerror, String::New(" ( "));
		strerror = String::Concat(strerror, String::New(buffer));
		strerror = String::Concat(strerror, String::New(" ) "));
        if (err_it->message) {
    		strerror = String::Concat(strerror, String::New(err_it->message));
        }

		err_it = err_it->child;
		if(err_it != NULL)
		{
			strerror = String::Concat(strerror, String::New("\n"));
		}
	}
	return scope.Close(strerror);
}

Persistent<FunctionTemplate> SVN::ct;
Persistent<String> SVN::revision_symbol = NODE_PSYMBOL("revision");
Persistent<String> SVN::recursive_symbol = NODE_PSYMBOL("recursive");
Persistent<String> SVN::depth_symbol = NODE_PSYMBOL("depth");
Persistent<String> SVN::force_symbol = NODE_PSYMBOL("force");
Persistent<String> SVN::externals_symbol = NODE_PSYMBOL("ignore_externals");

extern "C" void init(Handle<Object> target)
{
	HandleScope scope;

	SVN::InitModule(target);
}
