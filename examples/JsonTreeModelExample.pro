QT += core gui widgets

TARGET = JsonTreeModelExample
TEMPLATE = app

SOURCES += \
    main.cpp \
    widget.cpp \
    ../src/jsontreemodel.cpp

HEADERS += \
    widget.h \
    ../src/jsontreemodel.h

FORMS += \
    widget.ui
