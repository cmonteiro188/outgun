#!/bin/bash
#
# Compile script for Outgun 1.0.0, and may work in earlier versions.
# by ThOR27 - Thomaz de Oliveira dos Reis - thommy@globo.com
#

# This function was taken from the wineinstall script.
# for more information about what is wine www.winehq.org
function conf_yesno_answer {
  unset ANSWER
  while [ "$ANSWER" != 'yes' ] && [ "$ANSWER" != 'no' ]
  do {
    echo -n "$1"
    read ANSWER
  }
  done
}

function downandinstall {
	name=$1 
	compile=$2
	install=$3 
	siteurl=$4
	fileurl=$5
	filename=$6
	subdir=$7

	echo "Couldn't compile with $name, probabbly you don't have $name or $name-dev installed on your system"
	echo "you can try to download any package for your distribution in your distro repository" 
	echo "You can check at $siteurl"
	echo "But if you prefer, this script will try to download $name source, compile and install."
	echo "For the installation process you may need your root password"
	echo
	echo "Do you wan't the script to try downloading and installing $name?"

	conf_yesno_answer "(yes/no) "
 
	 if [ "$ANSWER" = 'yes' ]; 
	 then {
		
		if ! [ -f "$filename" ];
		then {
			wget "$fileurl"
		} fi
		
		if [ -f "$filename" ];
		 then {
			if [ "`tar -zxf "$filename" 2>&1`" == "" ];
			then {
				cd $subdir
				if { $compile; }
				then {
					if [ `whoami` = 'root' ]
					then {
						$install
					}
					else {
						if ! su root -c "$install"
						then {
							if ! su root -c "$install"
							then {
								echo
								echo "Incorrect password or problem installing hawkNL"
								exit 1
	          					} fi
						} fi
					} fi
					echo
					echo "$name compiled. Running the script again to see if everything is OK."
					echo
					cd ../
					sh ./compile.sh
				}
				else {
					echo "couldn't compile $name, see the messages and try to fix yourself"
				} fi 
			} 
			else {
				echo "Problems uncompressing $name, try downloading it again"
				rm -fr "$filename"
			} fi
		} 
		else {
			echo "Problems downloading the $name, check you internet connection and try again"
		} fi
	 
	}
	else {
		echo "OK, so install $name or $name-dev in your system and run this script again"
	} fi
}

function clean {
	rm -fr ./allegro.check.cpp
	rm -fr ./hawk.check.cpp
	rm -fr ./g++.check.cpp
	rm -fr ./allegro.check.bin
	rm -fr ./hawk.check.bin
	rm -fr ./g++.check.bin
}

echo
echo ===========================================================================================
echo "This script will try to compile outgun in your machine and their dependecies if needed"
echo "It's released under the GPL. Read docs for more info"
echo "WARNING: This script is still BETA version."
echo "Please, Report any errors to the FORUM at outgun.sf.net or by email at: thommy@globo.com"
echo ===========================================================================================
echo
echo "Cleaning files from a old configuration. . ."
clean;
echo
echo "Creating CPP files that will be needed for checking. . ."

#=====================================
# HawkNL Checker by Nix - Niko Ritari
#=====================================
cat > ./hawk.check.cpp <<EOF
#include <nl.h>
 
int main() {
    return nlInit() == NL_FALSE;
}

EOF
#=====================================

#=====================================
# Allegro Checker by Nix - Niko Ritari
#=====================================
cat > ./allegro.check.cpp <<EOF
#include <allegro.h>

int main() {
    (void)GFX_XWINDOWS_FULLSCREEN;
    allegro_init();
    return 0;
}

EOF
#=====================================

#=====================================
# g++ Checker by Nix - Niko Ritari
#=====================================
cat > ./g++.check.cpp <<EOF
#include <sstream>

using std::istringstream;
int main() {
        for (int i = 0; false; );
        for (int i = 0; false; );
        return 0;
}

EOF
#=====================================

echo
echo "Checking g++"
echo "Compiling. . ."
g++ g++.check.cpp -o g++.check.bin
if [ -f "./g++.check.bin" ];
then {
	echo "g++ compiled!"
	echo
	echo "Checking HawkNL"
	echo "Compiling. . ."
	
	g++ hawk.check.cpp -o hawk.check.bin -lNL -pthread

	if [ -f "./hawk.check.bin" ];
	then {
		echo "Compiled!"
		echo "Running. . ."
	
		if { ./hawk.check.bin; }
		then {
			echo "Running just fine. HawkNL is OK"
			echo 
		}	
		else {
  			echo "WARNING: Problems running a program compiled with HawkNL. Outgun should compile but will not run."
			echo "         Try to get a newer version of HawkNL (http://www.hawksoft.com/hawknl/) and test it again,"
			echo "         or symlink files for the filename that Outgun will be linked to."
			echo
		} fi
		echo
		echo "Checking Allegro"	
		echo "Compiling. . ."
			
		g++ allegro.check.cpp -o allegro.check.bin `allegro-config --libs`
			
		if [ -f "./allegro.check.bin" ];
		then {
			echo "Compiled!"
			echo "Running. . ."
				
			if [ "`./allegro.check.bin 2>&1`" == "" ]; 
			then {
				echo "Running just fine. Allegro is OK"
				echo
			}
			else {
		  		echo "WARNING: Problems running a program compiled with Allegro. Outgun should compile but will not run."
				echo "         Try to get a newer version of Allegro (http://alleg.sf.net/) and test it again,"
				echo "         or symlink files for the filename that Outgun will be linked to."
				echo
			} fi	
			
			echo "Now outgun is going to compiled, if there were no errors or warnings, you should be playing soon."
			echo "Not so soon, deependig of your machine speed, in my XP2200+ with 512 DDR 400, it takes 10 minutes to this process be completed"
			echo
			echo "Compiling Outgun. . ."
			cd ./src
			make LINUX=1
				
			if [ -f "../outgun" ];
			then {
				echo "Outgun compiled fine. You should run now by typing ./outgun"
				echo
				echo "Nice play!"
			}
			else {
				echo "I don't know how but outgun didn't compiled"
				echo "Check the errors message and try to find out yourself the reason and run this script again"
			} fi
					
		}
		else {
			downandinstall "Allegro" "./fix.sh unix&&./configure&&make" "make install" "http://alleg.sf.net/" "http://koti.mbnet.fi/outgun/dependencies/outgun_1.0_allegro_src.tar.gz" "./outgun_1.0_allegro_src.tar.gz" "./allegro-4.1.17/"
		} fi
	}
	else {
		downandinstall "hawkNL" "make -f makefile.linux" "make -f makefile.linux install" "http://www.hawksoft.com/hawknl/" "http://koti.mbnet.fi/outgun/dependencies/outgun_1.0_hawknl_src.tar.gz" "./outgun_1.0_hawknl_src.tar.gz" "./hawknl1.68/"
	} fi
}
else {
	echo "g++ not found in your system. Get it or, if you have, update it to a newer version"
} fi
