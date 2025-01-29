[
  "if" 
  "else" 
  "while" 
  "do" 
  "for" 
  "switch" 
  "case" 
  "default" 
  "return" 
  "break" 
  "continue"
  "static"
  "const"
] @keyword

(auto) @keyword

; anything.[captured]
(field_expression
  field:(field_identifier) @variable.parameter)

;array access captured[]
(subscript_expression
  argument:(identifier) @variable.parameter)

(reference_declarator ["&" "&&"] @operator)
(pointer_declarator "*" @operator)
(pointer_expression ["*" "&"] @operator)
(binary_expression 
  ["<" "<=" ">" ">=" "!=" "==" "&" "&&" "||" "|"] @operator)
(unary_expression ["!"] @operator)

(assignment_expression
  ["&=" "|=" "*=" "/=" "+=" "-="] @operator)
(update_expression ["++" "--"] @operator)


(number_literal) @number
(true) @bool
(false) @bool
(auto) @type

; Constants
(this) @number
(null "nullptr" @number)

(type_identifier) @type


;comment
(comment) @comment 



(string_literal) @string
(char_literal) @string
(escape_sequence) @string.escape
(raw_string_literal) @string
(primitive_type) @keyword
(function_declarator declarator:(_) @function) 

;Preprocessor
(preproc_include (string_literal) @string)
(preproc_include (system_lib_string) @string)
(preproc_include) @keyword

  (preproc_def) @keyword
  (preproc_def name:(identifier) @type)
  (preproc_ifdef) @keyword
  (preproc_ifdef name:(identifier) @type)
  (preproc_else) @keyword
  (preproc_directive) @keyword

"::" @punctuation.delimiter
"<=>" @operator

; Keywords
[
  "try"
  "catch"
  "noexcept"
  "throw"
] @keyword.exception

[
  "decltype"
  "explicit"
  "friend"
  "override"
  "using"
  "requires"
  "constexpr"
] @keyword

[
  "class"
  "namespace"
  "template"
  "typename"
  "concept"
] @keyword.type

(access_specifier) @keyword.modifier

[
  "co_await"
  "co_yield"
  "co_return"
] @keyword.coroutine

[
  "new"
  "delete"
  "xor"
  "bitand"
  "bitor"
  "compl"
  "not"
  "xor_eq"
  "and_eq"
  "or_eq"
  "not_eq"
  "and"
  "or"
] @keyword.operator


; functions
(call_expression
  function: (identifier) @function)

(function_declarator
  (qualified_identifier
    name: (identifier) @function.call))

(function_declarator
  (template_function
    (identifier) @function))

(operator_name) @function

"operator" @function

"static_assert" @function

(call_expression
  (qualified_identifier
    (identifier) @function))

(call_expression
  (template_function
    (identifier) @function))

((namespace_identifier) @module)

(ERROR) @error
