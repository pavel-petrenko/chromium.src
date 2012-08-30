{
  'TOOLS': ['newlib', 'glibc', 'win', 'linux'],
  'TARGETS': [
    {
      'NAME' : 'gtest_ppapi',
      'TYPE' : 'lib',
      'SOURCES' : [
        "gtest_event_listener.cc",
        "gtest_instance.cc",
        "gtest_module.cc",
        "gtest_nacl_environment.cc",
        "gtest_runner.cc",
      ],
    }
  ],
  'HEADERS': [
    {
      'FILES': [
        "gtest_event_listener.h",
        "gtest_instance.h",
        "gtest_nacl_environment.h",
        "gtest_runner.h",
        "thread_condition.h",
      ],
      'DEST': 'include/gtest_ppapi',
    },
  ],
  'DEST': 'testing',
  'NAME': 'gtest_ppapi',
}
