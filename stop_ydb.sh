#!/bin/sh
#set -x

pkill -9 ydb_server
pkill -9 lock_server
pkill -9 extent_server

