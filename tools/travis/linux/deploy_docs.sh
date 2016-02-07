#!/usr/env bash

set -eu

# Only deploy documentation if not building Pull Request
if [ "${TRAVIS_PULL_REQUEST}" == "false" ]
then
  # Decode and install the ssh private key which provides access to the
  # ror-documentation repository.
  openssl aes-256-cbc -pass pass:$encrypted_deploy_key_passwd -in tools/travis/linux/deploy_key.enc -out ~/.ssh/id_rsa -d > /dev/null 2>&1
  chmod 600 ~/.ssh/id_rsa
  
  # Initialize new empty repository for the documentation. Additionally configure the
  # user information which will be used to push the updated docs.
  mkdir -p ./build/gh-pages && cd ./build/gh-pages
  git init
  git config user.name "Travis CI"
  git config user.email "deploy@ror-documentation.org"
  
  # Add and commit the the newly generated documentation.
  cp -R ../doc/doxygen/html/* .
  git add -A
  git commit -m "[Automatic] Documentation update."
  
  # Deploy the resulting changes by pushing them to the gh-pages branch of the
  # ror-documenation repository. The new documentation is then available on the
  # corresponding Github pages.
  git push --force --quiet git@github.com:mikadou/ror-documentation.git master:gh-pages
  
  cd ../..
fi
