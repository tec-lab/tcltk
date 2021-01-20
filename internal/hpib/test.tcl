package require Hpib
hpib device pwm -interface lan\[192.168.1.53\]:hpib,13
pwm open
pwm write_read *IDN?
