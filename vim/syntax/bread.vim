if exists("b:current_syntax")
    finish
endif

let b:current_syntax = "bread"

syntax match breadComment "#.*$"
highlight link breadComment Comment

syntax keyword breadKeyword and or not set begin
syntax keyword breadKeyword if then else elif end
syntax keyword breadKeyword while break continue for do
highlight link breadKeyword Keyword

syntax keyword breadBoolean true false unit
highlight link breadBoolean Boolean

syntax match breadGlobal "\v\@[_a-zA-Z0-9]+"
highlight link breadGlobal Operator

syntax match breadNumber "\d\+\(e[-+]\=\d\+\)\="
syntax match breadNumber "\d\+\.\d*\(e[-+]\=\d\+\)\="
syntax match breadNumber "\d\+e[-+]\=\d\+\>"
highlight link breadNumber Number

syntax region breadString start=/\v"/ skip=/\v\\./ end=/\v"/
highlight link breadString String
