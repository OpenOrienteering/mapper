# OpenOrienteering Mapper

![Mapper Screenshot](http://openorienteering.github.io/mapper-manual/pages/images/main_window.png)

OpenOrienteering Mapper is an orienteering mapmaking program and provides a free and open source alternative to existing commercial software. OpenOrienteering Mapper runs on Android, Windows, Mac OS X and Linux.

 - [Manual](http://openorienteering.org/mapper-manual/)
 - [Downloads](https://github.com/OpenOrienteering/mapper/releases)
 - [Blog](http://openorienteering.github.io/)


## Contributing

### Writing Code

For building Mapper from source see [`INSTALL.md`](https://github.com/OpenOrienteering/mapper/blob/master/INSTALL.md). Pull requests are very welcome.

 - [Ticket system](https://github.com/OpenOrienteering/mapper/issues)
 - [API documentation](http://openorienteering.github.io/api-docs/mapper/)
 - [Unstable Builds](http://openorienteering.github.io/news/2015/mapper-unstable-packages/)
 - [Developer mailing list](https://lists.sourceforge.net/lists/listinfo/oorienteering-devel)


### Translating

The translations for Mapper are stored in `translations/OpenOrienteering_lang.ts`. The easiest way to edit those files is by using [Qt Linguist for translation](http://doc.qt.io/qt-5/linguist-translators.html). The translations can also be edited with any XML editor.

Adding a new translation is done by making a new copy of `OpenOrienteering_template.ts` and replacing `template` in the file name with the relevant language code. The new file also has to be added to `translations/CMakeLists.txt`.

Some strings such as basic buttons and colors has its translation within the Qt Framework, for translating those see [Qt Localization](https://wiki.qt.io/Qt_Localization).


### Writing Documentation

The Mapper manual lives in its [own repository](https://github.com/OpenOrienteering/mapper-manual) witch contains all information for you to get started.


### Reporting Issues

Issues and possible improvements can be posted to our public [Ticket system](https://github.com/OpenOrienteering/mapper/issues), please make sure you provide all relevant information about your problem or idea.


## License

Mapper is licensed under the [GNU GENERAL PUBLIC LICENSE Version 3](https://www.gnu.org/licenses/gpl.html).
