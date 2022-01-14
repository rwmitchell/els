#!/bin/bash
# 2020-11-09 - Generate LS_ICONS using default colors from LS_COLORS
# based on even-better-ls/ls_icon_generator.sh

# Create table of default values
# Load icons from ICON_SRC
# Use LS_COLORS to:
#    - override colors in existing entries
#    - create new entries without icons

ICON_SRC="$HOME/.rc/etc/ls_colors_generator.py"
declare -a table

unset Update_Existing

function show_icons {
  for i in "${!table[@]}"; do
    s=${table[$i]}
    printf "Icon-> %s | %s <-\n" $(color_glyph $s) "$s"
  done
}

function create_ls_icons {
  for i in "${!table[@]}"; do
    a=(${table[$i]})
    s=${table[$i]}
#   a[0]="${a[0]%\"}"     # remove leading
#   a[0]="${a[0]#\"}"     # and trailing quote symbols
    printf "%s=%d" ${a[0]} ${a[1]}
    [[ ${a[2]} != -1 ]] && printf ";%d" ${a[2]}
    [[ ${a[4]} != "" ]] && printf ";%d" ${a[4]}
    printf "m%s:" $(plain_character $s)
  done
  printf "\n"
}

function list_glyphs {
  # output glyph with extension/pattern

  # debug block, disabled
  if [[ $test == 1 ]]; then
    for i in "${!table[@]}"; do
      s=${table[$i]}
      t=(${s})
      c=$(color_glyph "$s" );
      printf "%s: %s --> %s # %s\n" "$c" "$s" "$t" "$i";
    done
    exit 0;
  fi

  if [ $# -gt 0 ]; then
    slct=1;
    mylist=" $@ "  # add surrounding spaces to match string matching below
  fi

  KEYS=$(
    for i in "${!table[@]}"; do
      s=${table[$i]}
      t=(${s})
      c=$(color_glyph "$s" );
      printf "%s:::%s\n" "$c" "$i"
    done | sort -k 4 | awk -F::: '{print $2}'
  )

  oglyph=""
  nglyph=""
  for KEY in $KEYS; do
    s=${table[$KEY]}
    t=(${s})
#   printf "%s %s\n" "$s" "$t"
    nglyph=$( color_glyph ${table[$KEY]} );
    if [ $slct ]; then
      ext="${t%\"}"    # need to remove quotes surrounding string
      ext="${ext#\"}"
      [[ $mylist == *" $ext "* ]] &&
        printf " %s\t%s\n" $( color_glyph $s ) $( color_ext $s )
    else
      if [ "$oglyph" != "$nglyph" ]; then
        printf "\n %s   %s" \
          $( color_glyph ${table[$KEY]} ) $( color_ext "${table[$KEY]}")
        oglyph="$nglyph";
      else
        printf "  %s" $( color_ext "${table[$KEY]}" )
      fi
    fi
  done
  printf "\n";

# for i in "${!table[@]}"; do
#   s=${table[$i]}
#   t=(${s})
#   printf "%s:%s\n" "$t" "$s"
# done # | sort | awk -F::: '{print $2}'

# printf "KEYS:%s\n" "$KEYS"

  exit 0;

}

function update_icon {
  new=($@)
  foo=("$@")
  bar="($@)"
# printf "CHECK: >%s< >%s< >%s<\n" "${new[0]}" "${foo[0]}" "${bar[0]}"
# printf "Update cnt: %d : >%s< : <%s> : <%s> <%s>\n" ${#new[@]} "${new[0]}" "${new[1]}" "${new[2]}" "${new[3]}"
  append=1
  for i in "${!table[@]}"; do
    s=${table[$i]}
    t=(${s})

#   printf "i:%s   s:%s  %s t:%s\n" "$i" "$s" $( color_glyph $s ) "$t"

#   [[ $verbose ]] && printf "CHK -->%s | %s<--\n" "${t[0]}" \"${new[0]}\"
    # Put quotes around $new to match quotes in $t
    if [[ "${t[0]}" == \"${new[0]}\" ]]; then     # only doing exact matches

#     printf "Update Match-->%s | %s=%s<--\n" $(color_glyph $s) $(color_ext "${table[$i]}") "${new[0]}"
#     printf "Update match-->%s | %s=%s<--\n" $(color_glyph $s) $(ocolor_ext "${table[$i]}") "${new[0]}"
#     printf "OLD cnt: %d : >%s< : < %s > : < %s > < %s > < %s >\n" ${#t[@]} "${t[0]}" "${t[1]}" "${t[2]}" "${t[3]}" "${t[4]}"

      [[ $verbose ]] && printf "OLD-->%s | %s<--\n" $( color_glyph ${table[$i]} ) $( color_ext "${table[$i]}")

      unset append
      if [[ $Update_Existing ]]; then
        t[1]=${new[1]}
        t[2]=${new[2]}
        [[ ${new[3]} > 0 ]] && t[4]=${new[3]}

        table[$i]=${t[@]}
      fi

#     s=${table[$i]}      # For DEBUGGING
#     t=(${s})            # For DEBUGGING
#     printf "NEW cnt: %d : >%s< : < %s > : < %s > < %s > < %s >\n" ${#t[@]} "${t[0]}" "${t[1]}" "${t[2]}" "${t[3]}" "${t[4]}"
#     printf "cnt: %d\n" ${#t[@]}
      [[ $verbose  ]] && printf "NEW-->%s | %s<--\n" $( color_glyph ${table[$i]} ) $( color_ext "${table[$i]}")
#     printf "NEW-->%s | %s<--\n" $( color_glyph ${table[$i]} ) $(ocolor_ext "${table[$i]}")
#   else
#     printf "<%s>:<%s>:<%s>:<%s>\n" "A:${t[0]}" "B:${t[1]}" "C:${t[2]}" "D:${t[3]}"
#     printf "Goose-->%s | >%s=%s<--\n" $(color_glyph $s) "${t[0]}" "${new[0]}"
      [[ $verbose ]] && printf "\n"
    fi
  done

# [[ $append ]] && printf "Append %d: %s\n" ${#table[@]} "${new[0]}"
  if [[ $append ]]; then
    [[ ${new[3]} > 0 ]] && new[4]=${new[3]}
    new[3]=f62f
    table+=( "$( printf "%s " ${new[@]} )")
  fi
# [[ $append ]] && printf "Bppend %d: %s\n" ${#table[@]} "${new[0]}"
# [[ $verbose ]] && printf "\n\n"
}

# Load colors and icons from "cc()" commands in ls_*_generator.py
# field order is: foreground, background, icon, style
function load_icons {
file=$(grep -i 0x $ICON_SRC|grep "cc("|grep -v "^#" | sed 's/#.*$//;s/:/ /;s/cc(//;s/,/ /g;s/)//;s/other=\"//;s/\" *$//')

  while read -r line
  do
#   printf "LINE: [ %s ]\n" "$line"
    t=($line)
#   t[0]="${t[0]%\"}"     # remove leading
#   t[0]="${t[0]#\"}"     # and trailing quote symbols

#   printf "LOAD: <%s>:<%s>:<%s>:<%s>\n" "A:${t[0]}" "B:${t[1]}" "C:${t[2]}" "D:${t[3]}"
#   table+=( "$( printf "%s %s %s %s %s" ${t[0]} ${t[1]} ${t[2]} ${t[3]} ${t[4]} )")
    table+=( "$( printf "%s " "${t[@]}" )")     # enclose 't' in quotes to avoid wildcard expansion
#   table+=(${t[@]})
#   table+=("$line")
#   printf "CNT: %3d: %s\n" ${#table[@]} "${table[ ${#table[@]}-1] }"
  done <<< "$file"
}

# LS_COLORS does not have a field order, but defined by value
function load_ls_colors {

  while read -r line
  do
#   printf "LINE: [%s]\n" "$line"
    t=( "lab" 39 -1 0 )
    line=$( echo "$line" | sed 's/\*//' )
    l=($line)
    t[0]=${l[0]}
    for ent in "${l[@]:1}"; do
      (( ent+=0 ))              # force string to numeric, probably not necessary
      if [[ $ent -lt 30 ]]; then
        t[3]=$ent
      elif [[ $ent -lt 40 ]]; then
        t[1]=$ent
        (( t[1]-=30 ))
      else
        t[2]=$ent
        (( t[2]-=40 ))
      fi
    done

    case ${t[0]} in
      di) t[0]="xDIRECTORY"; ;;
      ln) t[0]="xLINK";      ;;
      pi) t[0]="xPIPE";      ;;
      so) t[0]="xSOCKET";    ;;
      do) t[0]="xDOOR";      ;;
      bd) t[0]="xBLOCKDEV";  ;;
      cd) t[0]="xCHARDEV";   ;;
      or) t[0]="xORPHAN";    ;;
      su) t[0]="xSETUID";    ;;
      sg) t[0]="xGSTICKY";   ;;    # sticky group?
      tw) t[0]="xUNKN";      ;;    # what is this?
      ex) t[0]="xEXEC";      ;;
    esac

#   x="${#t[@]}"
#   printf "T: < %s > < %s > < %s >  < %s >\n" "${t[0]}" "${t[1]}" "${t[2]}" "${t[3]}"

#   printf "LS_COLORS: %d: >%s:%s<\n" $x "$(color_ext "$t" )"  "${t[0]}"
    update_icon "${t[@]}"
#   table+=("$line")
done <<< "$( echo $LS_COLORS | tr '=;:' '  \n' )"

}

function output_ls_icons {
  for ent in "${table[@]}"; do
    a=(${ent})
#   printf "\nA: >%s< :%3d:%3d:%3d  <%s>\n" "${a[0]}" "${a[1]}" "${a[2]}" "${a[3]}" "${ent}"
    s=${ent}
    a[0]="${a[0]%\"}"     # remove leading
    a[0]="${a[0]#\"}"     # and trailing quote symbols
    printf "%s=%d" "${a[0]}" ${a[1]}
    printf   ";%d" ${a[2]}  # [[ ${a[2]} != -1 ]] &&
    printf   ";%d" ${a[4]}  # [[ ${a[4]} != "" ]] &&
    printf ";%s:" $(plain_character $s)
  done
  printf "\n"
#   printf "%s=%d;%d;%d;%lc;" "${ent[0]}" "${ent[1]}" "${ent[2]}" "${ent[4]}" "${ent[3]}"
}

function color_ext {
  c=($@)

  if [[ ${#c[@]} == 4 ]]; then
    if [[ ${c[2]} == -1 ]]; then
      printf "[38;5;%dm%s[0m"            ${c[1]}                 ${c[0]}
    else
       printf "[38;5;%d;48;5;%dm%s[0m"   ${c[1]} ${c[2]}         ${c[0]}
    fi
  elif [[ ${#c[@]} == 5 ]]; then
    if [[ ${c[2]} == -1 ]]; then
      printf "[38;5;%d;%dm%s[0m"         ${c[1]}         ${c[4]} ${c[0]}
    else
      printf "[38;5;%d;48;5;%d;%dm%s[0m" ${c[1]} ${c[2]} ${c[4]} ${c[0]}
    fi
  fi
}

function ocolor_ext {         # use old format escape codes
  c=($@)

  if [[ ${#c[@]} == 4 ]]; then     # probaby don't need
    if [[ ${c[2]} == -1 ]]; then
      printf "[%d;m%s[0m"   ${c[1]}         ${c[0]}
    else
      printf "[%d;%dm%s[0m" ${c[1]} ${c[2]} ${c[0]}
    fi
  elif [[ ${#c[@]} == 5 ]]; then
    if [[ ${c[2]} == -1 ]]; then
      printf "[%d;%dm%s[0m"    ${c[1]}         ${c[4]} ${c[0]}
    else
      printf "[%d;%d;%dm%s[0m" ${c[1]} ${c[2]} ${c[4]} ${c[0]}
    fi
  fi

}

function color_glyph {
  c=($@)

  if [[ ${#c[@]} == 4 ]]; then
    [[ ${c[2]} == -1 ]] \
      && printf "[38;5;%dm%s[0m"         ${c[1]}         $(charset ${c[3]}) \
      || printf "[38;5;%d;48;5;%dm%s[0m" ${c[1]} ${c[2]} $(charset ${c[3]})
  elif [[ ${#c[@]} == 5 ]]; then
    [[ ${c[2]} == -1 ]] \
      && printf "[38;5;%d;%dm%s[0m"         ${c[1]}         ${c[4]} $(charset ${c[3]}) \
      || printf "[38;5;%d;48;5;%d;%dm%s[0m" ${c[1]} ${c[2]} ${c[4]} $(charset ${c[3]})
  fi
}

function plain_character {
  c=($@)
  [[ ${#c[@]} -ge 4 ]] && printf $(charset ${c[3]} )
}

function test_char {
  c=($@)
  printf "test: %d : %d :%d\n" ${c[1]} ${c[2]} ${c[3]}
}

function usage {
  cat << THEND
usage: $0 [-all] [ext1] [ext2]...[extN]

  With no arguments, update glyphs
  With -all, show all glyphs
  With extensions, show just those glyphs

  Extensions  MUST begin with a period
  Directories MUST begin with a '^'
  Special files          with an 'x'
THEND
  exit 0;
}


# ----------------  Main ----------------- #

  if [[ $# -ge 1 ]]; then
    showglyphs=1
    [[ "$1" == "-all" ]] && shift
    [[ "$1" == "-h"  ]] && usage
  fi

# [[ $# -eq 1 ]] && verbose=1   # 2021-09-11 disable

  load_icons

  [[ $verbose ]] && printf "Loaded %d icons\n" ${#table[@]}
# show_icons
  load_ls_colors

# printf "cnt: %d\n" ${#table[@]}
# printf "opt: <%s>\n" "${table[opt]}"

# show_icons
# create_ls_icons

  [[ $showglyphs ]] && ( list_glyphs $@ ; exit 0 )  # exit doesn't happen
  [[ $showglyphs ]] || output_ls_icons    # do not show for showglyphs

exit 0;

for i in "${!table[@]}"; do
  s=${table[$i]}
  printf "Check->%s | %s <-\n" $(color_glyph $s) "$s"
  if [[ "$s" =~ "*sbin" ]]; then
    printf "Match-->%s | %s<--\n" "$s" $(color_glyph $s)
    t=(${s})
#   printf "icon: < %s:\x%x:%d >\n" "\x${t[3]}" ${t[3]} ${t[3]}
#   printf "icon: >%s<\n" $( color_glyph ${t[@]} )
    t[1]=666;
    t[2]=2;
    t[3]=0xFFF0
    table[$i]=${t[@]}
    printf "cnt: %d\n" ${#t[@]}
    printf "NEW-->%s<--\n" "${table[$i]}"
  fi
done
