QT += network

# Change the following line to point to the webthree-umbrella project.
WEBTHREE_UMBRELLA_ROOT=~/Projects/github/webthree-umbrella

INCLUDEPATH += . \
    $WEBTHREE_UMBRELLA_ROOT/libweb3core \
    $WEBTHREE_UMBRELLA_ROOT/libethereum \
    $WEBTHREE_UMBRELLA_ROOT/libethereum/libethash-cl

CONFIG += c++11 static

LIBS += -L/usr/local/lib \
    -lethashseal \
    -lethash-cl \
    -lboost_system \
    -ldevcore \
    -lboost_thread
