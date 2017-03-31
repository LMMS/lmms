#!/usr/bin/env bash

if [ "$TYPE" = 'style' ]; then

  # shellcheck disable=SC2046
  shellcheck $(find -O3 "$TRAVIS_BUILD_DIR/.travis/" "$TRAVIS_BUILD_DIR/cmake/" -type f -name '*.sh')

  files=$(find -O3 "$TRAVIS_BUILD_DIR/src/" "$TRAVIS_BUILD_DIR/include/" "$TRAVIS_BUILD_DIR/tests/" -type f -regex '.*\.\(cpp\|hpp\|c\|h\)')

  for f in $files
  do
    KWStyle -v -xml "$TRAVIS_BUILD_DIR/kwstyle.xml" "$f" >> kwstyle-output.txt
  done;

  if [ "$(grep -c Error < kwstyle-output.txt)" -gt 0 ]; then
    cat kwstyle-output.txt
    rm -f kwstyle-output.txt
    exit 1;
  fi

  rm -f kwstyle-output.txt

  for f in $files
  do
    astyle --ascii "$f" >> astyle-output.txt
  done;

  # debug
  cat astyle-output.txt

  if [ "$(grep -c Error < astyle.txt)" -gt 0 ]; then
    cat astyle-output.txt
    rm -f astyle-output.txt
    exit 1;
  fi

  rm -f astyle-output.txt


else
  # shellcheck disable=SC1090
  . "$TRAVIS_BUILD_DIR/.travis/$TRAVIS_OS_NAME.$TARGET_OS.script.sh"

  make -j4

  if [[ $TARGET_OS != win* ]]; then make tests && ./tests/tests; fi;
fi
