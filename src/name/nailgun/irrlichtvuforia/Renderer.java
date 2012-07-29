/*==============================================================================
            Copyright (c) 2012 QUALCOMM Austria Research Center GmbH.
            All Rights Reserved.
            Qualcomm Confidential and Proprietary
            
@file 
    ImageTargetsRenderer.java

@brief
    Sample for ImageTargets

==============================================================================*/


package name.nailgun.irrlichtvuforia;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.opengl.GLSurfaceView;
import android.util.Log;

import com.qualcomm.QCAR.QCAR;


/** The renderer class for the ImageTargets sample. */
public class Renderer implements GLSurfaceView.Renderer
{
    public boolean mIsActive = false;
	private Context mContext;
    
    /** Native function for initializing the renderer. */
    public native void initRendering(String apkPath);
    
    
    /** Native function to update the renderer. */
    public native void updateRendering(int width, int height);

    public Renderer(Context context) {
    	mContext = context;
    }
    
    /** Called when the surface is created or recreated. */
    public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {
        DebugLog.LOGD("GLRenderer::onSurfaceCreated");

		PackageManager packageManager = mContext.getPackageManager();
		ApplicationInfo appInfo = null;
		try {
			appInfo = packageManager.getApplicationInfo(mContext.getPackageName(), 0);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
        
		Log.e("QWE", appInfo.sourceDir);
        // Call native function to initialize rendering:
        initRendering(appInfo.sourceDir);
        
        // Call QCAR function to (re)initialize rendering after first use
        // or after OpenGL ES context was lost (e.g. after onPause/onResume):
        QCAR.onSurfaceCreated();
    }
    
    
    /** Called when the surface changed size. */
    public void onSurfaceChanged(GL10 gl, int width, int height)
    {
        DebugLog.LOGD("GLRenderer::onSurfaceChanged");
        
        // Call native function to update rendering when render surface parameters have changed:
        updateRendering(width, height);

        // Call QCAR function to handle render surface size changes:
        QCAR.onSurfaceChanged(width, height);
    }    
    
    
    /** The native render function. */    
    public native void renderFrame();
    
    
    /** Called to draw the current frame. */
    public void onDrawFrame(GL10 gl)
    {
        if (!mIsActive)
            return;

        // Call our native function to render content
        renderFrame();
    }
}
