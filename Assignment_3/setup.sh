echo 42 > /sys/class/gpio/export
echo 43 > /sys/class/gpio/export
echo 54 > /sys/class/gpio/export
echo 55 > /sys/class/gpio/export

echo "out" > /sys/class/gpio/gpio42/direction
echo "out" > /sys/class/gpio/gpio43/direction
echo "out" > /sys/class/gpio/gpio54/direction
echo "out" > /sys/class/gpio/gpio55/direction

echo 1 > /sys/class/gpio/gpio42/value
echo 1 > /sys/class/gpio/gpio43/value
echo 1 > /sys/class/gpio/gpio54/value
echo 1 > /sys/class/gpio/gpio55/value
