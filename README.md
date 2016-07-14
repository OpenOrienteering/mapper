# OpenOrienteering Mapper

![Mapper Screenshot](http://openorienteering.github.io/mapper-manual/pages/images/main_window.png)

OpenOrienteering Mapper is an orienteering mapmaking program and provides
a free and open source alternative to existing commercial software.
OpenOrienteering Mapper runs on Android, Windows, Mac OS X and Linux.

 - [Mapper Homepage](http://www.openorienteering.org/apps/mapper/)
 - [Manual](http://www.openorienteering.org/mapper-manual/)
 - [Downloads](https://github.com/OpenOrienteering/mapper/releases)
 - [OpenOrienteering Blog](http://www.openorienteering.org/)


## Contributing

### Writing Code

For building Mapper from source see [`INSTALL.md`](https://github.com/OpenOrienteering/mapper/blob/master/INSTALL.md).
Pull requests are very welcome.

 - [Issue tracker](https://github.com/OpenOrienteering/mapper/issues)
 - [API documentation](http://www.openorienteering.org/api-docs/mapper/)
 - [Developer wiki](https://github.com/OpenOrienteering/mapper/wiki)


### Translating

The translations for Mapper are stored in `translations/OpenOrienteering_{LANGUAGE}.ts`.
The best way to edit those files is by using [Qt Linguist for translation](http://doc.qt.io/qt-5/linguist-translators.html).
The translations can also be edited with any XML editor.

Adding a new translation is done by making a new copy of `OpenOrienteering_template.ts`
and replacing `template` in the file name with the matching language code.
The new file also has to be added to `translations/CMakeLists.txt`.

Some strings such as basic buttons and colors get their translation from the
Qt Framework. For translating those see [Qt Localization](https://wiki.qt.io/Qt_Localization).


### Writing Documentation

The Mapper manual lives in its [own repository](https://github.com/OpenOrienteering/mapper-manual)
which contains all information for you to get started.


### Reporting Issues

Issues and possible improvements can be posted to our public [Ticket system](https://github.com/OpenOrienteering/mapper/issues).
Please make sure you provide all relevant information about your problem or idea.


## License

Mapper is licensed under the [GNU GENERAL PUBLIC LICENSE Version 3](https://www.gnu.org/licenses/gpl.html).
