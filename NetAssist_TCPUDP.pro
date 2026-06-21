QT += widgets network

CONFIG += c++17

# 项目元信息
TARGET = NetAssist
VERSION = 1.0.0
PROJECT_VERSION = VERSION

# Windows 图标（暂时注释，需要app.ico文件）
# win32 {
#     RC_ICONS = images/app.ico
# }

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    TcpClient.cpp \
    TcpServer.cpp \
    UDP.cpp \
    SendModel.cpp \
    protocal.cpp

HEADERS += \
    mainwindow.h \
    TcpClient.h \
    TcpServer.h \
    UDP.h \
    SendModel.h \
    protocal.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    res.qrc

# 默认规则
qnx: target.path = /tmp/{TARGET}/bin
else: unix:!android: target.path = /opt/{TARGET}/bin
!isEmpty(target.path): INSTALLS += target
