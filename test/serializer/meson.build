project(
	'jsonTest',
	'cpp',
	default_options: [
		'cpp_std=c++11',
		'warning_level=3'
	]
)

cxx = meson.get_compiler('cpp')
rSON = dependency('rSON')
tmplORM = declare_dependency(
	compile_args: '-I@0@/../..'.format(meson.source_root()),
	link_args: ['-L@0@/../..'.format(meson.source_root()), '-ltmplORM']
)

mysqlConfig = find_program('mysql_config')
mysql = declare_dependency(
	compile_args: run_command([mysqlConfig, '--cflags']).stdout().split(),
	link_args: run_command([mysqlConfig, '--libs']).stdout().split(),
	version: run_command([mysqlConfig, '--version']).stdout().strip()
)

mssqlToJSON = executable(
	'mssqlToJSON',
	['mssqlToJSON.cxx'],
	dependencies: [rSON, tmplORM],
	install: true,
	build_by_default: false
)

mysqlToJSON = executable(
	'mysqlToJSON',
	['mysqlToJSON.cxx'],
	dependencies: [rSON, tmplORM, mysql],
	install: true
)
