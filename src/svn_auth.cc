#include "svn.h"

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

