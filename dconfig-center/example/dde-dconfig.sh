#!/bin/bash

# configure file is in the position. e.g: /usr/share/dsg/apps/dconfig-example/configs/example.json
# global cache is in the position. e.g: /var/dsg/appdata/dconfig-example/configs/example.json
# user cache is in the position. e.g: /home/userhome/.config/dconfig-example/example.json

# list all appid.
dde-dconfig --list

# list all resource for the appid.
dde-dconfig --list -a=dconfig-example

# list all matched common resource.
dde-dconfig --list -r=""

#list all subpath for the resource.
dde-dconfig --list -a=dconfig-example -r=example


# query all method for `-m` argument.
dde-dconfig --get

# query all keys for the resource, which `subpath` is '/a'.
dde-dconfig --get -a=dconfig-example -r=example -s=/a

# query name for the key, which `language` is 'zh_CN'.
dde-dconfig --get -a=dconfig-example -r=example -m=name -k=canExit -l=zh_CN

# query description for the key.
dde-dconfig --get -a=dconfig-example -r=example -m=description -k=canExit

# query visibility for the key.
dde-dconfig --get -a=dconfig-example -r=example -m=visibility -k=canExit

# query permissions for the key.
dde-dconfig --get -a=dconfig-example -r=example -m=permissions -k=canExit


# set value for the key.
dde-dconfig --set -a=dconfig-example -r=example -k=canExit -v=false
# set value for the key, which data type is array and value is json format.
dde-dconfig --set -a dconfig-example -r=example -k=array -v="[ \"value1\", \"value2\" ]"
# set value for the key, which data type is map and value is json format.
dde-dconfig --set -a dconfig-example -r=example -k=map -v="{ \"key1\": \"value1\", \"key2\": \"value2\" }"

# query value for the key
dde-dconfig --get -a=dconfig-example -r=example -k=canExit
# query value for the key, which data type is array and value is json format.
dde-dconfig --get -a=dconfig-example -r=example -k=array
