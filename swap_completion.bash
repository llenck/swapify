_swap() {
	CUR="${COMP_WORDS[COMP_CWORD]}"
	PREV="${COMP_WORDS[COMP_CWORD-1]}"

	case "$PREV" in
		-s | --socket)
			# list the names of all sockets that are not just <pid>.sock
			COMPREPLY=( \
				$(compgen -W "$( \
					\ls -1 /run/user/$UID/swapify 2>/dev/null \
					| grep -Ev "^[0-9]+\.sock" \
				)" -- "$CUR" ) \
			)
			return 0
			;;
		-a | --action)
			COMPREPLY=( $(compgen -W "swap unswap exit" -- "$CUR") )
			return 0
			;;
		-p | --pid)
			# list the pids that we can find sockets for
			COMPREPLY=( $(compgen -W "$( \
				\ls -1 /run/user/$UID/swapify 2>/dev/null \
				| grep -E "^[0-9]+\.sock" \
				| cut -d '.' -f 1 \
			)" -- "$CUR") )
			return 0
			;;
		-P | --socket-path)
			_filedir
			return 0
			;;
		*)
			COMPREPLY=( $(compgen \
				-W "-h -a -p -s -P --help --action --pid --socket --socket-path
				$(
					\ls -1 /run/user/$UID/swapify 2>/dev/null \
					| grep -E "^[0-9]+\.sock" \
					| cut -d '.' -f 1)" \
				-- "$CUR" ) \
			)
			return 0
		;;
	esac
}

complete -F _swap swap
complete -c swapify
