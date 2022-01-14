#!/bin/bash

# aovids '?' from expanding in a startup directory with a single letter file
set -o noglob

# charset f5ab f708 fbcf faf8    # shades, eye crossed, eye crossed, incognito
#
function load {

  cat << EOT | sed 's/#.*//;/^$/d;s/^ *//'
  M 124  fb4e # Modified
  A  22  f457 # Added
  R  88  f00d # Removed
# C           # Clean       ( not output )
  !  88  f5f8 # Missing     ( tracked, not present )
  ? 238  f29c # Not tracked
  I 232  f708 # Ignored
EOT
echo $var
}
function conv {
  for ((i=0; i<${#arr[@]}; (i+=3))); do
    printf "%s=%d;%s:" ${arr[$i]} ${arr[$i+1]} $(charset ${arr[$i+2]})
  done
}
function  load_hgicons () {
  local src=$HOME/.rc/etc/hg_colors.sh
  export HG_ICONS=$( bash $src )
}

# ----- Main ----

  arr=($(load))

  printf "%s\n" $(conv)

