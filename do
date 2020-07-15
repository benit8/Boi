#!/bin/bash
set -e

############################################################

build_dir='build'
project_name='Boi'

############################################################

has_build_dir() {
	[ -d "$build_dir" ]
}

is_executable() {
	[ -x "$build_dir/$project_name" ]
}

is_shared_lib() {
	[ -x "$build_dir/lib$project_name.so" ]
}

is_static_lib() {
	[ -f "$build_dir/lib$project_name.a" ]
}

is_built() {
	has_build_dir && ( is_executable || is_shared_lib || is_static_lib )
}

############################################################

function build()
{
	if ! has_build_dir; then mkdir "$build_dir"; fi

	clear
	pushd "$build_dir"

	cmake .. -G "Unix Makefiles"
	cmake --build . -- -j $(nproc)

	popd
}

function run()
{
	if ! is_built; then build; fi
	if ! is_executable; then exit 1; fi

	"./$build_dir/$project_name" $@
}

function run_tests()
{
	if ! is_built; then build; fi

	"./${build_dir}/tests/${project_name}Tests" $@
}

function clean()
{
	if ! has_build_dir; then
		return 1
	fi

	rm -r "$build_dir"
}

############################################################

case "$1" in
	""   )                  ;&
	build) build            ;;
	run  ) run ${@:2}       ;;
	test ) run_tests ${@:2} ;;
	clean) clean            ;;
	*)
		echo "No operation '$1' found"
		exit 1
		;;
esac