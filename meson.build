project(
	'tmplORM',
	'cpp',
	default_options: [
		'cpp_std=c++11',
		'warning_level=3',
		'b_ndebug=if-release'
	],
	version: '0.2.0',
	meson_version: '>= 0.52'
)

cxx = meson.get_compiler('cpp')
pkgconfig = import('pkgconfig')
isWindows = target_machine.system() == 'windows'
coverage = get_option('b_coverage')

threading = dependency('threads')
dl = cxx.find_library('dl', required: not isWindows)
odbc = cxx.find_library('odbc')
rSON = dependency('rSON')
substrate = dependency('substrate')

mysqlConfig = find_program('mysql_config')
mysql = declare_dependency(
	compile_args: run_command([mysqlConfig, '--cflags']).stdout().split(),
	link_args: run_command([mysqlConfig, '--libs']).stdout().split()
)

add_project_arguments(
	cxx.get_supported_arguments(
		'-Wvla', '-Wimplicit-fallthrough'
	), language: 'cpp'
)

if cxx.get_id() == 'msvc'
	add_project_arguments(
		'-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1',
		'-D_CRT_NONSTDC_NO_WARNINGS=1',
		language: 'cpp'
	)
endif

tmplORMSrc = [
	'string.cxx', 'mysql.cxx', 'mssql.cxx', 'tmplORM.types.cxx'
]
tmplORMSrcDir = meson.current_source_dir()

tmplORM = shared_library(
	'tmplORM',
	tmplORMSrc,
	dependencies: [threading, dl, odbc, mysql, substrate],
	version: meson.project_version(),
	install: true
)

pkgconfig.generate(
	tmplORM,
	name: 'tmplORM',
	version: meson.project_version(),
	description: 'The tmplORM template meta-programming ORM\'s C++ runtime'
)

run_target(
	'buildCheck',
	command: executable(
		'_buildCheck',
		['buildCheck.cxx'],
		dependencies: [threading, dl, odbc, mysql, rSON],
		install: false,
		build_by_default: false
	)
)

install_headers(
	'mysql.hxx', 'mssql.hxx', 'tmplORM.hxx', 'tmplORM.mysql.hxx', 'tmplORM.mssql.hxx',
	'tmplORM.common.hxx', 'tmplORM.types.hxx', 'tmplORM.extern.hxx', 'conversions.hxx',
	'string.hxx',
	subdir: 'tmplORM'
)
install_headers('typestring/typestring.hh', subdir: 'typestring')

foreach header : ['json.hxx', 'fromJSON.hxx', 'toJSON.hxx', 'helpers.hxx']
	install_headers(join_paths('serializer', header), subdir: 'tmplORM/serializer')
endforeach

crunchMake = find_program('crunchMake', required: false)
subdir('test', if_found: crunchMake)
