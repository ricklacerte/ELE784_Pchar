#!/bin/bash
make clean
make
echo "===> clear du dmesg"
sudo dmesg -c > /dev/null
echo "===>  suppr du module"
sudo rmmod p_char
echo "===> affichage du dmesg + grep + grep exit"
dmesg | grep Buffer_circulaire | grep exit
echo "===>  insert module"
sudo insmod p_char.ko
echo "===> regarder les messages en continue"
watch 'dmesg | grep Buffer | tail -n 40'


