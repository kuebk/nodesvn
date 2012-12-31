#include "svn.h"

Handle<Value> SVN::__checkout (const Arguments &args)
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
        case 2: {
            if (!args[1]->IsString()) {
                ERROR("path has to be a string");
            }
            String::Utf8Value jsPath (args[1]->ToString());
            path = apr_pstrdup(subpool, *jsPath);
        }
        case 1: {
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

    if ( (err = svn_client_checkout3(&result_rev, tmp_url, path, &peg_revision, &revision, options.depth, options.ignore_externals, options.force, svn->ctx, subpool)) ) {
        result = Local<Object>::New(svn->error(err));
    } else {
        result = Object::New();
        result->Set(status_symbol, Integer::New(0));
        result->Set(revision_symbol, Integer::New((int) result_rev));
    }

    POOL_DESTROY

    return scope.Close(result);
}

