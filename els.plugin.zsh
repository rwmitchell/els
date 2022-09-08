alias -g MC='| mc'

[[ $+galias[MC] == 1 ]] || alias -g MC='| mc'

# setopt extendedglob - breaks els
# use _icwc to expand, then els to list
function _icwc() {                  # = ignore case wild card
  setopt extendedglob
  print $( print (#i)$~@ )   # ~ causes wildcard expansion
  setopt noextendedglob
}

# ignore case,  doesn't work with els_Eflag code
function lsi () { els +G~t~N                    $( _icwc $@ ) MC }
function lli () { els +T^NY-M-DT +G~At~smN      $( _icwc $@ ) MC }
function lti () { els +T^NY-M-DT +G~At~smN -rt  $( _icwc $@ ) MC }

function hidden() {      # show hidden extensions
  local arr=$( echo $els_Eflag | sed 's/+E//g;s/ /\n/g' )
  printf "%s\n" $arr
}

function _els_set_hide() {
  _els_string="\
  function ls  () { els +G~t~N    $els_Eflag \$@ MC }
  function lc  () { els +G~t~N -A $els_Eflag \$@ MC }

  function l   () { els +T^NY-M-DT +G~Atp~ugsmNL $els_Eflag \$@    }
  function lh  () { els +T^NY-M-DT +G~At~HmN     $els_Eflag \$@ MC }
  function ll  () { els +T^NY-M-DT +G~At~smN     $els_Eflag \$@ MC }
  function lt  () { els +T^NY-M-DT +G~At~smN -rt $els_Eflag \$@ MC }
  function lz  () { els +T^NY-M-DT +G~At~smN     $els_Eflag \$@    | sort -n -k2  }
  function Ll  () { els +T^NY-M-DT +G~At~smNL    $els_Eflag +FT{l} \$@     }  # show only symlinks
  function Lt  () { els +T^NY-M-DT +G~At~smNL    $els_Eflag -rt $@ }
  function Lz  () { els +T^NY-M-DT +G~At~smNL    $els_Eflag \$@    | sort -n -k2  }
  function lcrg() { els +G~t~N -AR +e".git"      $els_Eflag \$@ MC }    # recurse, exclude .git
  function lll () { els +T^NY-M-DT +Gl~At~smN     $els_Eflag \$@ MC }
  function li  () { els +T^NY-M-DT +Gl~Atp~ugsmNL $els_Eflag \$@    }   # show link count
  function llI () { els +T^NY-M-DT +Gl%11i~At~smN $els_Eflag \$@ MC }   # and inode number
  function llx () { els +T^NY-M-DT +G~At~smN -fr +FP{+x}    $els_Eflag \$@ MC }
  "
  eval $_els_string
}
# llx shows only executable files

# mnemonics for lll, li, and llI do not match match options, but
# nothing else seemed better

function hide_ext() {
  local args;
  if [[ $# == 0 ]]; then
      els_Eflag=""
  else
      for arg in $@; do
          els_Eflag+="+E'*$arg' "
      done
  fi

# printf ">>%s<<\n" "$_els_string"
  _els_set_hide
}

function unhide() {
  unset els_Eflag
  _els_set_hide
}

function hide_dir() {
  local args;
  if [[ $# == 0 ]]; then
      els_Eflag=""
  else
      for arg in $@; do
          els_Eflag+="+e$arg "
      done
  fi
  _els_set_hide
}

unhide
