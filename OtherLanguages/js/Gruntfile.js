const copyright = `/*! Lerc 4.0
Copyright 2015 - 2023 Esri
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
A local copy of the license and additional notices are located with the
source distribution at:
http://github.com/Esri/lerc/
Contributors:  Thomas Maurer, Wenxue Ju
*/
`;

// eslint-disable-next-line no-undef
module.exports = function (grunt) {
  const bundeFormat = grunt.option("format") || "umd";
  const outputSurfix = bundeFormat === "umd" ? "" : "." + bundeFormat;
  const distFolder = "dist";
  const lercBundle = `${distFolder}/LercDecode${outputSurfix}.js`;
  grunt.initConfig({
    eslint: {
      options: {
        quiet: true,
        fix: grunt.option("fix")
      },
      target: ["src/**/*.ts"]
    },
    clean: {
      dist: [`${distFolder}/**/*.*`, `!${distFolder}/*.wasm`]
    },
    copy: {
      dist: {
        files: [
          {
            src: "src/LercDecode.d.ts",
            dest: `${distFolder}/LercDecode.d.ts`
          },
          {
            src: "src/LercDecode.d.ts",
            dest: `${distFolder}/LercDecode.es.d.ts`
          },
          {
            expand: true,
            src: ["package.json", "README.*", "CHANGELOG.*"],
            dest: `${distFolder}/`
          }
        ],
        options: {
          process: (content, srcpath) => {
            if (!srcpath.includes("package.json")) {
              return content;
            }
            const json = { ...JSON.parse(content), dependencies: {}, devDependencies: {}, scripts: {} };
            return JSON.stringify(json, null, 2);
          }
        }
      }
    },
    rollup: {
      options: {
        banner: copyright,
        name: "Lerc",
        format: bundeFormat,
        sourcemap: false
        // useStrict: false
      },
      dist: {
        files: [
          {
            dest: lercBundle,
            src: "src/Lerc.js"
          }
        ]
      }
    },
    terser: {
      options: {},
      dist: {
        files: [
          {
            dest: lercBundle.replace(".js", ".min.js"),
            src: lercBundle
          }
        ]
      }
    }
  });

  //grunt.task.loadTasks

  grunt.loadNpmTasks("grunt-contrib-clean");
  grunt.loadNpmTasks("grunt-contrib-copy");
  grunt.loadNpmTasks("grunt-contrib-concat");
  grunt.loadNpmTasks("grunt-rollup");
  grunt.loadNpmTasks("grunt-eslint");
  grunt.loadNpmTasks("grunt-terser");

  grunt.registerTask("default", ["eslint", "rollup"]);
  grunt.registerTask("dist", ["default", "terser", "copy"]);
};
