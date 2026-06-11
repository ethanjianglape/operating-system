#!/bin/bash

find . -name "*.cpp" | xargs clang-format -i --style=file
find . -name "*.hpp" | xargs clang-format -i --style=file
