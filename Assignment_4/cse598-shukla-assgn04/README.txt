give ip address in double quote as input to run the file. IP address of the host machine where 3D plane software is running. 
E.g. main_4 "169.254.5.52" 1
where main_4 is the output file name 
"169.254.5.52" is the ipaddress of the host machine which will run 3d plane simulator
1 is the command to select between integration based angles and 2 for kalman filter based angles.

on x-axis plane is capable of rotating between -180 to 180 with very less  less error.
on y-axis plane is capable of rotating between -90 to 90 from earth reference with very less error. it introduces error to x-axis if goes beyong -90 or 90.
on z-axis it varies between -180 to 180 very erronous and might change its pivot. 

X-axis - coming out of the screen.
y-axis - vertical axis.
and 
z-axis - horizontal axis.

fo me I2C transactions were taking 2ms because of which i could achieve the best performance at 4.65ms it cannot go beyond this.
therefore the best sampling rate that can be achieved by me is of 200 samples per second.

