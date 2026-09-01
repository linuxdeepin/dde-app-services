#!/bin/bash

# SPDX-FileCopyrightText: 2024 - 2026 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

_dde_dconfig_read_option()
{
	local option=$1 i word
	_dde_dconfig_option_set=0
	_dde_dconfig_option_value=

	for ((i = 1; i < cword; ++i)); do
		word=${words[i]}
		if [[ $word == "$option" ]]; then
			_dde_dconfig_option_set=1
			if ((i + 1 < cword)); then
				_dde_dconfig_option_value=${words[i + 1]}
			fi
		elif [[ $word == "$option="* ]]; then
			_dde_dconfig_option_set=1
			_dde_dconfig_option_value=${word#*=}
		fi
	done

	# zsh and some bash-completion versions can retain the source quotes for
	# an explicitly empty word. They mean the documented empty appid/value.
	if [[ $_dde_dconfig_option_value == '""' || $_dde_dconfig_option_value == "''" ]]; then
		_dde_dconfig_option_value=
	fi
}

_dde_dconfig_find_command()
{
	local i word skip_next=0
	_dde_dconfig_command=
	for ((i = 1; i < cword; ++i)); do
		word=${words[i]}
		if ((skip_next)); then
			skip_next=0
			continue
		fi
		case $word in
		-u | -a | -r | -s | -k | -v | -p | -m | -l) skip_next=1 ;;
		-u=* | -a=* | -r=* | -s=* | -k=* | -v=* | -p=* | -m=* | -l=* | -h | --help) ;;
		list | --list) _dde_dconfig_command=list ;;
		get | --get) _dde_dconfig_command=get ;;
		set | --set) _dde_dconfig_command=set ;;
		reset | --reset) _dde_dconfig_command=reset ;;
		watch | --watch) _dde_dconfig_command=watch ;;
		gui | --gui) _dde_dconfig_command=gui ;;
		esac
		[[ -n $_dde_dconfig_command ]] && return
	done
}

_dde_dconfig_find_positionals()
{
	local i word skip_next=0 command_seen=0
	_dde_dconfig_positionals=()

	for ((i = 1; i < cword; ++i)); do
		word=${words[i]}
		if ((skip_next)); then
			skip_next=0
			continue
		fi
		case $word in
		-u | -a | -r | -s | -k | -v | -p | -m | -l)
			skip_next=1
			continue
			;;
		-u=* | -a=* | -r=* | -s=* | -k=* | -v=* | -p=* | -m=* | -l=* | -h | --help)
			continue
			;;
		--list | --get | --set | --reset | --watch | --gui)
			command_seen=1
			continue
			;;
		list | get | set | reset | watch | gui)
			if ((!command_seen)); then
				command_seen=1
				continue
			fi
			;;
		-*)
			continue
			;;
		esac

		if ((command_seen)); then
			if [[ $word == '""' || $word == "''" ]]; then
				word=
			fi
			_dde_dconfig_positionals+=("$word")
		fi
	done
	_dde_dconfig_positional_count=${#_dde_dconfig_positionals[@]}
}

_dde_dconfig_context()
{
	_dde_dconfig_find_positionals

	_dde_dconfig_read_option -a
	_dde_dconfig_appid_option_set=$_dde_dconfig_option_set
	_dde_dconfig_appid_set=$_dde_dconfig_option_set
	_dde_dconfig_appid=$_dde_dconfig_option_value
	if ((!_dde_dconfig_appid_set && _dde_dconfig_positional_count > 0)); then
		_dde_dconfig_appid_set=1
		_dde_dconfig_appid=${_dde_dconfig_positionals[0]}
	fi

	_dde_dconfig_read_option -r
	_dde_dconfig_resource_option_set=$_dde_dconfig_option_set
	_dde_dconfig_resource_set=$_dde_dconfig_option_set
	_dde_dconfig_resource=$_dde_dconfig_option_value
	if ((!_dde_dconfig_resource_set && _dde_dconfig_appid_set)); then
		_dde_dconfig_resource_set=1
		_dde_dconfig_resource=$_dde_dconfig_appid
	fi

	_dde_dconfig_read_option -k
	_dde_dconfig_key_option_set=$_dde_dconfig_option_set
	_dde_dconfig_key_set=$_dde_dconfig_option_set
	_dde_dconfig_key=$_dde_dconfig_option_value
	local key_positional_index=1
	((_dde_dconfig_appid_option_set)) && key_positional_index=0
	if ((!_dde_dconfig_key_set && _dde_dconfig_positional_count > key_positional_index)); then
		_dde_dconfig_key_set=1
		_dde_dconfig_key=${_dde_dconfig_positionals[key_positional_index]}
	fi
}

_dde_dconfig_add_matches()
{
	local insert_prefix=$1 typed=$2 candidate
	shift 2
	for candidate in "$@"; do
		[[ $candidate == "$typed"* ]] && COMPREPLY+=("${insert_prefix}${candidate}")
	done
}

_dde_dconfig_complete_subpath()
{
	local typed=$1
	# A subpath is runtime-defined. Complete only its required leading slash;
	# do not enumerate or validate any content below it.
	[[ / == "$typed"* ]] && COMPREPLY+=(/)
	compopt -o nospace 2>/dev/null
}

_dde_dconfig_complete_resource_option()
{
	# Do not synthesize an option=value word. Complete the option first, then
	# let the normal `-r VALUE` state complete the resource on the next Tab.
	_dde_dconfig_add_matches '' "$cur" -r
}

_dde_dconfig_complete_appids()
{
	local insert_prefix=$1 typed=$2 line
	local -a candidates=("''")
	while IFS= read -r line; do
		[[ -n $line ]] && candidates+=("$line")
	done < <(command dde-dconfig list 2>/dev/null)
	_dde_dconfig_add_matches "$insert_prefix" "$typed" "${candidates[@]}"
}

_dde_dconfig_complete_resources()
{
	local insert_prefix=$1 typed=$2 line
	local -a candidates=() command_line=(dde-dconfig list -a '')
	_dde_dconfig_context
	if ((_dde_dconfig_appid_set)); then
		command_line=(dde-dconfig list -a "$_dde_dconfig_appid")
	fi
	while IFS= read -r line; do
		[[ -n $line ]] && candidates+=("$line")
	done < <(command "${command_line[@]}" 2>/dev/null)
	_dde_dconfig_add_matches "$insert_prefix" "$typed" "${candidates[@]}"
}

_dde_dconfig_complete_keys()
{
	local insert_prefix=$1 typed=$2 line
	local -a candidates=() command_line
	_dde_dconfig_context
	((_dde_dconfig_appid_set)) || return
	((_dde_dconfig_resource_set)) || return
	[[ -n $_dde_dconfig_resource ]] || return

	command_line=(dde-dconfig --get -a "$_dde_dconfig_appid" -r "$_dde_dconfig_resource")
	# Subpath is dynamic. Key completion intentionally queries the resource
	# schema without trying to enumerate or validate the current -s value.
	while IFS= read -r line; do
		[[ -n $line ]] && candidates+=("$line")
	done < <(command "${command_line[@]}" 2>/dev/null)
	_dde_dconfig_add_matches "$insert_prefix" "$typed" "${candidates[@]}"
}

_dde_dconfig_option_used()
{
	local option=$1 word i
	# The current word is only a prefix being completed, not an option that
	# has already been consumed. This matches the Zsh context scan.
	for ((i = 1; i < cword; ++i)); do
		word=${words[i]}
		[[ $word == "$option" || $word == "$option="* ]] && return 0
	done
	return 1
}

_dde_dconfig_complete_options()
{
	local option
	local -a options=()
	case $_dde_dconfig_command in
	list) options=(-h --help -a -r -p) ;;
	get) options=(-h --help -u -a -r -s -k -p -m -l) ;;
	set) options=(-h --help -u -a -r -s -k -v -p) ;;
	reset | watch) options=(-h --help -u -a -r -s -k -p) ;;
	gui) options=(-h --help) ;;
	*) options=(-h --help -u -a -r -s -k -v -p -m -l) ;;
	esac

	for option in "${options[@]}"; do
		if ! _dde_dconfig_option_used "$option" && [[ $option == "$cur"* ]]; then
			COMPREPLY+=("$option")
		fi
	done
}

_dde_dconfig_complete_next()
{
	_dde_dconfig_context

	case $_dde_dconfig_command in
	list)
		if ((!_dde_dconfig_appid_set)); then
			_dde_dconfig_complete_appids '' "$cur"
		elif ((!_dde_dconfig_resource_option_set)); then
			_dde_dconfig_complete_resource_option
		else
			_dde_dconfig_complete_options
		fi
		;;
	get | set | reset | watch)
		if ((!_dde_dconfig_appid_set)); then
			_dde_dconfig_complete_appids '' "$cur"
		elif ((!_dde_dconfig_key_set)); then
			if [[ -n $_dde_dconfig_resource ]]; then
				# The CLI accepts the next positional argument as Key both after a
				# positional appid and when appid was supplied with -a.
				_dde_dconfig_complete_keys '' "$cur"
			elif ((!_dde_dconfig_resource_option_set)); then
				# An empty positional appid denotes generic configuration and has
				# no implicit resource. Offer -r as the next step.
				_dde_dconfig_complete_resource_option
			else
				_dde_dconfig_complete_options
			fi
		else
			_dde_dconfig_complete_options
		fi
		;;
	*)
		_dde_dconfig_complete_options
		;;
	esac
}

_dde_dconfig()
{
	local cur prev words cword
	# Keep --option=value as one word. Otherwise bash-completion splits it
	# around `=`, and the value can be mistaken for a positional argument.
	_init_completion -n = || return
	COMPREPLY=()
	_dde_dconfig_find_command

	case $cur in
	-a=*) _dde_dconfig_complete_appids '' "${cur#*=}"; return ;;
	-r=*) _dde_dconfig_complete_resources '' "${cur#*=}"; return ;;
	-k=*) _dde_dconfig_complete_keys '' "${cur#*=}"; return ;;
	-s=*) _dde_dconfig_complete_subpath "${cur#*=}"; return ;;
	-u=* | -v=* | -l=*) return ;;
	-p=*)
		cur=${cur#*=}
		_filedir -d
		return
		;;
	-m=*)
		_dde_dconfig_add_matches '' "${cur#*=}" value name description visibility permissions version isDefaultValue
		return
		;;
	esac

	case $prev in
	-a) _dde_dconfig_complete_appids '' "$cur"; return ;;
	-r) _dde_dconfig_complete_resources '' "$cur"; return ;;
	-k) _dde_dconfig_complete_keys '' "$cur"; return ;;
	-s) _dde_dconfig_complete_subpath "$cur"; return ;;
	-u | -v | -l) return ;;
	-p) _filedir -d; return ;;
	-m)
		_dde_dconfig_add_matches '' "$cur" value name description visibility permissions version isDefaultValue
		return
		;;
	esac

	if [[ $cur == -* ]]; then
		if [[ -z $_dde_dconfig_command ]]; then
			_dde_dconfig_add_matches '' "$cur" -h --help -u -a -r -s -k -v -p -m -l --list --get --set --reset --watch --gui
		else
			_dde_dconfig_complete_options
		fi
		return
	fi

	if [[ -z $_dde_dconfig_command ]]; then
		_dde_dconfig_add_matches '' "$cur" help list get set reset watch gui
		return
	fi

	_dde_dconfig_complete_next
}

complete -F _dde_dconfig dde-dconfig
