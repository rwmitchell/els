alias -g MC='| mc'

[[ $+galias[MC] == 1 ]] || alias -g MC='| mc'
[[ $+galias[GB] == 1 ]] || alias -g GB='|& PRISM="#90ee90#F0F0F0" prism -r -B'

# setopt extendedglob - breaks els
# use _icwc to expand, then els to list
function _icwc() {                  # = ignore case wild card
  setopt extendedglob
  print $( print (#i)$~@ )   # ~ causes wildcard expansion
  setopt noextendedglob
}

# ignore case,  doesn't work with els_Eflag code
# NOTE: wildcard chars MUST be delimited!
# ie: lsi \*foo\*
# or: lsi "*foo*"
function lsi () { els +G~q~N                    $( _icwc $@ ) MC }
function lli () { els +T^NY-M-DT +G~Aq~smN      $( _icwc $@ ) MC }
function lti () { els +T^NY-M-DT +G~Aq~smN -rt  $( _icwc $@ ) MC }

# provide quick stuff to hide
ELS_MS_HIDE=( .pptx .shs .lnk .exe )   # really boring stuff
ELS_MS_FILE=( .mp4  .ppt .xls .doc )   # mostly boring stuff

function hidden() {      # show hidden extensions
  local arr=$( echo $els_Eflag | sed 's/+E//g;s/ /\n/g' )
  printf "%s\n" $arr
}

function _els_set_hide() {
  _els_string="\
  function ls  () { els +G~q~N    $els_Eflag \$@ MC }
  function lc  () { els +G~q~N -A $els_Eflag \$@ MC }

  function l   () { els +T^NY-M-DT +G~Atp~ugsmNL  $els_Eflag \$@    }
  function lh  () { els +T^NY-M-DT +G~Aq~HmN      $els_Eflag \$@ MC }
  function ll  () { els +T^NY-M-DT +G~Aq~smN      $els_Eflag \$@ MC }   # size date glyph filename
  function lll () { els +T^NY-M-DT +G~Aq~slmN     $els_Eflag \$@ MC }   # size link-count date glyph filename
  function lt  () { els +T^NY-M-DT +G~Aq~slmN -rt $els_Eflag \$@ MC }   # above, sorted by time
  # putting GB last breaks the column alignment
  function lsgb() { els +G~q~N    $els_Eflag \$@ GB MC -R 6}
  function llgb() { els +T^NY-M-DT +G~Aq~smN      $els_Eflag \$@ GB MC -R }
  function Ll  () { els +T^NY-M-DT +G~Aq~slmNL    $els_Eflag +FT{l} \$@     }  # show only symlinks
  function Lt  () { els +T^NY-M-DT +G~Aq~slmNL    $els_Eflag -rt $@ }
  function lcrg() { els +G~t~N -AR +e".git"       $els_Eflag \$@ MC }   # recurse, exclude .git
  function li  () { els +T^NY-M-DT +G~Aqp~ugslmNL $els_Eflag \$@    }   # show link count
  function llI () { els +T^NY-M-DT +Gl%11i~Aq~smN $els_Eflag \$@ MC }   # and inode number
  function lx  () { els +T^NY-M-DT +G~Aqp~ugsmNL -fr +FP{+x}    $els_Eflag \$@    }
  function llx () { els +T^NY-M-DT +G~Aq~smN     -fr +FP{+x}    $els_Eflag \$@ MC }
  function lz  () {
    [[ \$# -gt 0 ]] && els +T^NY-M-DT +G~Aq~SsmN  $els_Eflag \$@   | sort -n -k1 \
                    || els +T^NY-M-DT +G~Aq~SsmN  $els_Eflag  *(.) | sort -n -k1
  }
  function Lz  () {
    [[ \$# -gt 0 ]] && els +T^NY-M-DT +G~Aq~SsmNL $els_Eflag \$@   | sort -n -k1 \
                    || els +T^NY-M-DT +G~Aq~SsmNL $els_Eflag  *(.) | sort -n -k1
  }

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
