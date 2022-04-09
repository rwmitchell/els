function hide() {
  local args;
  if [[ $# == 0 ]]; then
      els_Eflag=""
  else
      for arg in $@; do
          els_Eflag+="+E'*$arg' "
      done
  fi
  string="\
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
  "

# printf ">>%s<<\n" "$string"
  eval $string
}
function unhide() {
  unset els_Eflag
  hide
}

unhide
