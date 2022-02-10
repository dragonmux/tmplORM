crunchpp = find_program('crunch++')
libcrunchpp = dependency('crunch++')
subdir('data')

tmplORMTests = [
	'testString', 'testMySQL', 'testMSSQL', 'testPgSQL',
	'testMySQLMapper', 'testMSSQLMapper', 'testTypes', 'testDateTime'
]

crunchExtra = [
	'-I@0@'.format(tmplORMSrcDir)
]
if coverage
	crunchExtra += ['--coverage']
endif
if get_option('debug')
	crunchExtra += ['--debug']
endif
if cxx.get_id() != 'msvc'
	crunchExtra += ['-z', 'defs']
endif
if get_option('b_lto')
	crunchExtra += ['-flto']
endif

sanitizer = get_option('b_sanitize')
if sanitizer != 'none'
	crunchExtra += ['-fsanitize=' + sanitizer]
endif

if substrate.type_name() == 'internal'
	crunchExtra += ['-I@0@/deps/substrate'.format(meson.project_source_root())]
endif

testIncludes = []
foreach include : includes + extraIncludes
	if include.startswith('/')
		testIncludes += ['-I@0@'.format(include)]
	else
		testIncludes += ['-I@0@/@1@'.format(meson.project_source_root(), include)]
	endif
endforeach

pkgConfig = find_program('pkg-config')
pgsqlCflags = run_command([pkgConfig, '--cflags', 'libpq'], check: true).stdout().strip()
pgsqlLflags = run_command([pkgConfig, '--libs', 'libpq'], check: true).stdout().strip()

if pgsqlCflags == ''
	pgsqlCflags = []
else
	pgsqlCflags = pgsqlCflags.split()
endif

if pgsqlLflags == ''
	pgsqlLflags = []
else
	pgsqlLflags = pgsqlLflags.split()
endif

testHelpers = static_library(
	'testHelpers',
	['tmplORM.types.cxx'],
	pic: true,
	dependencies: [libcrunchpp, substrate],
	include_directories: include_directories('..'),
	install: false,
	build_by_default: true
)

objectMap = {
	'testString': {'tmplORM': ['string.cxx']},
	'testMySQL': {
		'tmplORM': ['mysql.cxx', 'string.cxx', 'tmplORM.types.cxx'],
		'libs': [mysqlCflags, mysqlLflags]
	},
	'testMySQLMapper': {'libs': [mysqlCflags, mysqlLflags]},
	'testMSSQL': {
		'tmplORM': ['mssql.cxx', 'string.cxx', 'tmplORM.types.cxx'],
		'libs': ['-L@0@'.format(odbc.get_variable(pkgconfig: 'libdir')), '-lodbc']
	},
	'testPgSQL': {
		'tmplORM': ['pgsql.cxx', 'tmplORM.types.cxx'],
		'libs': pgsqlCflags + pgsqlLflags + testIncludes
	},
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
			crunchMake, '-s', '-z', 'defs', '@INPUT@', '-o', '@OUTPUT@'
		] + crunchExtra + testLibs,
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
