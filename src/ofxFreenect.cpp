//
//  ofxFreenect.cpp
//
//  Created by Tobias Ebsen on 26/03/14.
//
//

#include "ofxFreenect.h"

static ofxFreenectContext contextMain;

//--------------------------------------------------------------
ofxFreenectContext::ofxFreenectContext() {
    bInited = false;
}

//--------------------------------------------------------------
void ofxFreenectContext::init() {

    if (bInited) {
        ofLogWarning("ofxFreenectContext", "already initialized");
        return;
    }
    
    if (freenect_init(&f_ctx, NULL) < 0) {
        ofLogError("ofxFreenectContext","failed to initialize");
        return;
    }
    
    bInited = true;
}

//--------------------------------------------------------------
bool ofxFreenectContext::isInitialized() {
    return bInited;
}

//--------------------------------------------------------------
int ofxFreenectContext::numDevices() {
    if (bInited)
        return freenect_num_devices(f_ctx);
    else {
        ofLogError("ofxFreenectContext", "not initialized");
        return 0;
    }
}

//--------------------------------------------------------------
ofxFreenectDevice::ofxFreenectDevice() {
    bIsOpen = false;
    bIsFrameNewVideo = bIsFrameNewDepth = false;
    bNeedsUpdateVideo = bNeedsUpdateDepth = false;
    depthTable = new ofxFreenectDepthTable();
    depthTable->generateExponential();
}

//--------------------------------------------------------------
ofxFreenectDevice::~ofxFreenectDevice() {
    delete depthTable;
    close();
}

//--------------------------------------------------------------
void ofxFreenectDevice::open() {
    
    if (ofThread::isMainThread()) {
        if (!contextMain.isInitialized())
            contextMain.init();
    }
    
    if (isThreadRunning()) {
        ofLogError("ofxFreenectDevice", "open request has already been issued");
    }

    startThread(true, false);
}

//--------------------------------------------------------------
void ofxFreenectDevice::close() {
    
    if (isThreadRunning())
        stopThread();
}

//--------------------------------------------------------------
void ofxFreenectDevice::update() {
    
    if (bIsOpen) {
        if (!videoTexture.isAllocated()) {
            videoTexture.allocate(vmode.width, vmode.height, GL_RGB);
        }
        
        if (!depthTexture.isAllocated()) {
            depthTexture.allocate(dmode.width, dmode.height, GL_LUMINANCE16);
        }
    }
    
    if (bNeedsUpdateVideo) {
        if (this->lock()) {
            videoTexture.loadData(videoPixels);
            bNeedsUpdateVideo = false;
            bIsFrameNewVideo = true;
            this->unlock();
        }
    }
    
    if (bNeedsUpdateDepth) {
        if (this->lock()) {
            depthTable->apply(depthPixels.getPixels(), depthPixels.getWidth()*depthPixels.getHeight());
            depthTexture.loadData(depthPixels);
            bNeedsUpdateDepth = false;
            bIsFrameNewDepth = true;
            this->unlock();
        }
    }
}

//--------------------------------------------------------------
void ofxFreenectDevice::draw(float x, float y) {
    videoTexture.draw(x, y);
}

//--------------------------------------------------------------
void ofxFreenectDevice::draw(float x, float y, float w, float h) {
    videoTexture.draw(x, y, w, h);
}

//--------------------------------------------------------------
void ofxFreenectDevice::drawDepth(float x, float y) {
    depthTexture.draw(x, y);
}

//--------------------------------------------------------------
void ofxFreenectDevice::drawDepth(float x, float y, float w, float h) {
    depthTexture.draw(x, y, w, h);
}

//--------------------------------------------------------------
void ofxFreenectDevice::rgb_cb(freenect_device *dev, void *rgb, uint32_t timestamp) {
    
    ofxFreenectDevice* fdevice = (ofxFreenectDevice*)freenect_get_user(dev);
    if (fdevice != NULL) {
        fdevice->lock();
        swap(fdevice->videoPixels, fdevice->videoPixelsBack);
        freenect_set_video_buffer(dev, fdevice->videoPixelsBack.getPixels());
        fdevice->bNeedsUpdateVideo = true;
        fdevice->unlock();
    }
}

void ofxFreenectDevice::depth_cb(freenect_device *dev, void *v_depth, uint32_t timestamp) {
    
    ofxFreenectDevice* fdevice = (ofxFreenectDevice*)freenect_get_user(dev);
    if (fdevice != NULL) {
        fdevice->lock();
        swap(fdevice->depthPixels, fdevice->depthPixelsBack);
        freenect_set_depth_buffer(dev, fdevice->depthPixelsBack.getPixels());
        fdevice->bNeedsUpdateDepth = true;
        fdevice->unlock();
    }
}

//--------------------------------------------------------------
void ofxFreenectDevice::threadedFunction() {
    
    if (freenect_init(&f_ctx, NULL) < 0) {
        ofLogError("ofxFreenectContext", "init failed");
        return;
    }
    
    while (isThreadRunning()) {

        if (freenect_num_devices(f_ctx) > 0) {

            if (freenect_open_device(f_ctx, &f_dev, 0) < 0) {
                ofLogError("ofxFreenectDevice", "failed to open device");
                ofSleepMillis(1000);
            }
            else {
                bIsOpen = true;
                
                freenect_set_led(f_dev, LED_GREEN);
                freenect_set_user(f_dev, this);
                
                vmode = freenect_get_current_video_mode(f_dev);
                videoPixels.allocate(vmode.width, vmode.height, 3);
                videoPixels.set(0);
                videoPixelsBack.allocate(vmode.width, vmode.height, 3);
                freenect_set_video_buffer(f_dev, videoPixelsBack.getPixels());
                
                dmode = freenect_get_current_depth_mode(f_dev);
                depthPixels.allocate(dmode.width, dmode.height, 1);
                depthPixels.set(0);
                depthPixelsBack.allocate(dmode.width, dmode.height, 1);
                freenect_set_depth_buffer(f_dev, depthPixelsBack.getPixels());
                
                freenect_set_video_callback(f_dev, rgb_cb);
                freenect_set_depth_callback(f_dev, depth_cb);
                
                freenect_start_video(f_dev);
                ofSleepMillis(100);
                freenect_start_depth(f_dev);

                while (isThreadRunning() && freenect_process_events(f_ctx) >= 0) {}
                
                bIsOpen = false;
                
                freenect_stop_depth(f_dev);
                freenect_stop_video(f_dev);
                freenect_close_device(f_dev);
            }
        }
        else {
            ofLogNotice("ofxFreenectDevice", "no devices found");
            ofSleepMillis(1000);
        }
    }
}

//--------------------------------------------------------------
int ofxFreenectDevice::getWidth() {
    return vmode.width;
}
//--------------------------------------------------------------
int ofxFreenectDevice::getHeight() {
    return vmode.height;
}

//--------------------------------------------------------------
ofPixels & ofxFreenectDevice::getPixels() {
    return videoPixels;
}

//--------------------------------------------------------------
ofShortPixels & ofxFreenectDevice::getDepthPixels() {
    return depthPixels;
}

//--------------------------------------------------------------
ofTexture & ofxFreenectDevice::getTextureReference() {
    return videoTexture;
}

//--------------------------------------------------------------
ofTexture & ofxFreenectDevice::getTextureReferenceDepth() {
    return depthTexture;
}

//--------------------------------------------------------------
bool ofxFreenectDevice::isFrameNew() {
    return bIsFrameNewVideo | bIsFrameNewDepth;
}

//--------------------------------------------------------------
bool ofxFreenectDevice::isFrameNewVideo() {
    return bIsFrameNewVideo;
}

//--------------------------------------------------------------
bool ofxFreenectDevice::isFrameNewDepth() {
    return bIsFrameNewDepth;
}

//--------------------------------------------------------------
freenect_device *ofxFreenectDevice::getDevice() {
    return f_dev;
}

//--------------------------------------------------------------
void ofxFreenectDepthTable::apply(uint16_t *pixels, int count) {
    for (int i=0; i<count; i++) {
        pixels[i] = table[pixels[i]];
    }
}

//--------------------------------------------------------------
void ofxFreenectDepthTable::generateLinear() {
    for (int i=0; i<2048; i++) {
        table[i] = i * 0xffff / 2047;
    }
    table[2047] = 0;
}

//--------------------------------------------------------------
void ofxFreenectDepthTable::generateExponential(float power, float multiply) {
    for (int i=0; i<2048; i++) {
        float v = i / 2047.;
        table[i] = powf(v, power) * multiply * 0xffff;
    }
    table[2047] = 0;
}
