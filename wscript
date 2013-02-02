test_paths = ['/usr/', '/usr/local/', '/opt/local/']

def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  conf.check(lib='svn_client-1', libpath=build_paths('lib'))
  conf.check(lib='svn_wc-1', libpath=build_paths('lib'))
  conf.check(lib='svn_ra-1', libpath=build_paths('lib'))
  conf.check(lib='svn_ra_svn-1', libpath=build_paths('lib'))
  conf.check(lib='svn_ra_local-1', libpath=build_paths('lib'))
  conf.check(lib='svn_ra_serf-1', libpath=build_paths('lib'))
  conf.check(lib='svn_subr-1', libpath=build_paths('lib'))
  conf.check(lib='svn_delta-1', libpath=build_paths('lib'))
  conf.check(lib='svn_ra_neon-1', libpath=build_paths('lib'))
  conf.check(lib='svn_repos-1', libpath=build_paths('lib'))
  conf.check(lib='svn_fs-1', libpath=build_paths('lib'))
  conf.check(lib='svn_fs_fs-1', libpath=build_paths('lib'))
  conf.check(lib='serf-1', libpath=build_paths('lib'))
  conf.check(lib='aprutil-1', libpath=build_paths('lib'));
  conf.check(lib='apr-1', libpath=build_paths('lib'))
  conf.check(lib='expat', libpath=build_paths('lib'))
  conf.check(lib='neon', libpath=build_paths('lib'))
  conf.check(lib='uuid', libpath=build_paths('lib'))


def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall"]
  obj.lib = ['svn_client-1', 'svn_wc-1', 'svn_ra-1', 'svn_ra_svn-1', 'svn_ra_local-1', 'svn_ra_serf-1', 'svn_subr-1', 'svn_delta-1', 'svn_ra_neon-1', 'svn_repos-1', 'svn_fs-1', 'svn_fs_fs-1', 'serf-1', 'aprutil-1', 'apr-1', 'expat', 'neon', 'uuid']
  obj.libpath = build_paths('lib') ;
  obj.target = "nodesvn"
  obj.source = [ "src/svn.cc", "src/svn_cat.cc", "src/svn_auth.cc", "src/svn_checkout.cc", "src/svn_commit.cc", "src/svn_options.cc", "src/svn_update.cc", "src/svn_upgrade.cc" ]
  obj.includes = build_paths('include') + build_paths('include/subversion-1') + build_paths('include/apr-1/') + build_paths('include/apr-1.0')

def all(bld):
  configure(bld);
  all(bld);

def build_paths(type):
  def normal(path):
    if path[-1] == '/':
      return path[:-1];

  return map(lambda path: normal(path)+'/'+type, test_paths[:]);
