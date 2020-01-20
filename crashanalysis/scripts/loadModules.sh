#!/bin/bash


PARAMS=""
MODE=load

while (( "$#" )); do
        case "$1" in
                -r|--mode)
                        MODE=$2
                        shift 2
                        ;;
                -m|--modules)
                        MODULES=$2
                        shift 2
                        ;;
		-p|--command)
			COMMAND=$2
			shift 2
			;;
                -h|--help)
                        echo "Usage: loadModules.sh [-r|--mode mode] [-m|--modules module-list]"
                        exit 1
                        ;;
                --)
                        shift
                        break
                        ;;
                -*|--*=)
                        echo "Error: Unsupported flag $1" >&2
                        exit 1
                        ;;
                *)
                        PARAMS="$PARAMS $1"
                        shift
                        ;;
        esac
done

eval set -- "$PARAMS"


source ./config/modulesinput.config


for filename in $(find . -name '*.ko'); 
do 
	echo "removing module $filename";
	rmmod $filename 2> /dev/null 
done


ALLMODULES=$(find . -name '*.ko')
if [ $MODE = load ]
then
	echo "Mode is $MODE. loading modules $MODULES"
	for module in $MODULES
	do
 		mymodule="$module".ko
		for path in $ALLMODULES
		do
 			if [[ $path = *"$mymodule" ]]
			then
				echo "Installing Module $path"
 				insmod $path mycommand=$COMMAND ${!module}
				break
			fi
		done
	done
fi
