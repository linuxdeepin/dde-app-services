#compdef dde-dconfig

# SPDX-FileCopyrightText: 2024 - 2026 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

_dde_dconfig_zsh_read_option()
{
  local option=$1 word
  local -i i
  _dde_dconfig_zsh_option_set=0
  _dde_dconfig_zsh_option_value=

  for ((i = 2; i < CURRENT; ++i)); do
    word=${words[i]}
    if [[ $word == $option ]]; then
      _dde_dconfig_zsh_option_set=1
      if ((i + 1 < CURRENT)); then
        _dde_dconfig_zsh_option_value=${words[i + 1]}
      fi
    elif [[ $word == ${option}=* ]]; then
      _dde_dconfig_zsh_option_set=1
      _dde_dconfig_zsh_option_value=${word#*=}
    fi
  done

  if [[ $_dde_dconfig_zsh_option_value == '""' || $_dde_dconfig_zsh_option_value == "''" ]]; then
    _dde_dconfig_zsh_option_value=
  fi
}

_dde_dconfig_zsh_find_command()
{
  local word
  local -i i skip_next=0
  _dde_dconfig_zsh_command=
  _dde_dconfig_zsh_command_token=
  for ((i = 2; i < CURRENT; ++i)); do
    word=${words[i]}
    if ((skip_next)); then
      skip_next=0
      continue
    fi
    case $word in
      -u | -a | -r | -s | -k | -v | -p | -m | -l) skip_next=1 ;;
      -u=* | -a=* | -r=* | -s=* | -k=* | -v=* | -p=* | -m=* | -l=* | -h | --help) ;;
      list | --list) _dde_dconfig_zsh_command=list ;;
      get | --get) _dde_dconfig_zsh_command=get ;;
      set | --set) _dde_dconfig_zsh_command=set ;;
      reset | --reset) _dde_dconfig_zsh_command=reset ;;
      watch | --watch) _dde_dconfig_zsh_command=watch ;;
      gui | --gui) _dde_dconfig_zsh_command=gui ;;
    esac
    if [[ -n $_dde_dconfig_zsh_command ]]; then
      _dde_dconfig_zsh_command_token=$word
      return
    fi
  done
}

_dde_dconfig_zsh_find_positionals()
{
  local word
  local -i i skip_next=0 command_seen=0
  _dde_dconfig_zsh_positionals=()

  for ((i = 2; i < CURRENT; ++i)); do
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
      _dde_dconfig_zsh_positionals+=("$word")
    fi
  done
  _dde_dconfig_zsh_positional_count=${#_dde_dconfig_zsh_positionals[@]}
}

_dde_dconfig_zsh_context()
{
  _dde_dconfig_zsh_find_positionals

  _dde_dconfig_zsh_read_option -a
  _dde_dconfig_zsh_appid_option_set=$_dde_dconfig_zsh_option_set
  _dde_dconfig_zsh_appid_set=$_dde_dconfig_zsh_option_set
  _dde_dconfig_zsh_appid=$_dde_dconfig_zsh_option_value
  if ((!_dde_dconfig_zsh_appid_set && _dde_dconfig_zsh_positional_count > 0)); then
    _dde_dconfig_zsh_appid_set=1
    _dde_dconfig_zsh_appid=${_dde_dconfig_zsh_positionals[1]}
  fi

  _dde_dconfig_zsh_read_option -r
  _dde_dconfig_zsh_resource_option_set=$_dde_dconfig_zsh_option_set
  _dde_dconfig_zsh_resource_set=$_dde_dconfig_zsh_option_set
  _dde_dconfig_zsh_resource=$_dde_dconfig_zsh_option_value
  if ((!_dde_dconfig_zsh_resource_set && _dde_dconfig_zsh_appid_set)); then
    _dde_dconfig_zsh_resource_set=1
    _dde_dconfig_zsh_resource=$_dde_dconfig_zsh_appid
  fi

  _dde_dconfig_zsh_read_option -k
  _dde_dconfig_zsh_key_option_set=$_dde_dconfig_zsh_option_set
  _dde_dconfig_zsh_key_set=$_dde_dconfig_zsh_option_set
  _dde_dconfig_zsh_key=$_dde_dconfig_zsh_option_value
  local -i key_positional_index=2
  ((_dde_dconfig_zsh_appid_option_set)) && key_positional_index=1
  if ((!_dde_dconfig_zsh_key_set && _dde_dconfig_zsh_positional_count >= key_positional_index)); then
    _dde_dconfig_zsh_key_set=1
    _dde_dconfig_zsh_key=${_dde_dconfig_zsh_positionals[key_positional_index]}
  fi
}

_dde_dconfig_zsh_complete_subpath()
{
  # A subpath is runtime-defined. Complete only its required leading slash;
  # an empty suffix keeps the cursor directly after `/` for further input.
  compadd -S '' -- /
}

_dde_dconfig_zsh_complete_resource_option()
{
  # Keep the generated command line consistent with Bash: complete `-r`
  # first and complete its value in the normal `-r VALUE` state.
  compadd -- -r
}

_dde_dconfig_zsh_complete_appids()
{
  local -a result
  result=("${(@f)$(command dde-dconfig list 2>/dev/null)}")
  result=(${result:#})
  # -Q inserts shell quotes rather than escaping them, so accepting this
  # candidate passes a real empty string to dde-dconfig.
  compadd -Q -- "''"
  ((${#result[@]})) && compadd -a result
}

_dde_dconfig_zsh_complete_resources()
{
  local insert_prefix=$1
  local -a result command_line
  _dde_dconfig_zsh_context
  command_line=(dde-dconfig list -a '')
  if ((_dde_dconfig_zsh_appid_set)); then
    command_line=(dde-dconfig list -a "$_dde_dconfig_zsh_appid")
  fi
  result=("${(@f)$(command "${command_line[@]}" 2>/dev/null)}")
  result=(${result:#})
  if ((${#result[@]})); then
    if [[ -n $insert_prefix ]]; then
      compadd -P "$insert_prefix" -a result
    else
      compadd -a result
    fi
  fi
}

_dde_dconfig_zsh_option_used()
{
  local option=$1 word
  local -i i
  for ((i = 2; i < CURRENT; ++i)); do
    word=${words[i]}
    [[ $word == $option || $word == ${option}=* ]] && return 0
  done
  return 1
}

_dde_dconfig_zsh_complete_options()
{
  # When an option prefix is already being typed, _arguments supplies the
  # same command-specific candidates. Avoid adding a second described copy.
  [[ $PREFIX == -* ]] && return 1

  local entry option
  local -a candidates=() supported=()
  case $_dde_dconfig_zsh_command in
    list)
      supported=(
        '-h:display help information' '--help:display help information'
        '-a:指定应用Id，空字符串表示应用无关配置'
        '-r:指定配置Id(resource)' '-p:指定工作目录前缀'
      )
      ;;
    get)
      supported=(
        '-h:display help information' '--help:display help information'
        '-u:指定用户uid' '-a:指定应用Id，空字符串表示应用无关配置'
        '-r:指定配置Id(resource)' '-s:指定动态子路径(subpath)'
        '-k:指定配置项Key值' '-p:指定工作目录前缀'
        '-m:指定get查询的方法' '-l:指定配置项语言'
      )
      ;;
    set)
      supported=(
        '-h:display help information' '--help:display help information'
        '-u:指定用户uid' '-a:指定应用Id，空字符串表示应用无关配置'
        '-r:指定配置Id(resource)' '-s:指定动态子路径(subpath)'
        '-k:指定配置项Key值' '-v:指定set命令的新值'
        '-p:指定工作目录前缀'
      )
      ;;
    reset | watch)
      supported=(
        '-h:display help information' '--help:display help information'
        '-u:指定用户uid' '-a:指定应用Id，空字符串表示应用无关配置'
        '-r:指定配置Id(resource)' '-s:指定动态子路径(subpath)'
        '-k:指定配置项Key值' '-p:指定工作目录前缀'
      )
      ;;
    gui)
      supported=('-h:display help information' '--help:display help information')
      ;;
    *)
      supported=(
        '-h:display help information' '--help:display help information'
        '-u:指定用户uid' '-a:指定应用Id，空字符串表示应用无关配置'
        '-r:指定配置Id(resource)' '-s:指定动态子路径(subpath)'
        '-k:指定配置项Key值' '-v:指定set命令的新值'
        '-p:指定工作目录前缀' '-m:指定get查询的方法'
        '-l:指定配置项语言'
      )
      ;;
  esac

  for entry in "${supported[@]}"; do
    option=${entry%%:*}
    _dde_dconfig_zsh_option_used "$option" || candidates+=("$entry")
  done
  ((${#candidates[@]})) || return 1
  _describe -t options 'options' candidates
}

_dde_dconfig_zsh_complete_keys()
{
  local insert_prefix=$1
  local -a result command_line
  _dde_dconfig_zsh_context
  ((_dde_dconfig_zsh_appid_set)) || return 1
  ((_dde_dconfig_zsh_resource_set)) || return 1
  [[ -n $_dde_dconfig_zsh_resource ]] || return 1

  command_line=(dde-dconfig --get -a "$_dde_dconfig_zsh_appid" -r "$_dde_dconfig_zsh_resource")
  # Subpath is dynamic. Key completion intentionally queries the resource
  # schema without trying to enumerate or validate the current -s value.
  result=("${(@f)$(command "${command_line[@]}" 2>/dev/null)}")
  result=(${result:#})
  if ((${#result[@]})); then
    if [[ -n $insert_prefix ]]; then
      compadd -P "$insert_prefix" -a result
    else
      compadd -a result
    fi
  fi
}

_dde_dconfig_zsh_complete_next()
{
  _dde_dconfig_zsh_context

  case $_dde_dconfig_zsh_command in
    list)
      if ((!_dde_dconfig_zsh_appid_set)); then
        _dde_dconfig_zsh_complete_appids
      elif ((!_dde_dconfig_zsh_resource_option_set)); then
        _dde_dconfig_zsh_complete_resource_option
      else
        _dde_dconfig_zsh_complete_options
      fi
      ;;
    get | set | reset | watch)
      if ((!_dde_dconfig_zsh_appid_set)); then
        _dde_dconfig_zsh_complete_appids
      elif ((!_dde_dconfig_zsh_key_set)); then
        if [[ -n $_dde_dconfig_zsh_resource ]]; then
          # The CLI accepts the next positional argument as Key both after a
          # positional appid and when appid was supplied with -a.
          _dde_dconfig_zsh_complete_keys
        elif ((!_dde_dconfig_zsh_resource_option_set)); then
          _dde_dconfig_zsh_complete_resource_option
        else
          _dde_dconfig_zsh_complete_options
        fi
      else
        _dde_dconfig_zsh_complete_options
      fi
      ;;
    *)
      _dde_dconfig_zsh_complete_options
      ;;
  esac
}

_dde-dconfig()
{
  local ret=1 state
  local -a commands=(
    'list:list模式，列出可配置的应用Id和配置Id'
    'get:get模式，用于查询指定配置项的信息'
    'set:set模式，用于设置配置项的值'
    'reset:reset模式，用于重置配置项的值'
    'watch:watch模式，用于监听配置项的更改'
    'gui:gui模式，用于启动dde-dconfig-editor'
  )
  _dde_dconfig_zsh_find_command
  local -a argument_options=(
    '(-h --help)-h[display help information]'
    '(-h --help)--help[display help information]'
  )
  case $_dde_dconfig_zsh_command in
    list)
      argument_options+=(
        '(-a)-a+[application id; empty means generic configuration]:appid:->appid'
        '(-r)-r+[configuration resource id]:resource:->resource'
        '(-p)-p+[working prefix directory]:directory:->directory'
      )
      ;;
    get)
      argument_options+=(
        '(-u)-u+[operate configuration for uid]:uid:->uid'
        '(-a)-a+[application id; empty means generic configuration]:appid:->appid'
        '(-r)-r+[configuration resource id]:resource:->resource'
        '(-s)-s+[dynamic configuration subpath]:subpath:->subpath'
        '(-k)-k+[configuration key]:key:->key'
        '(-p)-p+[working prefix directory]:directory:->directory'
        '(-m)-m+[query method]:method:->method'
        '(-l)-l+[configuration language]:language:->language'
      )
      ;;
    set)
      argument_options+=(
        '(-u)-u+[operate configuration for uid]:uid:->uid'
        '(-a)-a+[application id; empty means generic configuration]:appid:->appid'
        '(-r)-r+[configuration resource id]:resource:->resource'
        '(-s)-s+[dynamic configuration subpath]:subpath:->subpath'
        '(-k)-k+[configuration key]:key:->key'
        '(-v)-v+[new value for set]:value:->value'
        '(-p)-p+[working prefix directory]:directory:->directory'
      )
      ;;
    reset | watch)
      argument_options+=(
        '(-u)-u+[operate configuration for uid]:uid:->uid'
        '(-a)-a+[application id; empty means generic configuration]:appid:->appid'
        '(-r)-r+[configuration resource id]:resource:->resource'
        '(-s)-s+[dynamic configuration subpath]:subpath:->subpath'
        '(-k)-k+[configuration key]:key:->key'
        '(-p)-p+[working prefix directory]:directory:->directory'
      )
      ;;
    gui)
      ;;
    *)
      argument_options+=(
        '(-u)-u+[operate configuration for uid]:uid:->uid'
        '(-a)-a+[application id; empty means generic configuration]:appid:->appid'
        '(-r)-r+[configuration resource id]:resource:->resource'
        '(-s)-s+[dynamic configuration subpath]:subpath:->subpath'
        '(-k)-k+[configuration key]:key:->key'
        '(-v)-v+[new value for set]:value:->value'
        '(-p)-p+[working prefix directory]:directory:->directory'
        '(-m)-m+[query method]:method:->method'
        '(-l)-l+[configuration language]:language:->language'
        '--list[list mode]' '--get[get mode]' '--set[set mode]'
        '--reset[reset mode]' '--watch[watch mode]'
        '--gui[start dde-dconfig-editor]'
      )
      ;;
  esac

  # A hidden long command is itself an option. Keep its spec so _arguments can
  # parse the remaining words, while the self-exclusion prevents suggesting it
  # again. Positional commands do not need any hidden command specs.
  if [[ $_dde_dconfig_zsh_command_token == --* ]]; then
    case $_dde_dconfig_zsh_command in
      list) argument_options+=('(--list)--list[list mode]') ;;
      get) argument_options+=('(--get)--get[get mode]') ;;
      set) argument_options+=('(--set)--set[set mode]') ;;
      reset) argument_options+=('(--reset)--reset[reset mode]') ;;
      watch) argument_options+=('(--watch)--watch[watch mode]') ;;
      gui) argument_options+=('(--gui)--gui[start dde-dconfig-editor]') ;;
    esac
  fi

  # The rest argument uses a single colon so `words` retains the full
  # command line for the context scanner below.
  _arguments -C -s -S -n \
    "${argument_options[@]}" \
    '1:command:->commands' \
    '*:argument:->args' && ret=0

  _dde_dconfig_zsh_find_command
  # With an attached short-option value (`-r=value`), _arguments leaves
  # the leading `=` in PREFIX. Move it to IPREFIX so candidates match the
  # actual value while preserving the option text on insertion.
  [[ $PREFIX == '='* ]] && compset -p 1

  case $state in
    commands)
      if [[ -n $_dde_dconfig_zsh_command ]]; then
        _dde_dconfig_zsh_complete_next && ret=0
      else
        _describe -t commands 'commands' commands && ret=0
      fi
      ;;
    appid)
      _dde_dconfig_zsh_complete_appids && ret=0
      ;;
    resource)
      _dde_dconfig_zsh_complete_resources && ret=0
      ;;
    subpath)
      _dde_dconfig_zsh_complete_subpath && ret=0
      ;;
    key)
      _dde_dconfig_zsh_complete_keys && ret=0
      ;;
    directory)
      _directories && ret=0
      ;;
    method)
      compadd -- value name description visibility permissions version isDefaultValue && ret=0
      ;;
    uid | value | language)
      _message "enter $state" && ret=0
      ;;
    args)
      _dde_dconfig_zsh_complete_next && ret=0
      ;;
  esac

  return ret
}

_dde-dconfig
