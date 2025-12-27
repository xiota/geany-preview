// eslint.config.js
module.exports = [
  {
    files: ["**/*.js"],

    rules: {
      /* --- STRUCTURE (InsertBraces: true) --- */
      curly: ["error", "all"],

      /* --- INDENTATION (IndentWidth: 2) --- */
      indent: ["error", 2, { SwitchCase: 1 }],

      /* --- MAX LINE LENGTH (ColumnLimit: 96) --- */
      "max-len": ["error", { code: 96, ignoreUrls: true }],

      /* --- SPACING RULES (SpaceBeforeParens: ControlStatements) --- */
      "space-before-function-paren": [
        "error",
        {
          anonymous: "never",
          named: "never",
          asyncArrow: "always"
        }
      ],
      "keyword-spacing": ["error", { before: true, after: true }],
      "space-in-parens": ["error", "never"],
      "space-before-blocks": ["error", "always"],

      /* --- SEMICOLONS (Google style) --- */
      semi: ["error", "always"],

      /* --- QUOTES (Google style prefers double) --- */
      /* quotes: ["error", "double", { avoidEscape: true }], */

      /* --- WRAPPING / ALIGNMENT (BinPackArguments: false) --- */
      "function-paren-newline": ["error", "multiline"],
      "array-bracket-newline": ["error", { multiline: true }],
      "object-curly-newline": ["error", { multiline: true, consistent: true }],

      /* --- END OF FILE NEWLINE (InsertNewlineAtEOF: true) --- */
      "eol-last": ["error", "always"],

      /* --- NO SINGLE-LINE BLOCKS (AllowShortFunctionsOnASingleLine: Empty) --- */
      "brace-style": ["error", "1tbs", { allowSingleLine: false }],

      /* --- MISC CLEANLINESS --- */
      "no-trailing-spaces": "error",
      "comma-dangle": ["error", "never"]
    }
  }
];
