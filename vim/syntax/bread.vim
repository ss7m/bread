if exists("b:current_syntax")
    finish
endif

let b:current_syntax = "bread"

syntax keyword swindleKeyword and or not unit set begin
syntax keyword swindleKeyword if then else elif end
syntax keyword swindleKeyword while break continue for do
highlight link swindleKeyword Keyword

syntax keyword swindleBoolean true false
highlight link swindleBoolean Boolean

syntax match swindleGlobal "\v\@[_a-zA-Z0-9]+"
highlight link swindleGlobal Operator

syntax match swindleNumber "\d\+\.\d*\(e[-+]\=\d\+\)\=[fl]\="
syntax match swindleNumber "\d\+\(e[-+]\=\d\+\)\=[fl]\="
syntax match swindleNumber "\d\+e[-+]\=\d\+[fl]\=\>"
highlight link swindleNumber Number

syntax region swindleString start=/\v"/ skip=/\v\\./ end=/\v"/
highlight link swindleString String
