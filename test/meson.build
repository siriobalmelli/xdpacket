tests = [
  'field_test.c',
  'overflow_test.c'
]

foreach t : tests
	name = t.split('.')[0]
	name_spaced = ' '.join(name.split('_'))

	a_test = executable(name,
			[t] + src_files,
			dependencies : deps,
			include_directories : inc)
	test(name_spaced, a_test, timeout : 45)
endforeach
