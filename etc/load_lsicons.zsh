# NOTE: Paths below will need to be updated for your installation
load_lsicons () {
	local src=$HOME/.rc/etc/ls_colors_generator.py 
	local icn=$HOME/.rc/etc/LS_ICONS.cfg 
	local prg=$HOME/.rc/etc/ls_icons_generator 
	if [[ $# -gt 0 ]]
	then
		$prg $@
	else
		[[ ! -e $icn ]] && $prg > $icn
		[[ $icn -ot $src ]] && $prg > $icn
		export LS_ICONS=$( cat $icn )  && printf "icons loaded\n"
	fi
}
