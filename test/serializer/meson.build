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
substrate = dependency('substrate')
tmplORM = declare_dependency(
	compile_args: '-I@0@/../..'.format(meson.source_root()),
	link_args: ['-L@0@/../..'.format(meson.source_root()), '-ltmplORM']
)

mysqlConfig = find_program('mysql_config')
mysql = declare_dependency(
	compile_args: run_command([mysqlConfig, '--cflags'], check: true).stdout().split(),
	link_args: run_command([mysqlConfig, '--libs'], check: true).stdout().split(),
	version: run_command([mysqlConfig, '--version'], check: true).stdout().strip()
)

mssqlToJSON = executable(
	'mssqlToJSON',
	['mssqlToJSON.cxx'],
	dependencies: [rSON, substrate, tmplORM],
	install: true
)

mysqlToJSON = executable(
	'mysqlToJSON',
	['mysqlToJSON.cxx'],
	dependencies: [rSON, substrate, tmplORM, mysql],
	install: true
)
