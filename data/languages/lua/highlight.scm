;; Keywords

"return" @keyword

[
 "in"
 "local"
] @keyword

(break_statement) @keyword

(do_statement
[
  "do"
  "end"
] @keyword)

(while_statement
[
  "while"
  "do"
  "end"
] @keyword.repeat)

(repeat_statement
[
  "repeat"
  "until"
] @keyword.repeat)

(if_statement
[
  "if"
  "elseif"
  "else"
  "then"
  "end"
] @keyword.conditional)

(elseif_statement
[
  "elseif"
  "then"
  "end"
] @keyword.conditional)

(else_statement
[
  "else"
  "end"
] @keyword.conditional)

(for_statement
[
  "for"
  "do"
  "end"
] @keyword)

;; Function
(function_declaration
[
  "function"
  "end"
] @keyword)

(function_definition
[
  "function"
  "end"
] @keyword)

(function_declaration
  name:(identifier) @variable)
(function_call
  name:(identifier) @variable)

;;function [dirwatch]:[scan](directory, bool)
(method_index_expression
    table: (identifier) @variable
    method: (identifier) @function)

;; function [dirwatch]:[__index](args)
(dot_index_expression
    table: (identifier) @variable
    field: (identifier) @function)

(method_index_expression
  method:(identifier) @function)

;;local [common] = [require] "core.common"
(assignment_statement
      (variable_list
        name: (identifier) @variable)
      (expression_list
        value: (function_call
          name: (identifier) @function.call)))

;; Table
(table_constructor
  (field
    name: (identifier) @string))

;; Operators

[
 "and"
 "not"
 "or"
] @operator

[
  "+"
  "-"
  "*"
  "/"
  "%"
  "^"
  "#"
  "=="
  "~="
  "<="
  ">="
  "<"
  ">"
  "="
  "&"
  "~"
  "|"
  "<<"
  ">>"
  "//"
  ".."
] @operator


(nil) @number

[
  (false)
  (true)
] @boolean


(comment) @comment

(hash_bang_line) @preproc
(number) @number
(string) @string
(escape_sequence) @string.escape
