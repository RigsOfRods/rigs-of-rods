
#!/usr/bin/env bash

if ! [ -x "$(command -v clang-format)" ]; then
  printf "\e[31mError: the command clang-format was not found\e[0m"
  exit
fi

find .. -regex '.*\.\(h\|cpp\)' -not -path '*/plugins/*' -exec clang-format -style=file -i {} \;