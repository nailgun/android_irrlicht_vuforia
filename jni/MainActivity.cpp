#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <GLES/gl.h>
#include <irrlicht.h>

#include <QCAR/QCAR.h>
#include <QCAR/CameraDevice.h>
#include <QCAR/Renderer.h>
#include <QCAR/VideoBackgroundConfig.h>
#include <QCAR/Trackable.h>
#include <QCAR/Tool.h>
#include <QCAR/Tracker.h>
#include <QCAR/TrackerManager.h>
#include <QCAR/ImageTracker.h>
#include <QCAR/CameraCalibration.h>
#include <QCAR/UpdateCallback.h>
#include <QCAR/DataSet.h>

#define LOG_TAG    "IRRLICHT_VUFORIA"
#define LOG(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#ifdef __cplusplus
extern "C"
{
#endif

// Screen dimensions:
unsigned int screenWidth        = 0;
unsigned int screenHeight       = 0;

// Indicates whether screen is in portrait (true) or landscape (false) mode
bool isActivityInPortraitMode   = false;

// The projection matrix used for rendering virtual objects:
QCAR::Matrix44F projectionMatrix;

// Constants:
static const float kObjectScale = 3.f;

QCAR::DataSet* dataSetStonesAndChips    = 0;
QCAR::DataSet* dataSetTarmac            = 0;

bool switchDataSetAsap          = false;

irr::IrrlichtDevice* mDevice = 0;
irr::video::IVideoDriver* mDriver = 0;
irr::scene::ISceneManager* mSceneManager = 0;
irr::scene::IDummyTransformationSceneNode* mTransformationNode = 0;
irr::scene::ICameraSceneNode* mCamera = 0;
irr::scene::ISceneNode* mNode = 0;

// Object to receive update callbacks from QCAR SDK
class MainActivity_UpdateCallback : public QCAR::UpdateCallback
{   
    virtual void QCAR_onUpdate(QCAR::State& /*state*/)
    {
        if (switchDataSetAsap)
        {
            switchDataSetAsap = false;

            // Get the image tracker:
            QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
            QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
                trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
            if (imageTracker == 0 || dataSetStonesAndChips == 0 || dataSetTarmac == 0 ||
                imageTracker->getActiveDataSet() == 0)
            {
                LOG("Failed to switch data set.");
                return;
            }
            
            if (imageTracker->getActiveDataSet() == dataSetStonesAndChips)
            {
                imageTracker->deactivateDataSet(dataSetStonesAndChips);
                imageTracker->activateDataSet(dataSetTarmac);
            }
            else
            {
                imageTracker->deactivateDataSet(dataSetTarmac);
                imageTracker->activateDataSet(dataSetStonesAndChips);
            }
        }
    }
};

MainActivity_UpdateCallback updateCallback;

JNIEXPORT int JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_getOpenGlEsVersionNative(JNIEnv *, jobject)
{
    return 1;
}


JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_setActivityPortraitMode(JNIEnv *, jobject, jboolean isPortrait)
{
    isActivityInPortraitMode = isPortrait;
}



JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_switchDatasetAsap(JNIEnv *, jobject)
{
    switchDataSetAsap = true;
}


JNIEXPORT int JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_initTracker(JNIEnv *, jobject)
{
    LOG("Java_name_nailgun_irrlichtvuforia_MainActivity_initTracker");
    
    // Initialize the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* tracker = trackerManager.initTracker(QCAR::Tracker::IMAGE_TRACKER);
    if (tracker == NULL)
    {
        LOG("Failed to initialize ImageTracker.");
        return 0;
    }

    LOG("Successfully initialized ImageTracker.");
    return 1;
}


JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_deinitTracker(JNIEnv *, jobject)
{
    LOG("Java_name_nailgun_irrlichtvuforia_MainActivity_deinitTracker");

    // Deinit the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    trackerManager.deinitTracker(QCAR::Tracker::IMAGE_TRACKER);
}


JNIEXPORT int JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_loadTrackerData(JNIEnv *, jobject)
{
    LOG("Java_name_nailgun_irrlichtvuforia_MainActivity_loadTrackerData");
    
    // Get the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
                    trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    if (imageTracker == NULL)
    {
        LOG("Failed to load tracking data set because the ImageTracker has not"
            " been initialized.");
        return 0;
    }

    // Create the data sets:
    dataSetStonesAndChips = imageTracker->createDataSet();
    if (dataSetStonesAndChips == 0)
    {
        LOG("Failed to create a new tracking data.");
        return 0;
    }

    dataSetTarmac = imageTracker->createDataSet();
    if (dataSetTarmac == 0)
    {
        LOG("Failed to create a new tracking data.");
        return 0;
    }

    // Load the data sets:
    if (!dataSetStonesAndChips->load("StonesAndChips.xml", QCAR::DataSet::STORAGE_APPRESOURCE))
    {
        LOG("Failed to load data set.");
        return 0;
    }

    if (!dataSetTarmac->load("Tarmac.xml", QCAR::DataSet::STORAGE_APPRESOURCE))
    {
        LOG("Failed to load data set.");
        return 0;
    }

    // Activate the data set:
    if (!imageTracker->activateDataSet(dataSetStonesAndChips))
    {
        LOG("Failed to activate data set.");
        return 0;
    }

    LOG("Successfully loaded and activated data set.");
    return 1;
}


JNIEXPORT int JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_destroyTrackerData(JNIEnv *, jobject)
{
    LOG("Java_name_nailgun_irrlichtvuforia_MainActivity_destroyTrackerData");

    // Get the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
        trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    if (imageTracker == NULL)
    {
        LOG("Failed to destroy the tracking data set because the ImageTracker has not"
            " been initialized.");
        return 0;
    }
    
    if (dataSetStonesAndChips != 0)
    {
        if (imageTracker->getActiveDataSet() == dataSetStonesAndChips &&
            !imageTracker->deactivateDataSet(dataSetStonesAndChips))
        {
            LOG("Failed to destroy the tracking data set StonesAndChips because the data set "
                "could not be deactivated.");
            return 0;
        }

        if (!imageTracker->destroyDataSet(dataSetStonesAndChips))
        {
            LOG("Failed to destroy the tracking data set StonesAndChips.");
            return 0;
        }

        LOG("Successfully destroyed the data set StonesAndChips.");
        dataSetStonesAndChips = 0;
    }

    if (dataSetTarmac != 0)
    {
        if (imageTracker->getActiveDataSet() == dataSetTarmac &&
            !imageTracker->deactivateDataSet(dataSetTarmac))
        {
            LOG("Failed to destroy the tracking data set Tarmac because the data set "
                "could not be deactivated.");
            return 0;
        }

        if (!imageTracker->destroyDataSet(dataSetTarmac))
        {
            LOG("Failed to destroy the tracking data set Tarmac.");
            return 0;
        }

        LOG("Successfully destroyed the data set Tarmac.");
        dataSetTarmac = 0;
    }

    return 1;
}


JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_onQCARInitializedNative(JNIEnv *, jobject)
{
    // Register the update callback where we handle the data set swap:
    QCAR::registerCallback(&updateCallback);

    // Comment in to enable tracking of up to 2 targets simultaneously and
    // split the work over multiple frames:
    // QCAR::setHint(QCAR::HINT_MAX_SIMULTANEOUS_IMAGE_TARGETS, 2);
    // QCAR::setHint(QCAR::HINT_IMAGE_TARGET_MULTI_FRAME_ENABLED, 1);
}

#define DONT_SAVE_STATE

JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_Renderer_renderFrame(JNIEnv *, jobject)
{
    QCAR::State state = QCAR::Renderer::getInstance().begin();
    QCAR::Renderer::getInstance().drawVideoBackground();
       
#ifndef DONT_SAVE_STATE
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
#endif

    // Did we find any trackables this frame?
    for(int tIdx = 0; tIdx < state.getNumActiveTrackables(); tIdx++)
    {
        // Get the trackable:
        const QCAR::Trackable* trackable = state.getActiveTrackable(tIdx);
        QCAR::Matrix44F modelViewMatrix =
            QCAR::Tool::convertPose2GLMatrix(trackable->getPose());        

        // Choose the texture based on the target name:
        int textureIndex;
        if (strcmp(trackable->getName(), "chips") == 0)
        {
            textureIndex = 0;
        }
        else if (strcmp(trackable->getName(), "stones") == 0)
        {
            textureIndex = 1;
        }
        else
        {
            textureIndex = 2;
        }

        if (!mDevice->run()) {
            // TODO: error
        }

        irr::core::matrix4& cameraMatrix = mTransformationNode->getRelativeTransformationMatrix();
        cameraMatrix.setM(modelViewMatrix.data);

        mDriver->beginScene(false, true);
        mSceneManager->drawAll();
        mDriver->endScene();
    }

#ifndef DONT_SAVE_STATE        
    glDisable(GL_DEPTH_TEST);

    glDisable(GL_TEXTURE_2D);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

    QCAR::Renderer::getInstance().end();
}


void
configureVideoBackground()
{
    // Get the default video mode:
    QCAR::CameraDevice& cameraDevice = QCAR::CameraDevice::getInstance();
    QCAR::VideoMode videoMode = cameraDevice.
                                getVideoMode(QCAR::CameraDevice::MODE_DEFAULT);


    // Configure the video background
    QCAR::VideoBackgroundConfig config;
    config.mEnabled = true;
    config.mSynchronous = true;
    config.mPosition.data[0] = 0.0f;
    config.mPosition.data[1] = 0.0f;
    
    if (isActivityInPortraitMode)
    {
        //LOG("configureVideoBackground PORTRAIT");
        config.mSize.data[0] = videoMode.mHeight
                                * (screenHeight / (float)videoMode.mWidth);
        config.mSize.data[1] = screenHeight;

        if(config.mSize.data[0] < screenWidth)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenWidth;
            config.mSize.data[1] = screenWidth * 
                              (videoMode.mWidth / (float)videoMode.mHeight);
        }
    }
    else
    {
        //LOG("configureVideoBackground LANDSCAPE");
        config.mSize.data[0] = screenWidth;
        config.mSize.data[1] = videoMode.mHeight
                            * (screenWidth / (float)videoMode.mWidth);

        if(config.mSize.data[1] < screenHeight)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenHeight
                                * (videoMode.mWidth / (float)videoMode.mHeight);
            config.mSize.data[1] = screenHeight;
        }
    }

    LOG("Configure Video Background : Video (%d,%d), Screen (%d,%d), mSize (%d,%d)", videoMode.mWidth, videoMode.mHeight, screenWidth, screenHeight, config.mSize.data[0], config.mSize.data[1]);

    // Set the config:
    QCAR::Renderer::getInstance().setVideoBackgroundConfig(config);
}


JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_initApplicationNative(
                            JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_name_nailgun_irrlichtvuforia_MainActivity_initApplicationNative");
    
    // Store screen dimensions
    screenWidth = width;
    screenHeight = height;

    LOG("Java_name_nailgun_irrlichtvuforia_MainActivity_initApplicationNative finished");
}


JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_deinitApplicationNative(
                                                        JNIEnv* env, jobject obj)
{
    LOG("Java_name_nailgun_irrlichtvuforia_MainActivity_deinitApplicationNative");
    mDevice->drop();
}


JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_startCamera(JNIEnv *,
                                                                         jobject)
{
    LOG("Java_name_nailgun_irrlichtvuforia_MainActivity_startCamera");

    // Initialize the camera:
    if (!QCAR::CameraDevice::getInstance().init())
        return;

    // Configure the video background
    configureVideoBackground();

    // Select the default mode:
    if (!QCAR::CameraDevice::getInstance().selectVideoMode(
                                QCAR::CameraDevice::MODE_DEFAULT))
        return;

    // Start the camera:
    if (!QCAR::CameraDevice::getInstance().start())
        return;

    // Uncomment to enable flash
    //if(QCAR::CameraDevice::getInstance().setFlashTorchMode(true))
    //	LOG("IMAGE TARGETS : enabled torch");

    // Uncomment to enable infinity focus mode, or any other supported focus mode
    // See CameraDevice.h for supported focus modes
    //if(QCAR::CameraDevice::getInstance().setFocusMode(QCAR::CameraDevice::FOCUS_MODE_INFINITY))
    //	LOG("IMAGE TARGETS : enabled infinity focus");

    // Start the tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* imageTracker = trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER);
    if(imageTracker != 0)
        imageTracker->start();
}


JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_stopCamera(JNIEnv *, jobject)
{
    LOG("Java_name_nailgun_irrlichtvuforia_MainActivity_stopCamera");

    // Stop the tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* imageTracker = trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER);
    if(imageTracker != 0)
        imageTracker->stop();
    
    QCAR::CameraDevice::getInstance().stop();
    QCAR::CameraDevice::getInstance().deinit();
}


JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_setProjectionMatrix(JNIEnv *, jobject)
{
    LOG("Java_name_nailgun_irrlichtvuforia_MainActivity_setProjectionMatrix");

    // Cache the projection matrix:
    const QCAR::CameraCalibration& cameraCalibration =
                                QCAR::CameraDevice::getInstance().getCameraCalibration();
    projectionMatrix = QCAR::Tool::getProjectionGL(cameraCalibration, 2.0f,
                                            2000.0f);
    if (mCamera) {
        irr::core::matrix4 proj;
        proj.setM(projectionMatrix.data);
        mCamera->setProjectionMatrix(proj);
    }
}


JNIEXPORT jboolean JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_activateFlash(JNIEnv*, jobject, jboolean flash)
{
    return QCAR::CameraDevice::getInstance().setFlashTorchMode((flash==JNI_TRUE)) ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jboolean JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_autofocus(JNIEnv*, jobject)
{
    return QCAR::CameraDevice::getInstance().setFocusMode(QCAR::CameraDevice::FOCUS_MODE_TRIGGERAUTO) ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jboolean JNICALL
Java_name_nailgun_irrlichtvuforia_MainActivity_setFocusMode(JNIEnv*, jobject, jint mode)
{
    int qcarFocusMode;

    switch ((int)mode)
    {
        case 0:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_NORMAL;
            break;
        
        case 1:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_CONTINUOUSAUTO;
            break;
            
        case 2:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_INFINITY;
            break;
            
        case 3:
            qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_MACRO;
            break;
    
        default:
            return JNI_FALSE;
    }
    
    return QCAR::CameraDevice::getInstance().setFocusMode(qcarFocusMode) ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_Renderer_initRendering(
                                                    JNIEnv* env, jobject obj, jstring apkPath)
{
    LOG("Java_name_nailgun_irrlichtvuforia_Renderer_initRendering");

    mDevice = irr::createDevice(irr::video::EDT_OGLES1,
        irr::core::dimension2d<irr::u32>(screenWidth, screenHeight), 32);

    {
        jboolean isCopy;
        const char* apkPathC = env->GetStringUTFChars(apkPath, &isCopy);
        LOG(apkPathC);
        mDevice->getFileSystem()->addZipFileArchive(apkPathC, false, true);
        env->ReleaseStringUTFChars(apkPath, apkPathC);
    }

    mDriver = mDevice->getVideoDriver();
    mSceneManager = mDevice->getSceneManager();

    mSceneManager->setAmbientLight(
        irr::video::SColorf(0.85f,0.85f,0.85f));
    mSceneManager->addLightSceneNode(NULL,
        irr::core::vector3df(-150, 200, -50),
        irr::video::SColorf(1.0f, 1.0f, 1.0f), 4000.0f);

    mTransformationNode = mSceneManager->addDummyTransformationSceneNode();
    mCamera = mSceneManager->addCameraSceneNode();
    irr::scene::IAnimatedMesh* mesh = mSceneManager->getMesh("dwarf.x");
    mNode = mSceneManager->addAnimatedMeshSceneNode(mesh, mTransformationNode);
    mNode->setMaterialFlag(irr::video::EMF_LIGHTING, true);
    mNode->setRotation(irr::core::vector3df(90, 0, 90));

    if (mCamera) {
        irr::core::matrix4 proj;
        proj.setM(projectionMatrix.data);
        mCamera->setProjectionMatrix(proj);
    }
}


JNIEXPORT void JNICALL
Java_name_nailgun_irrlichtvuforia_Renderer_updateRendering(
                        JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_name_nailgun_irrlichtvuforia_Renderer_updateRendering");

    // Update screen dimensions
    screenWidth = width;
    screenHeight = height;

    // Reconfigure the video background
    configureVideoBackground();
}

#ifdef __cplusplus
}
#endif
