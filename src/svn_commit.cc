#include "svn.h"
#include "svn_commit.h"

svn_error_t *__commit_log (const char **log_msg, const char **tmp_file, const apr_array_header_t *commit_items, void *baton, apr_pool_t *pool) {
    *log_msg = (const char *) baton;
    return SVN_NO_ERROR;
}

svn_error_t *__commit_callback (const svn_commit_info_t *commit_info, void *baton, apr_pool_t *pool) {
    return SVN_NO_ERROR;
}

Handle<Value> SVN::__commit (const Arguments &args) {
    HandleScope scope;

    SVN *svn = ObjectWrap::Unwrap<SVN>(args.This());
    apr_pool_t *subpool = svn_pool_create(svn->pool);
    svn_error_t *err;
    Local<Object> result;

    String::Utf8Value path (args[0]->ToString());
    String::Utf8Value msg (args[1]->ToString());
    apr_array_header_t *targets = apr_array_make(subpool, 1, sizeof(const char*));
    *(const char **)apr_array_push(targets) = *path;

    svn->ctx->log_msg_func3 = __commit_log;
    svn->ctx->log_msg_baton3 = *msg;

    if ( (err = svn_client_commit5(targets, svn_depth_infinity, 0, 0, 0, NULL, NULL, __commit_callback, NULL, svn->ctx, subpool)) ) {
        result = Local<Object>::New(svn->error(err));
    } else {
        result = Object::New();
        result->Set(status_symbol, Integer::New(0));
    }

    POOL_DESTROY

    return scope.Close(result);
}

