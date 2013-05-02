{
    "variables": {
        "svn_lib%": "<(module_root_dir)/build/subversion/lib",
        "neon_lib%": "<(module_root_dir)/build/neon/lib"
    },
    "targets": [
        {
            "target_name": "nodesvn",
            "dependencies": ["libsvn"],
            "sources": [
                "src/svn.cc",
                "src/svn_cat.cc",
                "src/svn_auth.cc",
                "src/svn_checkout.cc",
                "src/svn_commit.cc",
                "src/svn_options.cc",
                "src/svn_update.cc",
                "src/svn_upgrade.cc"
            ],
            "include_dirs": [
                "<(module_root_dir)/build/subversion/include",
                "<(module_root_dir)/build/subversion/include/subversion-1",
                "<(module_root_dir)/build/subversion/include/apr-1",
                "<(module_root_dir)/build/neon/include"
            ],
            "cflags": ["-g", "-Wall"],
            "defines":["FILE_OFFSET_BITS=64", "LARGEFILE_SOURCE"],
            "libraries": [
                "<(svn_lib)/libsvn_client-1.a",
                "<(svn_lib)/libsvn_wc-1.a",
                "<(svn_lib)/libsvn_ra-1.a",
                "<(svn_lib)/libsvn_ra_svn-1.a",
                "<(svn_lib)/libsvn_ra_local-1.a",
                "<(svn_lib)/libsvn_ra_serf-1.a",
                "<(svn_lib)/libsvn_subr-1.a",
                "<(svn_lib)/libsvn_delta-1.a",
                "<(svn_lib)/libsvn_ra_neon-1.a",
                "<(svn_lib)/libsvn_repos-1.a",
                "<(svn_lib)/libsvn_fs-1.a",
                "<(svn_lib)/libsvn_fs_fs-1.a",
                "<(svn_lib)/libserf-0.a",
                "<(svn_lib)/libaprutil-1.a",
                "<(svn_lib)/libapr-1.a",
                "-lexpat",
                "<(neon_lib)/libneon.a",
                "-luuid"
            ]
        },
        {
            "target_name": "libsvn",
            "type": "none",
            "dependencies": ["libneon"],
            "actions": [{
                "action_name": "build_libsvn",
                "inputs": [""],
                "outputs": [""],
                "action": ["sh", "build_libsvn.sh"]
            }]
        },
        {
            "target_name": "libneon",
            "type": "none",
            "actions": [{
                "action_name": "build_libneon",
                "inputs": [""],
                "outputs": [""],
                "action": ["sh", "build_libneon.sh"]
            }]
        }
    ]
}
