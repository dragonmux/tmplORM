#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
from argparse import ArgumentParser
from pathlib import Path
from subprocess import run
from concurrent.futures import ThreadPoolExecutor
from sys import exit

parser = ArgumentParser(
	description = 'Light-weight wrapper around clang-tidy to enable `ninja clang-tidy` to function properly',
	allow_abbrev = False
)
parser.add_argument('-s', required = True, type = str, metavar = 'sourcePath',
	dest = 'sourcePath', help = 'Path to the source directory to run clang-tidy in')
parser.add_argument('-p', required = True, type = str, metavar = 'buildPath',
	dest = 'buildPath', help = 'Path to the build directory containing a compile_commands.json')
args = parser.parse_args()

def globFiles():
	srcDir = Path(args.sourcePath)
	paths = {'serializer', 'test'}
	suffixes = {'c','C', 'cc', 'cpp', 'cxx', 'CC', 'h', 'H', 'hh', 'hpp', 'hxx', 'HH'}
	for path in paths:
		for suffix in suffixes:
			yield srcDir.glob('{}/**/*.{}'.format(path, suffix))
	# This also processes all the files in the root of the project
	for suffix in suffixes:
		yield srcDir.glob('*.{}')

def gatherFiles():
	for fileGlob in globFiles():
		for file in fileGlob:
			yield file

extraArgs = [
	f'--extra-arg=-I{args.sourcePath}/deps/substrate',
	'--extra-arg=-I/usr/include/postgresql/server'
	'--extra-arg=-I/usr/include/crunch++',
	f'--extra-arg=-I{args.sourcePath}',
]

futures = []
returncode = 0
with ThreadPoolExecutor() as pool:
	for file in gatherFiles():
		futures.append(pool.submit(run, ['clang-tidy'] + extraArgs + ['-p', args.buildPath, file]))
	returncode = max((future.result().returncode for future in futures))
exit(returncode)
