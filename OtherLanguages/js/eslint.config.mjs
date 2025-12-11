import js from "@eslint/js";
import parser from "@typescript-eslint/parser";
import tseslint from "@typescript-eslint/eslint-plugin";
import prettier from "eslint-plugin-prettier";

export default [
  js.configs.recommended,
  {
    plugins: {
      "@typescript-eslint": tseslint,
      "prettier": prettier.default || prettier
    },
    languageOptions: {
      parser,
      ecmaVersion: 2020,
      sourceType: "module"
    },
    env: {
      browser: true,
      node: true
    },
    rules: {
      "arrow-body-style": "error",
      "curly": "error",
      "eqeqeq": ["error", "smart"],
      "new-parens": "error",
      "no-caller": "error",
      "no-case-declarations": "error",
      "no-cond-assign": "error",
      "no-unused-vars": "off",
      "no-debugger": "error",
      "no-else-return": "error",
      "no-eval": "error",
      "no-extra-semi": "error",
      "no-implied-eval": "error",
      "no-inner-declarations": "error",
      "no-labels": ["error", { allowLoop: true, allowSwitch: true }],
      "no-new-func": "error",
      "no-new-wrappers": "error",
      "no-redeclare": "error",
      "no-restricted-imports": "error",
      "no-return-await": "error",
      "no-unsafe-finally": "error",
      "no-unused-expressions": ["error", { allowShortCircuit: true, allowTernary: true }],
      "no-var": "error",
      "object-shorthand": "error",
      "prefer-const": ["error", { destructuring: "all" }],
      "prefer-exponentiation-operator": "error",
      "prettier/prettier": "error",
      "radix": "error"
    }
  }
];
