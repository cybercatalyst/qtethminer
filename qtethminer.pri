QT += network

# Change the following line to point to the webthree-umbrella project.
WEBTHREE_UMBRELLA_ROOT=/home/jacob/Projects/github/webthree-umbrella

INCLUDEPATH += . \
    $$WEBTHREE_UMBRELLA_ROOT/libweb3core \
    $$WEBTHREE_UMBRELLA_ROOT/libethereum \
    $$WEBTHREE_UMBRELLA_ROOT/libethereum/libethash-cl

CONFIG += c++11

INCLUDEPATH += \
    $$PWD

LIBS += \
    -L/usr/local/lib

LIBS += \
    -L../qtethminer -lqtethminer

# webthree-umbrella libs and dependencies
LIBS += \
    -lethashseal \
    -lethash-cl \
    -ldevcore \
    -lboost_system \
    -lboost_thread
