#include "svn.h"

Handle<Value> SVN::__upgrade (const Arguments &args) {
    HandleScope scope;

    //system
    SVN *svn = ObjectWrap::Unwrap<SVN>(args.This());
    apr_pool_t *subpool = svn_pool_create(svn->pool);
    svn_error_t *err;

    //svn
    const char *path;

    //additional
    Local<Object> result;

    if (!args[0]->IsString()) {
        ERROR("path has to be a string");
    }

    String::Utf8Value jsPath (args[0]->ToString());
    path = apr_pstrdup(subpool, *jsPath);

    if ( (err = svn_client_upgrade(path, svn->ctx, subpool)) ) {
        result = Local<Object>::New(svn->error(err));
    } else {
        result = Object::New();
        result->Set(status_symbol, Integer::New(0));
    }

    POOL_DESTROY

    return scope.Close(result);
}
