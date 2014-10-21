#!/bin/bash

make
echo "===> clear du dmesg"
sudo dmesg -c > /dev/null
echo "===>  suppr du module"
sudo rmmod p_char
echo "===> affichage du dmesg + grep + grep exit"
dmesg | grep Buffer_circulaire | grep exit
echo "===>  insert module"
sudo insmod p_char.ko
echo "===> affichage du dmesg + grep"
dmesg | grep Buffer_circulaire
echo "===> regarder les messages en continus"

watch 'dmesg | grep Buffer | tail -n 10'


