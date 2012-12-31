#ifndef SVN_COMMIT_H
#define SVN_COMMIT_H

svn_error_t *__commit_log (const char **log_msg, const char **tmp_file, const apr_array_header_t *commit_items, void *baton, apr_pool_t *pool);
svn_error_t *__commit_callback (const svn_commit_info_t *commit_info, void *baton, apr_pool_t *pool);

#endif
