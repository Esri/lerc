## Release procedure

Update version numbers in src/LercLib/include/Lerc_c_api.h and CMakeLists.txt

### npm release

- Config build environment. For 4.0 release: Node.js v16 LTS, npm 8.11
- Update the following files in OtherLanguages/js
  - Run npm version xxx to update version numbers in package.json
  - Update CHANGELOG.md
  - Update copyright year in Gruntfile.js, README.md and README.hbs
  - If applicable, update usage in README.md and README.hbs
- Build and publish to npm

```
cd OtherLanguages/js
npm install && npm run build
cd dist
npm pack --dry-run (check file list and version)
npm login
npm publish
```
