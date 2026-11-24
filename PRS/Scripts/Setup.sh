#!/usr/bin/env bash
set -e
cmake --preset debug
cmake --build --preset debug --parallel
echo 'Build OK: Intermediate/debug/bin/MyOpenWorld'
