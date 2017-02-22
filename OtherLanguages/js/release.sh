#!/bin/bash

# config
VERSION=$(node --eval "console.log(require('./package.json').version);")
NAME=$(node --eval "console.log(require('./package.json').name);")

# no tests (yet)
# npm test || exit 1

# checkout temp branch for release
git checkout -b gh-release

# build files
npm run lint && npm run build

# force add files
git add LercDecode.min.js -f

# commit changes with a versioned commit message
git commit -m "build $VERSION"

# push commit so it exists on GitHub when we run gh-release
git push upstream gh-release

# run gh-release to create the tag and push release to github
gh-release

# checkout master and delete release branch locally and on GitHub
git checkout master
git branch -D gh-release
git push upstream :gh-release

# publish release on NPM
npm publish
