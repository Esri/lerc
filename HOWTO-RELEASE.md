## Release procedure

Update version numbers in src/LercLib/include/Lerc_c_api.h

### npm release

- Config build environment. For 3.0 release: Node.js v14.17, npm 6.14.13
- Update the following files in OtherLanguages/js
  - Update version numbers in package.json
  - Update CHANGELOG.md
  - If applicable, update the copyright year and usage in README.md and README.hbs
- Build and publish to npm

```
cd OtherLanguages/js
npm install && npm run lint && npm run build
npm login
npm pack (only needed to review tar file locally)
npm publish
```
