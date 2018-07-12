QT += core gui widgets

TARGET = JsonTreeModelExample
TEMPLATE = app

SOURCES += \
    main.cpp \
    jsonwidget.cpp \
    ../src/jsontreemodel.cpp

HEADERS += \
    jsonwidget.h \
    ../src/jsontreemodel.h

FORMS += \
    jsonwidget.ui
