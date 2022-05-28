#!/bin/bash
# ***************************************************************************************
#     Copyright (c) 2015-2019
# ***************************************************************************************

# Default variables
HOST_ARCH=`uname -m | sed -e 's/i.86/i686/' -e 's/^armv.*/arm/'`
INSTALL_DIR_OVERWRITE=yes
LOG_PATH=/var/log/MVSDK

START_DIR=`dirname $0`
START_DIR=`cd $START_DIR; pwd`
USER_ID=`id -u`
CP_BIN=cp

CURRENT_KERNEL_VERSION=`uname -r`

#CentOS6.5 native kernel
LOWEST_KERNEL_MAJOR_VERSION=2
LOWEST_KERNEL_MINOR_VERSION=6
LOWEST_KERNEL_REVISION_VERSION=32

#Ubuntu18.10 native kernel
HIGHEST_KERNEL_MAJOR_VERSION=5
HIGHEST_KERNEL_MINOR_VERSION=0
HIGHEST_KERNEL_REVISION_VERSION=0

# Check required priviledge
if [ "$USER_ID" != "0" ]; then
	echo ""
	echo "Check execution permissions..........NG"
	echo "ERROR: You need to run the installer as superuser (root account)."
	echo ""	
	exit 1
else
	echo "Check execution permissions..........OK"
fi

#check arguments(*.run)
if [ "$4" = "" ]; then
	install=normal
elif [ $4 = "force" ]; then
	install=$4
else
	echo ""
	echo "ERROR: Arguments is invalid."
	echo ""
	exit 1
fi

#check platform 
if [ $HOST_ARCH != "x86_64" ] && [ $HOST_ARCH != "i686" ]; then
	echo ""
	echo "Check installation platform..........NG"
	echo "ERROR: The current installation package is only for the x86 architecture."
	echo "ERROR: The current architecture is $HOST_ARCH, please replace the installation for $HOST_ARCH."
	echo ""	
	exit 1
else
	echo "Check installation platform..........OK"
fi

#check rc.local
RC_LOCAL=rc.local
RC_LOCAL_LOCATION=/etc/$RC_LOCAL
RC_D_LOCAL_LOCATION=/etc/rc.d/$RC_LOCAL

#CentOS6.5 and Ubuntu use 'lsb_release' command to get $LINUX_OS and $LINUX_OS_VERSION
if [ ! -f "/etc/os-release" ]; then
	LINUX_OS=`lsb_release -d | awk '{print $2}'`
	if [ $LINUX_OS = "CentOS" ]; then
		SYSTEM=CentOS
		LINUX_OS_VERSION=`lsb_release -d | awk '{print $4}'`
		if [ ! -f $RC_D_LOCAL_LOCATION ]; then
			echo ""	
			echo "Check driver loading service file..........NG"
			echo "WARN: $RC_D_LOCAL_LOCATION does not exit."
			echo ""	
		else
			echo "Check driver loading service file..........OK"
		fi	
	elif [ $LINUX_OS = "Ubuntu" ]; then
		SYSTEM=Ubuntu
		LINUX_OS_VERSION=`lsb_release -d | awk '{print $3}'`
		if [ ! -f $RC_LOCAL_LOCATION ]; then
			echo ""	
			echo "Check driver loading service file..........NG"
			echo "WARN: $RC_LOCAL_LOCATION does not exit."
			echo ""	
		else
			echo "Check driver loading service file..........OK"
		fi
	fi
else
	LINUX_OS_TMP=`cat /etc/os-release | grep PRETTY_NAME | awk '{print $1}'`
	if [[ $LINUX_OS_TMP =~ "Ubuntu" ]]; then
		SYSTEM=Ubuntu
		LINUX_OS=`lsb_release -d | awk '{print $2}'`
		LINUX_OS_VERSION=`lsb_release -d | awk '{print $3}'`
		if [ ! -f $RC_LOCAL_LOCATION ]; then
			echo ""	
			echo "Check driver loading service file..........NG"
			echo "WARN: $RC_LOCAL_LOCATION does not exit."
			echo ""	
		else
			echo "Check driver loading service file..........OK"
		fi		
	elif [[ $LINUX_OS_TMP =~ "CentOS" ]]; then
		#CentOS7 use '/etc/os-release' to get $LINUX_OS_VERSION
		SYSTEM=CentOS
		LINUX_OS=CentOS
		LINUX_OS_VERSION=`cat /etc/os-release | grep PRETTY_NAME | awk '{print $3}'`
		if [ ! -f $RC_D_LOCAL_LOCATION ]; then
			echo ""	
			echo "Check driver loading service file..........NG"
			echo "WARN: $RC_D_LOCAL_LOCATION does not exit."
			echo ""	
		else
			echo "Check driver loading service file..........OK"
		fi			
	fi
fi

#check kernel driver sign
if [ $install = "normal" ]; then
	DRIVERSIGN=`grep "CONFIG_MODULE_SIG_FORCE" /boot/config-$CURRENT_KERNEL_VERSION`
	if [[ $DRIVERSIGN =~ "CONFIG_MODULE_SIG_FORCE=y" ]]; then	
		echo ""	
		echo "Check kernel driver sign..........NG"
		echo "ERROR: The current kernel allow only signed driver to be loaded."
		echo ""	
		exit 1
	else
		echo "Check kernel driver sign..........OK"
	fi
fi

#check kernel header file
if [ $install = "normal" ]; then	
	KERNEL_HEADER=/lib/modules/$CURRENT_KERNEL_VERSION/build
	if [ ! -d $KERNEL_HEADER ]; then
		echo ""
		echo "Check kernel header file..........NG"
		echo "ERROR: The kernel header($KERNEL_HEADER) corresponding to the kernel version is missing."
		echo ""	
		exit 1
	else
		echo "Check kernel header file..........OK"
	fi
fi

#check kernel version
if [ $install = "normal" ]; then
	MAJOR=$(echo ${CURRENT_KERNEL_VERSION%%.*})
	if [[ $MAJOR -lt $LOWEST_KERNEL_MAJOR_VERSION ]] || [[ $MAJOR -gt $HIGHEST_KERNEL_MAJOR_VERSION ]]; then
		echo ""
		echo "Check the major version of the current kernel($CURRENT_KERNEL_VERSION)..........NG"
		echo "WARN: The minimum kernel version supported by this installation package is $LOWEST_KERNEL_MAJOR_VERSION.$LOWEST_KERNEL_MINOR_VERSION.$LOWEST_KERNEL_REVISION_VERSION."
		echo "WARN: The maximum kernel version supported by this installation package is $HIGHEST_KERNEL_MAJOR_VERSION.$HIGHEST_KERNEL_MINOR_VERSION.$HIGHEST_KERNEL_REVISION_VERSION."
		echo ""		
		#exit 1
	else
		MINOR_TMP=$(echo ${CURRENT_KERNEL_VERSION#*.})
		MINOR=$(echo ${MINOR_TMP%%.*})
		if [[ $MAJOR -eq $LOWEST_KERNEL_MAJOR_VERSION && $MINOR -lt $LOWEST_KERNEL_MINOR_VERSION ]] || 
		[[ $MAJOR -eq $HIGHEST_KERNEL_MAJOR_VERSION && $MINOR -gt $HIGHEST_KERNEL_MINOR_VERSION ]]; then
			echo ""
			echo "Check the minor version of the current kernel($CURRENT_KERNEL_VERSION)..........NG"
			echo "WARN: The minimum kernel version supported by this installation package is $LOWEST_KERNEL_MAJOR_VERSION.$LOWEST_KERNEL_MINOR_VERSION.$LOWEST_KERNEL_REVISION_VERSION."
			echo "WARN: The maximum kernel version supported by this installation package is $HIGHEST_KERNEL_MAJOR_VERSION.$HIGHEST_KERNEL_MINOR_VERSION.$HIGHEST_KERNEL_REVISION_VERSION."
			echo ""			
			#exit 1
		else
			VERSION_TMP=$(echo ${MINOR_TMP#*.})
			VERSION=$(echo ${VERSION_TMP%%-*})
			if [[ $MAJOR -eq $LOWEST_KERNEL_MAJOR_VERSION && $VERSION -lt $LOWEST_KERNEL_REVISION_VERSION ]] || 
			[[ $MAJOR -eq $HIGHEST_KERNEL_MAJOR_VERSION && $VERSION -gt $HIGHEST_KERNEL_REVISION_VERSION ]]; then
				echo ""
				echo "Check the revision version of the current kernel($CURRENT_KERNEL_VERSION)..........NG"
				echo "WARN: The minimum kernel version supported by this installation package is $LOWEST_KERNEL_MAJOR_VERSION.$LOWEST_KERNEL_MINOR_VERSION.$LOWEST_KERNEL_REVISION_VERSION."
				echo "WARN: The maximum kernel version supported by this installation package is $HIGHEST_KERNEL_MAJOR_VERSION.$HIGHEST_KERNEL_MINOR_VERSION.$HIGHEST_KERNEL_REVISION_VERSION."
				echo ""					
				#exit 1
			else
				echo "Check kennel version..........OK"
			fi
		fi
	fi
else
	echo ""
	echo "Entry mandatory installation mode."
fi

HOST_ARCH=`uname -m | sed -e 's/i.86/i686/' -e 's/^armv.*/arm/'`

INSTALL_ROOT=/opt/$1/$2
INSTALL_ROOT_OLD=/opt/$1/MVViewer

# Initial screen
clear
echo "  ( $LINUX_OS-$LINUX_OS_VERSION-$HOST_ARCH )"
echo "========================================"
echo ""

#log path create
mkdir -p $LOG_PATH
chmod 777 -R $LOG_PATH

# This section is to remove the old directory of the SDK...
# We take care of moving the licenses...
if [ -d $INSTALL_ROOT/lib ] || [ -d $INSTALL_ROOT_OLD/lib ]; then
	echo -n "An older version of the MV SDK has been found. Replace [$INSTALL_DIR_OVERWRITE]? "
	read ANSWER
	until [ "$ANSWER" = "yes" -o "$ANSWER" = "no" -o "$ANSWER" = "" ]; do
		echo "Please type yes or no."
		echo -n "An older version of the MV SDK has been found. Replace  [$INSTALL_DIR_OVERWRITE]? "
		read ANSWER
	done
	
	if [ ! "$ANSWER" = "" ]; then
	INSTALL_DIR_OVERWRITE=$ANSWER
	fi
	
	if [ "$INSTALL_DIR_OVERWRITE" = "yes" ]; then
		# Cleanup the old installation if any
		if [ -f "$INSTALL_ROOT/bin/uninstall.sh" ]; then
			$INSTALL_ROOT/bin/uninstall.sh
			
		# Here delete the sub-folder because of the potential existence of a 
		# cross-compiled version of the SDK
		rm -rf $INSTALL_ROOT/bin
		rm -rf $INSTALL_ROOT/include
		rm -rf $INSTALL_ROOT/lib
		rm -rf $INSTALL_ROOT/module
		rm -rf $INSTALL_ROOT/share	
		
		elif [ -f "$INSTALL_ROOT_OLD/bin/uninstall.sh" ]; then
			$INSTALL_ROOT_OLD/bin/uninstall.sh

		# Here delete the sub-folder because of the potential existence of a 
		# cross-compiled version of the SDK
		rm -rf $INSTALL_ROOT_OLD/bin
		rm -rf $INSTALL_ROOT_OLD/include
		rm -rf $INSTALL_ROOT_OLD/lib
		rm -rf $INSTALL_ROOT_OLD/module
		rm -rf $INSTALL_ROOT_OLD/share
		
		fi
	fi
fi

echo ""
echo "Installing software on ${INSTALL_ROOT}."

mkdir -p $INSTALL_ROOT

#bin directory copy
if [ ! "`ls $START_DIR/bin/* 2>/dev/null`" = "" ]; then
	mkdir -p $INSTALL_ROOT/bin
	if [ $HOST_ARCH = "x86_64" ]; then
		cp -rf $START_DIR/bin/m64x86/* $INSTALL_ROOT/bin
		cp -rf $START_DIR/bin/common/* $INSTALL_ROOT/bin
	elif [ $HOST_ARCH = "i686" ]; then
		cp -rf $START_DIR/bin/m32x86/* $INSTALL_ROOT/bin
		cp -rf $START_DIR/bin/common/* $INSTALL_ROOT/bin
	fi
fi

#include directory copy
if [ ! "`ls $START_DIR/include/* 2>/dev/null`" = "" ]; then
	mkdir -p $INSTALL_ROOT/include
	cp -rf $START_DIR/include/* $INSTALL_ROOT/include
fi

#lib directory copy
if [ ! "`ls $START_DIR/lib/* 2>/dev/null`" = "" ]; then
	mkdir -p $INSTALL_ROOT/lib
	if [ $HOST_ARCH = "x86_64" ]; then
		cp -rf $START_DIR/lib/m64x86/* $INSTALL_ROOT/lib
		cp -rf $START_DIR/lib/common/* $INSTALL_ROOT/lib
	elif [ $HOST_ARCH = "i686" ]; then
		cp -rf $START_DIR/lib/m32x86/* $INSTALL_ROOT/lib
		cp -rf $START_DIR/lib/common/* $INSTALL_ROOT/lib
	fi
fi

#module directory copy
if [ ! "`ls $START_DIR/module/* 2>/dev/null`" = "" ]; then
	#USBDriver director copy
	mkdir -p $INSTALL_ROOT/module/USBDriver
	cp -rf $START_DIR/module/common/USBDriver/* $INSTALL_ROOT/module/USBDriver/
	mkdir -p $INSTALL_ROOT/module/USBDriver/RayDriverApi
	cp -rf $START_DIR/module/common/RayDriverApi/* $INSTALL_ROOT/module/USBDriver/RayDriverApi
	
	#GigEDriver director copy
	mkdir -p $INSTALL_ROOT/module/GigEDriver
	cp -rf $START_DIR/module/common/GigEDriver/* $INSTALL_ROOT/module/GigEDriver/
	mkdir -p $INSTALL_ROOT/module/GigEDriver/RayDriverApi
	cp -rf $START_DIR/module/common/RayDriverApi/* $INSTALL_ROOT/module/GigEDriver/RayDriverApi

	cp -rf $START_DIR/module/common/loadAllDrv.sh $INSTALL_ROOT/module/
	if [ $HOST_ARCH = "x86_64" ]; then
		cp -rf $START_DIR/module/m64x86/USBDriver/* $INSTALL_ROOT/module/USBDriver/libModule
		cp -rf $START_DIR/module/m64x86/GigEDriver/* $INSTALL_ROOT/module/GigEDriver/libModule
	elif [ $HOST_ARCH = "i686" ]; then
		cp -rf $START_DIR/module/m32x86/USBDriver/* $INSTALL_ROOT/module/USBDriver/libModule
		cp -rf $START_DIR/module/m32x86/GigEDriver/* $INSTALL_ROOT/module/GigEDriver/libModule
	fi
fi

#share directory copy
if [ ! "`ls $START_DIR/share/* 2>/dev/null`" = "" ]; then
	mkdir -p $INSTALL_ROOT/share
	cp -rf $START_DIR/share/* $INSTALL_ROOT/share
  
	if [ -d "$START_DIR/share/Python/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/Python/dll/x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/Python/dll/x64/libMVSDK.so
			cp -rf $INSTALL_ROOT/lib/libImageConvert.so $INSTALL_ROOT/share/Python/dll/x64
			cp -rf $INSTALL_ROOT/lib/GenICam/bin/Linux64_x64/* $INSTALL_ROOT/share/Python/dll/x64
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf $INSTALL_ROOT/share/Python/dll/x64
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/Python/dll/x86/libMVSDK.so
			cp -rf $INSTALL_ROOT/lib/libImageConvert.so $INSTALL_ROOT/share/Python/dll/x86
			cp -rf $INSTALL_ROOT/lib/GenICam/bin/Linux32_i86/* $INSTALL_ROOT/share/Python/dll/x86
		fi
	fi

	#####C Sample#####
	if [ -d "$START_DIR/share/C/CommPropAccess/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C/CommPropAccess/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/CommPropAccess/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C/CommPropAccess/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/CommPropAccess/depends/m32x86/libMVSDK.so
		fi
	fi

	if [ -d "$START_DIR/share/C/LineTrigger/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C/LineTrigger/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/LineTrigger/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C/LineTrigger/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/LineTrigger/depends/m32x86/libMVSDK.so
		fi
	fi

	if [ -d "$START_DIR/share/C/Record/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C/Record/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/Record/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C/Record/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/Record/depends/m32x86/libMVSDK.so
		fi
	fi
	
	if [ -d "$START_DIR/share/C/ResumeConnect/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C/ResumeConnect/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/ResumeConnect/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C/ResumeConnect/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/ResumeConnect/depends/m32x86/libMVSDK.so
		fi
	fi
	
	if [ -d "$START_DIR/share/C/SimpleSample/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C/SimpleSample/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/SimpleSample/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C/SimpleSample/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/SimpleSample/depends/m32x86/libMVSDK.so
		fi
	fi

	if [ -d "$START_DIR/share/C/SoftTrigger/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C/SoftTrigger/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/SoftTrigger/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C/SoftTrigger/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C/SoftTrigger/depends/m32x86/libMVSDK.so
		fi
	fi
	
	#####C++ Sample#####	
	if [ -d "$START_DIR/share/C++/CommPropAccess/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C++/CommPropAccess/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/CommPropAccess/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C++/CommPropAccess/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/CommPropAccess/depends/m32x86/libMVSDK.so
		fi
	fi

	if [ -d "$START_DIR/share/C++/LineTrigger/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C++/LineTrigger/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/LineTrigger/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C++/LineTrigger/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/LineTrigger/depends/m32x86/libMVSDK.so
		fi
	fi

	if [ -d "$START_DIR/share/C++/Record/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C++/Record/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/Record/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C++/Record/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/Record/depends/m32x86/libMVSDK.so
		fi
	fi
	
	if [ -d "$START_DIR/share/C++/ResumeConnect/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C++/ResumeConnect/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/ResumeConnect/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C++/ResumeConnect/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/ResumeConnect/depends/m32x86/libMVSDK.so
		fi
	fi

	if [ -d "$START_DIR/share/C++/SimpleSample/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C++/SimpleSample/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/SimpleSample/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C++/SimpleSample/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/SimpleSample/depends/m32x86/libMVSDK.so
		fi
	fi
	
	if [ -d "$START_DIR/share/C++/SoftwareTrigger/" ]; then
		if [ $HOST_ARCH = "x86_64" ]; then
			rm -rf $INSTALL_ROOT/share/C++/SoftwareTrigger/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/SoftwareTrigger/depends/m64x86/libMVSDK.so
		elif [ $HOST_ARCH = "i686" ]; then
			rm -rf INSTALL_ROOT/share/C++/SoftwareTrigger/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/C++/SoftwareTrigger/depends/m32x86/libMVSDK.so
		fi
	fi

	#####Qt Sample#####
	if [ -d "$START_DIR/share/Qt/ShowImage/" ]; then
		mkdir -p $INSTALL_ROOT/share/Qt/ShowImage/imageformats
		mkdir -p $INSTALL_ROOT/share/Qt/ShowImage/platforms	
		if [ $HOST_ARCH = "x86_64" ]; then
			cp -rf $START_DIR/bin/m64x86/imageformats/* $INSTALL_ROOT/share/Qt/ShowImage/imageformats
			cp -rf $START_DIR/bin/m64x86/platforms/* $INSTALL_ROOT/share/Qt/ShowImage/platforms
			
			cp -rf $INSTALL_ROOT/share/Qt/ShowImage/bin/m64x86/* $INSTALL_ROOT/share/Qt/ShowImage			
			rm -rf $INSTALL_ROOT/share/Qt/ShowImage/bin
			rm -rf $INSTALL_ROOT/share/Qt/ShowImage/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/Qt/ShowImage/depends/m64x86/libMVSDK.so
			cp -rf $INSTALL_ROOT/lib/libImageConvert.so $INSTALL_ROOT/share/Qt/ShowImage/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libVideoRender.so $INSTALL_ROOT/share/Qt/ShowImage/depends/m64x86
		elif [ $HOST_ARCH = "i686" ]; then
			cp -rf $START_DIR/bin/m32x86/imageformats/* $INSTALL_ROOT/share/Qt/ShowImage/imageformats
			cp -rf $START_DIR/bin/m32x86/platforms/* $INSTALL_ROOT/share/Qt/ShowImage/platforms
			
			cp -rf $INSTALL_ROOT/share/Qt/ShowImage/bin/m32x86/* $INSTALL_ROOT/share/Qt/ShowImage
			rm -rf $INSTALL_ROOT/share/Qt/ShowImage/bin
			rm -rf $INSTALL_ROOT/share/Qt/ShowImage/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/Qt/ShowImage/depends/m32x86/libMVSDK.so
			cp -rf $INSTALL_ROOT/lib/libImageConvert.so $INSTALL_ROOT/share/Qt/ShowImage/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libVideoRender.so $INSTALL_ROOT/share/Qt/ShowImage/depends/m32x86	
		fi
	fi

	if [ -d "$START_DIR/share/Qt/ShowQImage/" ]; then
		mkdir -p $INSTALL_ROOT/share/Qt/ShowQImage/imageformats
		mkdir -p $INSTALL_ROOT/share/Qt/ShowQImage/platforms	
		if [ $HOST_ARCH = "x86_64" ]; then
			cp -rf $START_DIR/bin/m64x86/imageformats/* $INSTALL_ROOT/share/Qt/ShowQImage/imageformats
			cp -rf $START_DIR/bin/m64x86/platforms/* $INSTALL_ROOT/share/Qt/ShowQImage/platforms
			
			cp -rf $INSTALL_ROOT/share/Qt/ShowQImage/bin/m64x86/* $INSTALL_ROOT/share/Qt/ShowQImage
			rm -rf $INSTALL_ROOT/share/Qt/ShowQImage/bin
			rm -rf $INSTALL_ROOT/share/Qt/ShowQImage/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/Qt/ShowQImage/depends/m64x86/libMVSDK.so
			cp -rf $INSTALL_ROOT/lib/libImageConvert.so $INSTALL_ROOT/share/Qt/ShowQImage/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libVideoRender.so $INSTALL_ROOT/share/Qt/ShowQImage/depends/m64x86
		elif [ $HOST_ARCH = "i686" ]; then
			cp -rf $START_DIR/bin/m32x86/imageformats/* $INSTALL_ROOT/share/Qt/ShowQImage/imageformats
			cp -rf $START_DIR/bin/m32x86/platforms/* $INSTALL_ROOT/share/Qt/ShowQImage/platforms
			
			cp -rf $INSTALL_ROOT/share/Qt/ShowQImage/bin/m32x86/* $INSTALL_ROOT/share/Qt/ShowQImage
			rm -rf $INSTALL_ROOT/share/Qt/ShowQImage/bin
			rm -rf $INSTALL_ROOT/share/Qt/ShowQImage/depends/m64x86
			cp -rf $INSTALL_ROOT/lib/libMVSDK.so.*.*.*.* $INSTALL_ROOT/share/Qt/ShowQImage/depends/m32x86/libMVSDK.so
			cp -rf $INSTALL_ROOT/lib/libImageConvert.so $INSTALL_ROOT/share/Qt/ShowQImage/depends/m32x86
			cp -rf $INSTALL_ROOT/lib/libVideoRender.so $INSTALL_ROOT/share/Qt/ShowQImage/depends/m32x86	
		fi
	fi
fi

if [ -d $INSTALL_ROOT/lib/ ]; then
	echo ""
	echo "MVSDK flexible connection"
	pushd $INSTALL_ROOT/lib/ >>/dev/null
	ln -s libMVSDK.so.*.*.*.* libMVSDK.so
	popd >>/dev/null
fi

if [ -d $INSTALL_ROOT/lib/Qt ]; then
	echo "Qt flexible connection"
	pushd $INSTALL_ROOT/lib/Qt >>/dev/null
	ln -s libicudata.so.54.1 libicudata.so.54
	ln -s libicui18n.so.54.1 libicui18n.so.54
	ln -s libicuuc.so.54.1 libicuuc.so.54

	ln -s libQt5Core.so.5.5.1 libQt5Core.so.5
	ln -s libQt5DBus.so.5.5.1 libQt5DBus.so.5
	ln -s libQt5Gui.so.5.5.1 libQt5Gui.so.5
	ln -s libQt5Network.so.5.5.1 libQt5Network.so.5
	ln -s libQt5Widgets.so.5.5.1 libQt5Widgets.so.5
	ln -s libQt5XcbQpa.so.5.5.1 libQt5XcbQpa.so.5
	ln -s libQt5Xml.so.5.5.1 libQt5Xml.so.5
	popd >>/dev/null 
fi

# check if we need to add the libraries to be added to the path
echo ""
echo -n "Add libraries to the path"
pushd $INSTALL_ROOT/bin >>/dev/null
./install.sh --install
popd >>/dev/null

# check if we need to open firewall source port 3956
echo -n "open firewall source port 3956"
iptables -I INPUT -p udp --sport 3956 -j ACCEPT

# Build the Universal Pro For Ethernet module when installing
if [ -d $INSTALL_ROOT/module/GigEDriver ]; then
	echo ""
	echo -n "Install MV Universal Pro For Ethernet"
	pushd $INSTALL_ROOT/module/GigEDriver >>/dev/null
	bash build.sh
	bash loadDrv.sh
	popd >>/dev/null 
fi

# Build the Universal Pro For U3V module when installing
if [ -d $INSTALL_ROOT/module/USBDriver ]; then    
	echo ""
	echo -n "Install MV Universal Pro For U3V"
	pushd $INSTALL_ROOT/module/USBDriver >>/dev/null
	bash build.sh
	#bash loadDrv.sh
	popd >>/dev/null
fi
    
# Ask to run the rp_filter configuration
echo ""
echo "The Linux network stack does not allow the delivery of outbound "
echo "packets by default."
echo ""
echo "For this reason, an interface cannot enumerate devices on a different "
echo "subnet if not on a default gateway. The installer can run a script "
echo "to change the default configuration and work around this issue. "
echo ""
echo -n "Do you want the installer to run the script [yes]?"
read ANSWER
until [ "$ANSWER" = "yes" -o "$ANSWER" = "no" -o "$ANSWER" = "" ]; do
	echo -n "Do you want the installer to run the script [yes]?"
	read ANSWER
done
if [ "$ANSWER" = "" ]; then
	ANSWER="yes"
fi

if [ "$ANSWER" = "yes" ]; then
	bash $INSTALL_ROOT/bin/set_rp_filter.sh --mode=0
else
	echo ""
	echo "INFO: This operation can be done later using the following script:"
	echo "$INSTALL_ROOT/bin/set_rp_filter.sh "
	echo ""
fi

echo "Installed on $INSTALL_ROOT"
echo "Installation complete."
echo ""
echo ""
