" Adapted from Lua's indent file
if exists("b:did_indent")
    finish
endif
let b:did_indent = 1

setlocal indentexpr=GetBreadIndent()

setlocal indentkeys+=0=end,0=elif

setlocal autoindent

if exists("*GetBreadIndent")
    finish
endif

function! GetBreadIndent()
    " Find a non blank line above the the current line
    let prevlnum = prevnonblank(v:lnum - 1)

    if prevlnum == 0
        return 0
    endif

    let ind = indent(prevlnum)
    let prevline = getline(prevlnum)
    let midx = match(prevline, '\%(\<begin\>\|\<subclass\>\|\<then\>\|\<do\>\)\s*$')

    if midx == -1
        let midx = match(prevline, '(\s*$')
        if midx == -1
          let midx = match(prevline, '\(\<func\|\<constructor\|\<subclass\)\s*(')
        endif
    endif

    if midx != -1
        if synIDattr(synID(prevlnum, midx + 1, 1), "name") != "breadComment" && prevline !~ '\<end\>'
            let ind = ind + shiftwidth()
        endif
    endif

    let midx = match(getline(v:lnum), '^\s*\%(end\>\|else\>\|elif\>\)')
    
    if midx == -1
        let midx = match(getline(v:lnum), ')\s*$')
    endif

    if midx != -1 && synIDattr(synID(v:lnum, midx + 1, 1), "name") != "breadComment"
        let ind = ind - shiftwidth()
    endif

    return ind
endfunction
