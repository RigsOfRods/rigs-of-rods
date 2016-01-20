#!/usr/env bash

set -o nounset

# Decode and install the ssh private key which provides access to the
# ror-documentation repository.
openssl aes-256-cbc -K $encrypted_60d08e33f760_key -iv $encrypted_60d08e33f760_iv -in tools/travis/linux/deploy_key.enc -out ~/.ssh/id_rsa -d
chmod 600 ~/.ssh/id_rsa


# Initialize new empty repository for the documentation. Additionally configure the
# user information which will be used to push the updated
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
git push --force --quiet git@github.com:mikadou/ror-documentation.git master:gh-pages > /dev/null 2>&1

cd ../..

