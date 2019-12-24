tests = [
  'field_test.c',
  'overflow_test.c',
  'value_test.c'
  ]

foreach t : tests
  name = t.split('.')[0]
  name_format = name.split('_')[0]

  a_test = executable(name,
    [t] + src_files,
    include_directories : inc,
    dependencies        : deps
    )
  # the 'core' test suite is what gets checked with e.g. Valgrind
  test(name_format,
    a_test,
    suite   : 'core',
    timeout : 45
    )
endforeach
