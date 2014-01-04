proj-4.8.0 is quite old, and its build system needs modifications for Android support.

config.guess and config.sub were taken from
http://git.savannah.gnu.org/gitweb/?p=config.git;a=tree;h=3bfabc1475612c4425af0358c101f85f1b97fe64;hb=3bfabc1475612c4425af0358c101f85f1b97fe64
(Wed, 1 Jan 2014 00:37:33 +0000, commit 3bfabc1475612c4425af0358c101f85f1b97fe64).
Both files are under the GPL3v3.

configure.patch was created manually by applying the changes from
http://git.savannah.gnu.org/cgit/libtool.git/commit/?id=8eeeb00daef8c4f720c9b79a0cdb89225d9909b6
(2013-10-08 21:42:07, commit 8eeeb00daef8c4f720c9b79a0cdb89225d9909b6).
The original change modified a file named libtool.m4 which is part of GNU libtool
which is under GPLv2 or any later version.

