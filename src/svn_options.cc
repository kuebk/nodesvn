#include "svn.h"

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

