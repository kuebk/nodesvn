#include "svn.h"

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

