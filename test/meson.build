tests = [
	'field_test.c',
]

foreach t : tests
	name = t.split('.')[0]
	name_spaced = ' '.join(name.split('_'))

	a_test = executable(name + '_shared', t,
			include_directories : inc)
	test(name_spaced, a_test, timeout : 45)
endforeach
