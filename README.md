Qt Data Tree Model
==================

This library provides a flexible way to display hierarchical data in a Qt C++
model. The current implementation supports JSON documents; it is quite feasible
to adapt the library to support QVariantList/QVariantMap trees and certain CBOR
trees too.

System Requirements
-------------------
* A C++11 compliant compiler
* A recent version of Qt 5 (tested on Qt 5.11)

Examples
--------
Use Qt Creator (or your preferred IDE) to open the _JsonTreeModelExample.pro_
from the [examples/](examples) folder.

Documentation
-------------
* To use this library, see _doc/build/API/html/index.html_
* To hack this library, see _doc/build/Implementation/html/index.html_

Usage
-----
Simply copy the source files as-is from [src/](src) into your own project.

Licensing
---------
Copyright (c) 2018 Sze Howe Koh <<szehowe.koh@gmail.com>>

The Qt Data Tree Model library is published under the Mozilla Public License
v2.0 (see [LICENSE.MPLv2](LICENSE.MPLv2)), while examples are published under
the MIT License (see [examples/LICENSE.MIT](examples/LICENSE.MIT)).
