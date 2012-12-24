def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")


def build(bld):
  bld.exec_command("mkdir -p subversion && cd ../deps/subversion && ./configure --with-pic --disable-shared --without-shared --enable-debug --prefix=%s && make clean install" % (bld.bdir + "/subversion"))
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall"]
  obj.target = "nodesvn"
  obj.source = [ "src/svn.cc", "src/svn_cat.cc", "src/svn_auth.cc", "src/svn_checkout.cc", "src/svn_commit.cc", "src/svn_options.cc", "src/svn_update.cc", "src/svn_upgrade.cc" ]
  obj.includes = [bld.bdir + "/subversion/include", bld.bdir + "/subversion/include/subversion-1", bld.bdir + "/subversion/include/apr-1"]
  obj.libpath = [bld.bdir + "/subversion/lib"]
  obj.lib = ['svn_client-1', 'svn_wc-1', 'svn_ra-1', 'svn_ra_svn-1', 'svn_ra_local-1', 'svn_ra_serf-1', 'svn_subr-1', 'svn_delta-1', 'svn_ra_neon-1', 'svn_repos-1', 'svn_fs-1', 'svn_fs_fs-1', 'serf-0', 'aprutil-1', 'apr-1', 'expat', 'neon', 'uuid']
