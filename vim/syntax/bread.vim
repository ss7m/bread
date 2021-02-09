if exists("b:current_syntax")
    finish
endif

let b:current_syntax = "bread"

syntax match breadComment "#.*$"
highlight link breadComment Comment

syntax keyword breadKeyword and or not set begin func
syntax keyword breadKeyword if then else elif end
syntax keyword breadKeyword while break continue for do
syntax keyword breadKeyword subclass constructor
highlight link breadKeyword Keyword

syntax keyword breadBoolean true false
highlight link breadBoolean Boolean

syntax keyword breadSpecial unit self this super
highlight link breadSpecial Special

syntax match breadBuiltin "\v\@[_a-zA-Z0-9]+"
highlight link breadBuiltin Identifier

" integer number
syn match breadNumber "\<\d\+\>"
" floating point number, with dot, optional exponent
syn match breadNumber  "\<\d\+\.\d*\%([eE][-+]\=\d\+\)\=\>"
" floating point number, without dot, with exponent
syn match breadNumber  "\<\d\+[eE][-+]\=\d\+\>"
highlight link breadNumber Number

syntax region breadString start=/\v"/ skip=/\v\\./ end=/\v"/
highlight link breadString String
