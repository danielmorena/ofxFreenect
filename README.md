ofxFreenect
===========

Wrapping libfreenect for openFramworks

This is a simpler, less CPU-intensive, and re-connecting Kinect interface for openFrameworks.
The existing ofxKinect works great for most purposes, but uses a lot of CPU power updating depth pixels. ofxKinect also crashes the application when unplugging the Kinect. This is due to some threading restrictions in the underlying libusb.

ofxFreenect is an attempt to make a stabile, lightweight and simple interface for using Kinect with openFrameworks. Please feel free to contribute.

