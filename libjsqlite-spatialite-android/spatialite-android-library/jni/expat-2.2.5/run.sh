#! /usr/bin/env bash
# Copyright (C) 2017 Expat development team
# Licensed under the MIT license

case "arm-linux-eabi" in
*-mingw*)
    exec wine "$@"
    ;;
*)
    exec "$@"
    ;;
esac
