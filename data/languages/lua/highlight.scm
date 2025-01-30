;; Keywords

"return" @keyword

[
 "in"
 "local"
] @keyword

(label_statement) @label

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
] @repeat)

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



;; Variables
((identifier) @number
 (#eq? @number "self"))

(variable_list
  name:(identifier) @label)

;; Function
(function_declaration
  name:(identifier) @function)


(nil) @constant.builtin

[
  (false)
  (true)
] @boolean


(comment) @comment
(hash_bang_line) @preproc
(number) @number
(string) @string
(escape_sequence) @string.escape
