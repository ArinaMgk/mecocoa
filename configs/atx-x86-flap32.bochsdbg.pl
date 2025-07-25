#!/usr/bin/perl
# ASCII Perl(5.34.0) TAB4 LF
# Attribute: Ubuntu
# LastCheck: 20240216
# AllAuthor: @dosconio
# ModuTitle: Debug Script for Bochs x86 Emulator 2.7
# Copyright: Dosconio Mecocoa, BCD License Version 3

# Windows Environment
# my $ubin = $ENV{'ubinpath'}; #TODO
#TODO mov in Linux env
$a = "C:/Soft/Bochs-2.7/";
$floimg = "D:/bin/I686/mecocoa/mccax86.img";

print "### configuration file generated by source of Mecocoa\n";

##  print "# how much memory do you want to allocate to the emulated machine?\n";
##  print "megs = 32\n";

print "# ROM images\n";
# BIOS
print "romimage: file=\"${a}BIOS-bochs-latest\", address=0x00000000, options=none\n";
# VGA VIOS
print "vgaromimage: file=\"${a}VGABIOS-lgpl-latest\"\n";

print "# Disk\n";
print "floppya: type=1_44, 1_44=\"${floimg}\", status=inserted, write_protected=0\n";
print "ata0: enabled=true, ioaddr1=0x1f0, ioaddr2=0x3f0, irq=14\n";
print "ata0-master: type=disk, path=\"D:\\bin\\fixed.vhd\", mode=flat, cylinders=32, heads=2, spt=32, sect_size=512, model=\"Generic 1234\", biosdetect=auto, translation=lba\n";
print "ata0-slave: type=none\n";

print "# Bootstrap\n";
print "boot: floppy, disk\n";

print "# where to send log message\n";
print "log: -\n";

print "# clock (add this, or the system frequency maybe unexpected)\n";
print "clock: sync=realtime, time0=local, rtc_sync=1\n";

print "# keyboard\n";
print "keyboard: type=mf, serial_delay=250, paste_delay=100000, user_shortcut=none\n";

print "# mouse\n";
print "mouse: type=ps2, enabled=false, toggle=ctrl+mbutton\n";


##  private_colormap: enabled=0

##  config_interface: textconfig


print "\n"
