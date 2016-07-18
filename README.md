# qtethminer
Pod that directly links the ethereum miner into your application.

# How to build
This pod assumes that you have cloned and build the cpp-ethereum webtree umbrella project and installed it system wide. Because this pod is linking directly against cpp-ethereum and the make install command of the cpp-ethereum project does not copy all headers, you also need to specify the root of the webthree umbrella project.
