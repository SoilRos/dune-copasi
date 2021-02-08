#!/usr/bin/env bash

yarn install

set +e

BRANCH=$(cat ../../.git/HEAD | awk -F '/' '{print $NF}')

for tag in $(git tag)
  if [[ -d docs ]]; then
    yarn run docusaurus docs:version $tag
  fi
done

git checkout $BRANCH

yarn build