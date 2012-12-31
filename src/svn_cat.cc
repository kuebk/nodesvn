#include "svn.h"

Handle<Value> SVN::__cat (const Arguments &args) {
    HandleScope scope;
    Local<String> url;
    SVN *svn = ObjectWrap::Unwrap<SVN>(args.This());
    svn_opt_revision_t revision;

    switch (args.Length()) {
        case 2: {
            if (args[1]->IsNumber()) {
                revision.value.number = args[1]->ToNumber()->Value();
                revision.kind = svn_opt_revision_number;
            } else {
                revision.kind = svn_opt_revision_head;
            }
        }
        case 1: {
            if (args[0]->IsString()) {
                url = args[0]->ToString();
            } else {
                return ThrowException(Exception::Error(
                    String::New("Your URL path must be a string")
                ));
            }
            revision.kind = svn_opt_revision_head;
        }
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

