#!/bin/bash


if [ `id -u` -ne 0 ]; then
	echo "The script must be run as root! (you can use sudo)\n"
	exit 1
fi

echo "Checking for configuration file..."
if [ -e config/crashanalysis.config ]
then
	echo "configuration file exists..."
else
	echo "Please supply configuration file as crashanalysis.config in current directory"
	exit 1
fi

echo "Checking for Module handler file..."
if [ -e loadModules.sh ]
then
	echo "Module handler file exists..."
else
	echo "Module loader script missing..."
	exit 1
fi

echo "Checking for Modules input file..."
if [ -e config/modulesinput.config ]
then
	echo "Modules input file exists..."
else
	echo "Modules input config file missing..."
	exit 1
fi

count=`grep -c "kern.crit" /etc/rsyslog.d/50-default.conf`

if [ $count -gt 0 ] 
then
	echo "kern.crit logging is set. Checking filename..."
	count=`grep -c "crashanalysis.log" /etc/rsyslog.d/50-default.conf`
	if [ $count -gt 0 ] 
	then
		echo "Kernel logging for crashanalysis is all set"
	else
		echo "Modifying kern.crit logging to crashanalysis"
		sed -i 's/^kern.crit.*/kern.crit \t\t\t-\/var\/log\/crashanalysis.log/' /etc/rsyslog.d/50-default.conf
	fi
else
	echo "kern.crit logging is not set. Setting it now"
	echo -e "kern.crit \t\t\t-/var/log/crashanalysis.log" >> /etc/rsyslog.d/50-default.conf
fi
service rsyslog restart


source ./config/crashanalysis.config

echo "Application Start Command: $COMMAND"
echo "Tester Application Comm  : $TESTERAPPCOMMAND"
echo "Application status check : pgrep $PSNAME"
echo "Tester status check      : pgrep $PSTESTERNAME"
echo "PASS One Duration        : $PASS1DURATION"
echo "Number of Passes         : $NOOFPASSES"
echo "Modules for Pass 1       : $PASS1MODULES"
if [ $NOOFPASSES -gt 1 ]
then
	echo "Modules for Pass 2       : $PASS2MODULES"
	echo "Wait time before app kill: $WAITTIME"
fi

echo "============================"
echo "Starting Crash Analaysis ..."
echo "============================"


echo "======checking for modules availability======"
MODULELIST="$PASS1MODULES $PASS2MODULES"
for module in $MODULELIST
do
	mymodule="$module".ko
	if [ $(find . -name $mymodule) ]
	then
		echo "module $module is present..."
	else
		echo "module $module does not exist." 
		echo "Please place module in module directory and start again"
		exit 1
	fi
done
echo "========Requested modules available========"

echo "================PASS 1=================="



echo "Loading requested Modules..."


./loadModules.sh -r load -m $PASS1MODULES -p $PSNAME


echo "Starting Application..."
echo "The application and tester (if any)"
echo "will start now and will be terminated"
echo "after $PASS1DURATION seconds."


$COMMAND & $TESTERAPPCOMMAND &
for i in {1..$PASS1DURATION}
do
	echo "Waiting for Application to stop: $i out of $PASS1DURATION seconds passed"
	sleep 1
done
echo "Pass 1 duration timer expired. Stoping application..."
pkill -9 $PSNAME
pkill -9 $PSTESTERNAME

echo "Application has stopped..."
echo "Unloading Modules..."
./loadModules.sh -r unload -m $PASS1MODULES
echo "==============PASS 1 OVER=============="

if [ $NOOFPASSES -eq 1 ]
then 
	echo "Analysis over, Please check output at the "
	echo "end of /var/log/crashanalysis.log. Bye"
	exit 0
fi



echo "Starting second pass, Application can be killed"
echo "multiple times during this pass. Whole system can"
echo "become non-responsive as well, depending on state"
echo "of application"


echo "=================PASS 2================"





for module in $PASS2MODULES
do
	mymodule="$module".ko

	echo "Loading module $mymodule..."


	./loadModules.sh -r load -m $mymodule -p $PSNAME


	echo "Starting Application..."
	./tracer/src/./tracer $COMMAND &
	$TESTERAPPCOMMAND &

	for i in {1..$WAITTIME}
	do
		echo "Waiting for Application to stop: $i out of $WAITTIME seconds passed"
		sleep 1
	done

 	if [ $(pgrep $PSNAME -c) -gt 0 ]
	then
		pkill -9 $PSNAME
		pkill -9 $PSTESTERNAME
	fi


	echo "Application has stopped..."
	echo "Unloading Module $mymodule..."
	./loadModules.sh -r unload -m $mymodule
done
echo "==============PASS 2 OVER=============="
echo "Analysis over, Please check output at the "
echo "end of /var/log/crashanalysis.log. Bye"
exit 0

