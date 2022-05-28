#!/bin/bash
#-------------------------------------------------------------
#**
#file      install.sh
#brief     Galaxy embedded SDK install script 
#version   1.0.1908.8071
#date      2019.08.07
#/
#-------------------------------------------------------------

# Return 0 for 'n', 1 for 'y'
ChooseYorN() 
{
    local q="$1 (Y/n) : "
    while true; do
        read -p "$q" yn
        case $yn in
            [Yy]* ) return 0; ;;
            [Nn]* ) return 1; ;;
            "" ) return 0; ;;
            * );;
        esac
    done
}

#returns 0 when the user answered en
#returns 1 when the user answered cn
askLanguage() {
        local q="$1 (En/Cn) : "
        while true; do
            read -p "$q" language
            case $language in 
                [Ee]* ) return 1; ;;
                [Cc]* ) return 0; ;;
                "" ) return 1; ;;
                * );;
            esac
        done
}

#return 1 for '32', 0 for '64'
ChooseSDK() {
        local q="$1 (32/64) : "
        while true; do
            read -p "$q" sdk
            case $sdk in 
                32 ) return 1; ;;
                64 ) return 0; ;;
                "" ) return 0; ;;
                * );;
            esac
        done
}

SUDO="sudo "
CURRENT_PATH=`pwd`
ARCH_1="x86_64"
ARCH_2="x86"

if command -v getconf >/dev/null 2>&1; then
    if [ $(getconf LONG_BIT) == '64' ];
    then 
        ARCH=$ARCH_1
    else
        ARCH=$ARCH_2
    fi

    if ! ChooseYorN "Will install $ARCH SDK. Sure to continue?" ;
    then
       echo ""
       if ChooseSDK "Choose the SDK you will install, x86(32) or x64(64)?" ; 
       then 
           ARCH=$ARCH_1
       else
           ARCH=$ARCH_2
       fi
    fi
else
    if ChooseSDK "Choose the SDK you will install, x86(32) or x64(64)?" ; 
       then 
           ARCH=$ARCH_1
       else
           ARCH=$ARCH_2
    fi
fi

if [ $ARCH == $ARCH_1 ] ;
then 
    BIN_BIT=x64
else
    BIN_BIT=x32
fi 

ls | grep bin | grep -v $BIN_BIT | xargs rm -rf
mv bin_$BIN_BIT bin

LIB_PATH=$CURRENT_PATH/lib/$ARCH
cd "$CURRENT_PATH"/lib 
ls | grep -v -w $ARCH | xargs rm -rf
echo ""

INSTRUCTION_CN="C软件开发说明书.pdf"
INSTRUCTION_EN=""
FAQ_CN="Linux版SDK常见问题解答.pdf"
FAQ_EN=""

#returns 0 when the user answered en
#returns 1 when the user answered cn
if askLanguage "Choose language?" ; 
then
    rm -f "$CURRENT_PATH"/doc/C\ SDK\ Programming\ Reference\ Manual.pdf
	rm -f "$CURRENT_PATH"/doc/Linux\ SDK\ FAQs.pdf
    rm -f "$CURRENT_PATH"/bin/resource/settings/GalaxyView.ini
	rm -f "$CURRENT_PATH"/bin/GxGigeIPConfig
    mv "$CURRENT_PATH"/bin/resource/settings/GalaxyView_cn.ini "$CURRENT_PATH"/bin/resource/settings/GalaxyView.ini
	mv "$CURRENT_PATH"/bin/GxGigeIPConfig_cn "$CURRENT_PATH"/bin/GxGigeIPConfig
else
    rm -f "$CURRENT_PATH"/doc/"$INSTRUCTION_CN"
	rm -f "$CURRENT_PATH"/doc/"$FAQ_CN"
    rm -f "$CURRENT_PATH"/bin/resource/settings/GalaxyView_cn.ini
	rm -f "$CURRENT_PATH"/bin/GxGigeIPConfig_cn
fi

echo ""

USR_LIB=/usr/lib
LIB_GXIAPI=libgxiapi.so
LIB_GXU3VTL=GxU3VTL.cti
LIB_GXGVTL=GxGVTL.cti
CONFIG_PATH=$CURRENT_PATH/config

# Exit if user lib did not exist
if [ ! -d "$USR_LIB" ] ; 
then
    echo "***Libary file add failed*** : Libary directory '$USR_LIB' not found in your system."
    echo "Script Exit..."
    exit 1;
fi
# Exit if lib not found in package
if [ ! -e "$LIB_PATH/$LIB_GXIAPI" ] || [ ! -e "$LIB_PATH/$LIB_GXU3VTL" ] || [ ! -e "$LIB_PATH/$LIB_GXGVTL" ]; 
then
    echo "***Libary file add failed*** : Libary missing!"
    echo "Script Exit..."
    exit 1;
fi

# Override old library or not
if [ -e "$USR_LIB/$LIB_GXIAPI" ] || [ -e "$USR_LIB/$LIB_GXU3VTL" ] || [ -e "$USR_LIB/$LIB_GXGVTL" ] || [ -L "$USR_LIB/$LIB_GXIAPI" ] || [ -L "$USR_LIB/$LIB_GXU3VTL" ] || [ -L "$USR_LIB/$LIB_GXGVTL" ]; 
then      
    if ChooseYorN "$LIB_GXIAPI or $LIB_GXU3VTL or $LIB_GXGVTL already exist, are you sure to override?"; 
    then
    $SUDO rm -f "$USR_LIB"/$LIB_GXIAPI
    $SUDO rm -f "$USR_LIB"/$LIB_GXU3VTL
    $SUDO rm -f "$USR_LIB"/$LIB_GXGVTL
    $SUDO cp -af "$LIB_PATH"/$LIB_GXIAPI $USR_LIB
    $SUDO cp -af "$LIB_PATH"/$LIB_GXU3VTL $USR_LIB
    $SUDO cp -af "$LIB_PATH"/$LIB_GXGVTL $USR_LIB
    echo "Libary already override."
    echo ""
    else
    echo "Libary did not override."
    echo ""
    fi
else
    $SUDO cp -af "$LIB_PATH"/$LIB_GXIAPI $USR_LIB
    $SUDO cp -af "$LIB_PATH"/$LIB_GXU3VTL $USR_LIB
    $SUDO cp -af "$LIB_PATH"/$LIB_GXGVTL $USR_LIB
fi

# Copy udev file to get device access permission of normal user, take effect after replug.
UDEV_RULES_PATH=/etc/udev/rules.d
UDEV_RULES_FILE=99-galaxy-u3v.rules
if [ -e "$CONFIG_PATH"/$UDEV_RULES_FILE ] ;
then
    if [ -d "$UDEV_RULES_PATH" ] ; 
    then
       $SUDO cp -af "$CONFIG_PATH"/$UDEV_RULES_FILE $UDEV_RULES_PATH
	   $SUDO service udev reload &> /dev/null
	   $SUDO service udev restart &> /dev/null
	   $SUDO systemctl restart systemd-udevd.service &> /dev/null
    else
        echo "***Udev file add failed*** : Udev rules directory '$UDEV_RULES_PATH' not found in your system.
You may need to add your device access permission manually,
or you can use 'sudo' to get permission to access device." 
        echo "" 
    fi
else
    echo "***Udev file add failed*** : Udev rules file '$UDEV_RULES_FILE' missing!.
You may need to add your device access permission manually,
or you can use 'sudo' to get permission to access device." 
    echo "" 
fi

# Copy limits file to get increase thread priority permission, not particularly necessary, take effect after reboot.
LIMITS_CONF_PATH=/etc/security/limits.d
LIMITS_CONF_FILE=galaxy-limits.conf
if [ -e "$CONFIG_PATH"/$LIMITS_CONF_FILE ] ;
then
    if [ -d "$LIMITS_CONF_PATH" ] ; 
    then
        $SUDO cp -af "$CONFIG_PATH"/$LIMITS_CONF_FILE $LIMITS_CONF_PATH
    else
        echo "***Limits file add failed*** : Limits conf directory '$LIMITS_CONF_PATH' not found in your system."
        echo ""
    fi
else
    echo "***Limits file add failed*** : Limits conf file '$LIMITS_CONF_FILE' missing!." 
    echo ""
fi

EXTLINUX_CONF=/boot/extlinux/extlinux.conf
USB_MEM_MB=/sys/module/usbcore/parameters/usbfs_memory_mb
if [ -f $USB_MEM_MB ]; then
    $SUDO echo 1000 > $USB_MEM_MB
fi

if command -v update-grub >/dev/null 2>&1; then
    $SUDO sh "$CURRENT_PATH"/set_usbfs_memory_size.sh
elif [ -f $EXTLINUX_CONF ]; then
    if grep usbcore.usbfs_memory_mb $EXTLINUX_CONF > /dev/null; then
        echo "" > /dev/null
    else
        $SUDO sed -i 's/APPEND/APPEND usbcore.usbfs_memory_mb=1000/g' $EXTLINUX_CONF
    fi
fi  

cd "$CURRENT_PATH"
rm set_usbfs_memory_size.sh
rm -rf config
rm -rf lib

echo "-----------------------------------------------------------------"
echo "All configurations will take effect after the system is rebooted"
echo "If you don't want to reboot the system for a while"
echo "you will need to unplug and replug the camera."
echo "-----------------------------------------------------------------"
