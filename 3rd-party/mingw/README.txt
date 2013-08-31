This directory provides scripts for reproducibly creating archives of MinGW-w64
based on the mingw-builds [1] scripts and CMake. Given a particular
configuration, it will download all neccessary components, such as 7Zip,
MSYS, MinGW, mingw-builds and, either from upstream or as a
complete source archive created by a previous run, the actual sources.

Mingw-builds requires a host running the Windows operation system.

The modifications to mingw-builds aim to
- closely reproduce a particular MinGW binary archive which is distributed
  by MinGW builds and/or the Qt project® [2],
- reduce build issues triggered by the MSYS environment,
- update the location of upstream sources,
- speed up the compilation of GCC,
- update license documents in the created binary archive,
- mark the created GCC binaries as being "built by OpenOrienteering project",
  and consequently give OpenOrienteering's support page as bug URL.
- always create a corresponding source archive and a configuration.

All modifications are marked by "# Modification from OpenOrienteering".

Synopsis:

mkdir build
cd build
cmake -P PATH-TO-OPENORIENTEERING/3rd-party/mingw/configurations/CONFIGNAME.cmake
make build-from-upstream
make archives

This will download build prerequisites and sources from upstream and produce
source and binary archives plus an updated cmake configuration file in
msys\temp\mingw-archives. To shortcut all indiviudal source downloads/checkouts/
patching, you may build from the configuration's source archive by replacing
"make build-from-upstream" by:

make build-from-archive

THis will download a complete source archive from OpenOrienteering.

MSYS processes sometimes may fail due to the way some behaviours are
implemented for the Windows environment and how this interacts with other
software (anti-virus etc.) on the system. It may help to create the build
directory in the root directory of some drive. It even works if this drive is
another directory mapped to a drive letter by means of SUBST.
If the build stops at unexpected places, especially during build-from-upstream,
re-run the failed step a few times. There is some chance that the step may
succeed eventually. You may run CMake with -DKEEP_GOING=1 to have the script
retry the build until it succeeds or until you cancel it. If you use KEEP_GOING,
you still need to monitor the process for live locks caused by missing files
that should have been created by a previous step of the build process.


References

[1] http://sourceforge.net/projects/mingwbuilds/
[2] http://qt-project.org/

Qt is a registered trade mark of Digia plc and/or its subsidiaries.
