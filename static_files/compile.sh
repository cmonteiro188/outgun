#!/bin/bash
#
# Compile script for Outgun 1.0.0, and may work in earlier versions.
# by ThOR27 - Thomaz de Oliveira dos Reis - thommy@globo.com

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

echo "This script will compile outgun in your machine"
echo "This script is still BETA version. And it's released under the GPL"
echo "Please, Report any errors to the FORUM at outgun.sf.net or by email at: thor27@gmail.com"
echo
echo Checking g++
echo Compiling. . .
g++ g++.check.cpp -o g++.check.bin
if [ -f "./g++.check.bin" ];
then {
	echo g++ compiled!
	echo
	echo Checking HawkNL
	echo Compiling. . .
	
	g++ hawknl.check.cpp -o hawk.check.bin -lNL -pthread

	if [ -f "./hawk.check.bin" ];
	then {
		echo Compiled!
		echo Running. . .
	
		if { ./hawk.check.bin; }
		then {
			echo "Running just fine. HawkNL is OK"
			echo 
		}	
		else {
  			echo "WARNING: Problems running a program compiled with HawkNL. Outgun should compile but will not run."
			echo "	       Try to get a newer version of HawkNL (http://www.hawksoft.com/hawknl/) and test it again,"
			echo "         or symlink files for the filename that Outgun will be linked to."
		} fi
		echo Checking Allegro	
		echo Compiling. . .
			
		g++ allegro.check.cpp -o allegro.check.bin `allegro-config --libs`
			
		if [ -f "./allegro.check.bin" ];
		then {
			echo Compiled!
			echo Running. . .
				
			if [ "`./allegro.check.bin 2>&1`" == "" ]; 
			then {
				echo "Running just fine. Allegro is OK"
				echo
			}
			else {
		  		echo "WARNING: Problems running a program compiled with Allegro. Outgun should compile but will not run."
				echo "	       Try to get a newer version of Allegro (http://alleg.sf.net/) and test it again,"
				echo "         or symlink files for the filename that Outgun will be linked to."
			} fi	
			
			echo "Now outgun is going to compiled, if there were no errors or warnings, you should be playing soon."
			echo "Not so soon, deependig of your machine speed, in my XP2200+ with 512 DDR 400, it takes 10 minutes to this process be completed"
			echo
			echo Compiling Outgun. . .
			cd ./src
			make LINUX=1
				
			if [ -f "../outgun" ];
			then {
				echo "Outgun compiled fine. You should run now by typing ./outgun"
				echo "Nice play"
			}
			else {
				echo "I don't know how but outgun didn't compiled"
				echo "Check the errors message and try to find out yourself the reason and run this script again"
			} fi
					
		}
		else {
			echo "Couldn't compile with Allegro, probabbly you don't have allegro or allegro-dev installed on your system"
			echo "you can try to download any package for your distribution in http://alleg.sf.net or on your distro repository"
			echo "But if you prefer, this script will try to download allegro source, compile and install."
			echo "For the installation process you may need your root password"
			echo
			echo "Do you wan't the script to try downloading and installing allegro?"
			conf_yesno_answer "(yes/no) "
			 if [ "$ANSWER" = 'yes' ]; 
			 then {
				 wget "http://koti.mbnet.fi/outgun/dependencies/outgun_1.0_allegro_src.tar.gz"
				 
			 }
			 else {
			 	echo "OK, so install allegro or allegro-dev in your system and run this script again"
			 } fi
			
		} fi
	}
	else {
			echo "Couldn't compile with HawkNL, probabbly you don't have hawknl or hawknl-dev installed on your system"
			echo "you can try to download any package for your distribution in your distro repository or the source code" 
			echo "at http://www.hawksoft.com/hawknl/"
			echo "But if you prefer, this script will try to download HawkNL source, compile and install."
			echo "For the installation process you may need your root password"
			echo
			echo "Do you wan't the script to try downloading and installing HawkNL?"
			
			conf_yesno_answer "(yes/no) "
			 
			 if [ "$ANSWER" = 'yes' ]; 
			 then {
				
				if ! [ -f "outgun_1.0_hawknl_src.tar.gz" ];
				then {
					wget "http://koti.mbnet.fi/outgun/dependencies/outgun_1.0_hawknl_src.tar.gz"
			 	}
				
				if [ -f "outgun_1.0_hawknl_src.tar.gz" ];
				 then {
					if [ "`tar -zxf outgun_1.0_hawknl_src.tar.gz 2>&1`" == "" ];
					then {
						cd ./hawknl1.68/
						if { make -f makefile.linux; }
						then {
							if [ `whoami` = 'root' ]
  							then {
								make -f makefile.linux install
							}
							else {
							      if ! su root -c "make -f makefile.linux install"
							      then {
      									if ! su root -c "make -f makefile.linux install"
							        	then {
										echo "Incorrect password"
										exit 1
             						         	}fi
								} fi
   							} fi

							echo
							echo "HawkNL compiled. Running the script again to see if everything is OK."
							echo
							cd ../
							sh ./compile.sh
						}
						else {
							echo "couldn't compile HawkNL, see the messages and try to fix yourself"
						} fi 
					} 
					else {
						echo "Problems uncompressing HawkNL, try downloading it again"
						rm -fr outgun_1.0_hawknl_src.tar.gz
					} fi
				} 
				else {
					echo "Problems downloading the HawkNL, check you internet connection and try again"
				} fi
			 
			 }
			 else {
			 	echo "OK, so install hawknl or hawknl-dev in your system and run this script again"
			 } fi
	} fi
}
else {
	echo "g++ not found in your system. Get it or, if you have, update it to a newer version"
} fi
