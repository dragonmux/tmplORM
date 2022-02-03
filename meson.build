project(
	'tmplORM',
	'cpp',
	default_options: [
		'cpp_std=c++11',
		'warning_level=3',
		'b_ndebug=if-release'
	],
	version: '0.2.0',
	meson_version: '>= 0.52',
	subproject_dir: 'deps'
)

cxx = meson.get_compiler('cpp')
pkgconfig = import('pkgconfig')
isWindows = target_machine.system() == 'windows'
coverage = get_option('b_coverage')

threading = dependency('threads')
dl = cxx.find_library('dl', required: not isWindows)
odbc = dependency('odbc')
pgsql = dependency('libpq')
if get_option('build_check')
	rSON = dependency('rSON')
endif
substrate = dependency('substrate', fallback: ['substrate', 'substrate_dep'], 	default_options: ['build_tests=false'])

includes = []

if cxx.has_header('/usr/include/postgresql/server/catalog/pg_type_d.h')
	includes += ['/usr/include/postgresql/server']
else
	grep = find_program('grep')
	if (build_machine.system() == 'linux' and
		run_command(grep, '-q', 'ID=arch', '/etc/os-release', check: false).returncode() == 0)
		warning('Please install the "posgresql" package')
	else
		warning('Cannot find required PostgreSQL header "catalog/pg_type_d.h" - ' +
			'please report this to your distro to have them add it to libpq')
	endif
	include += ['fixes']
endif

mysqlConfig = find_program('mysql_config')
mysqlCflags = run_command([mysqlConfig, '--cflags'], check: true).stdout().split()
mysqlLflags = run_command([mysqlConfig, '--libs'], check: true).stdout().split()
mysql = declare_dependency(
	compile_args: mysqlCflags,
	link_args: mysqlLflags
)

extended_warnings = [
	'-Wdouble-promotion',
	'-Wformat=2',
	'-Wformat-overflow=2',
	'-Wformat-signedness',
	'-Wformat-truncation',
	'-Wnull-dereference',
	'-Wmissing-attributes',
	'-Wmissing-braces',
	'-Wsequence-point',
	'-Wreturn-type',
	'-Wunused',
	'-Wunused-local-typedefs',
	'-Wunused-const-variable=2',
	'-Wmaybe-uninitialized',
	'-Wunknown-pragmas',
	'-Wstrict-aliasing',
	'-Wstrict-overflow=3',
	'-Wstring-compare',
	'-Wstringop-overflow',
	'-Warith-conversion',
	'-Wvla-parameter',
	'-Wduplicated-branches',
	'-Wshadow=local',
	'-Wunsafe-loop-optimizations',
	'-Wbad-function-cast',
	'-Wcast-qual',
	'-Wcast-align=strict',
	'-Wcast-function-type',
	'-Wconversion',
	'-Wdangling-else',
	'-Wsign-conversion',
	'-Wfloat-conversion',
	'-Wredundant-decls',
	'-Winline',
	'-Wvla',
	'-Wstack-protector',
	'-Wunsuffixed-float-constant',
	'-Wimplicit-fallthrough',
]

add_project_arguments(
	cxx.get_supported_arguments(extended_warnings),
	language: 'cpp'
)

if cxx.get_id() == 'msvc'
	add_project_arguments(
		'-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1',
		'-D_CRT_NONSTDC_NO_WARNINGS=1',
		language: 'cpp'
	)
endif

tmplORMSrc = [
	'string.cxx', 'mysql.cxx', 'mssql.cxx', 'pgsql.cxx',
	'tmplORM.types.cxx'
]
tmplORMSrcDir = meson.current_source_dir()

tmplORM = shared_library(
	'tmplORM',
	tmplORMSrc,
	include_directories: include_directories(includes),
	dependencies: [threading, dl, odbc, pgsql, mysql, substrate],
	version: meson.project_version(),
	install: true
)

pkgconfig.generate(
	tmplORM,
	name: 'tmplORM',
	version: meson.project_version(),
	description: 'The tmplORM template meta-programming ORM\'s C++ runtime'
)

if get_option('build_check')
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
endif

install_headers(
	'mysql.hxx', 'mssql.hxx', 'pgsql.hxx', 'tmplORM.hxx',
	'tmplORM.mysql.hxx', 'tmplORM.mssql.hxx', 'tmplORM.pgsql.hxx', 'tmplORM.common.hxx',
	'tmplORM.types.hxx', 'tmplORM.extern.hxx', 'string.hxx',
	subdir: 'tmplORM'
)
install_headers('typestring/typestring.hh', subdir: 'typestring')

foreach header : ['json.hxx', 'fromJSON.hxx', 'toJSON.hxx', 'helpers.hxx']
	install_headers(join_paths('serializer', header), subdir: 'tmplORM/serializer')
endforeach

if get_option('build_tests')
	crunchMake = find_program('crunchMake', required: false)
	subdir('test', if_found: crunchMake)
endif

tmplORM_dep = declare_dependency(
	include_directories: include_directories('.'),
	link_with: tmplORM,
	version: meson.project_version()
)

runClangTidy = find_program('runClangTidy.py')
run_target(
	'clang-tidy',
	command: [runClangTidy, '-s', meson.current_source_dir(), '-p', meson.current_build_dir()]
)
