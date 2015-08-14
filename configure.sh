#!/bin/sh
if [ -z $1 ]
then
        echo "Usage: $0 [fpga-file]"
        exit 1
fi

bitfile=$1

GPIO_INITN=133
GPIO_DONE=134
GPIO_RESETN=135

gpio_num()
{
	name=$1
	eval echo \$GPIO_$name
}
gpio_export()
{
	name=$1
	direction=$2
	num=$(gpio_num $name)
	if [ ! -d /sys/class/gpio/gpio$num ]; then
		echo $num > /sys/class/gpio/export
	fi
	echo $direction > /sys/class/gpio/gpio$num/direction
}
gpio_get()
{
	name=$1
	cat /sys/class/gpio/gpio$(gpio_num $name)/value
}
gpio_set()
{
	name=$1
	value=$2
	echo $value > /sys/class/gpio/gpio$(gpio_num $name)/value
}
gpio_wait()
{
	name=$1
	value=$2
	echo -n "waiting for $name to be $value"
	for n in $(seq 40); do
		actual=$(gpio_get $name)
		if [ "$actual" = "$value" ]; then
			echo " ok"
			return
		fi
		sleep 0.1
		echo -n "."
	done
	echo " timeout"
	exit 1
}

set -e
echo "exporting GPIOs"
gpio_export INITN in
gpio_export DONE in
gpio_export RESETN out

echo "flipping reset"
gpio_set RESETN 0
gpio_set RESETN 1
gpio_wait INITN 1

echo "configuring FPGA"
dd if=${bitfile} of=/dev/spidev2.0 bs=32
gpio_wait DONE 1

echo "turning on clock to FPGA"
./devmem2 0x020c8160 w 0x00000D2B
