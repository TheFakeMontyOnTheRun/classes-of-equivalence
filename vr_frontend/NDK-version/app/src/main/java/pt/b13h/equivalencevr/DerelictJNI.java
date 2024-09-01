package pt.b13h.equivalencevr;

import android.content.res.AssetManager;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

public class DerelictJNI {
    static {
        System.loadLibrary("native-lib");
    }

    public static native void init(int width, int height);

    public static native void initAssets(AssetManager assetManager);

    public static native void sendCommand(char cmd);

    public static native int getSoundToPlay();

    public static native int isOnMainMenu();

    public static native void drawFrame();

    public static native void onDestroy();

    public static native void startFrame();

    public static native void setPerspective(float[] perspective);

    public static native void setLookAtMatrix(float[] lookAt);

    public static native void setXZAngle(float xzAngle);

    public static native void endFrame();
}
