load_lsicons is a [01;33mshell[m function [01;32mfrom[m [01;32m/Users/rwmitchell/.rc/.zshrc-aliases[m

]1337;File=name=eWVsbG93;size=113;inline=1;width=100%;height=5px;preserveAspectRatio=no:iVBORw0KGgoAAAANSUhEUgAAAAEAAAABAQMAAAAl21bKAAAAA1BMVEX//wCKxvRFAAAACklEQVQI12NgAAAAAgAB4iG8MwAAAABJRU5ErkJggg==
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

]1337;File=name=cmVk;size=113;inline=1;width=100%;height=5px;preserveAspectRatio=no:iVBORw0KGgoAAAANSUhEUgAAAAEAAAABAQMAAAAl21bKAAAAA1BMVEX/AAAZ4gk3AAAACklEQVQI12NgAAAAAgAB4iG8MwAAAABJRU5ErkJggg==
