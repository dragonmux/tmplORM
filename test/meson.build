crunchpp = find_program('crunch++')
libcrunchpp = dependency('crunch++')
subdir('data')

tmplORMTests = [
	'testString', #'testMySQL', 'testMSSQL', 'testMySQLMapper', 'testMSSQLMapper',
	'testConversions', 'testFixedVector', 'testTypes', 'testDateTime'
]

if coverage
	crunchCoverage = ['--coverage']
else
	crunchCoverage = []
endif

sanitizer = get_option('b_sanitize')
if sanitizer != 'none'
	crunchCoverage += ['-fsanitize=' + sanitizer]
endif

testHelpers = static_library(
	'testHelpers',
	['conversions.cxx', 'fixedVector.cxx', 'tmplORM.types.cxx'],
	pic: true,
	dependencies: [libcrunchpp],
	include_directories: include_directories('..'),
	install: false,
	build_by_default: true
)

objectMap = {
	'testString': {'tmplORM': ['string.cxx']},
	'testMySQL': {'tmplORM': ['mysql.cxx', 'string.cxx', 'tmplORM.types.cxx']},
	'testMSSQL': {'tmplORM': ['mssql.cxx', 'string.cxx', 'tmplORM.types.cxx'], 'libs': [odbc]},
	'testConversions': {'test': ['conversions.cxx']},
	'testFixedVector': {'test': ['fixedVector.cxx']},
	'testTypes': {'tmplORM': ['string.cxx', 'tmplORM.types.cxx'], 'test': ['tmplORM.types.cxx']},
	'testDateTime': {'tmplORM': ['string.cxx', 'tmplORM.types.cxx']}
}

foreach test : tmplORMTests
	map = objectMap.get(test, {})
	tmplORMObjs = map.has_key('tmplORM') ? [tmplORM.extract_objects(map['tmplORM'])] : []
	testObjs = map.has_key('test') ? [testHelpers.extract_objects(map['test'])] : []
	testLibs = map.get('libs', [])
	custom_target(
		test,
		command: [
			crunchMake, '-s', '-z', 'defs', '@INPUT@', '-o', '@OUTPUT@', '-I' + tmplORMSrcDir
		] + crunchCoverage + testLibs,
		input: [test + '.cxx'] + tmplORMObjs + testObjs,
		output: test + '.so',
		build_by_default: true
	)

	test(
		test,
		crunchpp,
		args: [test],
		workdir: meson.current_build_dir()
	)
endforeach
