package pt.b13h.equivalencevr

import android.content.Context
import android.opengl.GLES20
import android.util.AttributeSet
import com.google.vrtoolkit.cardboard.CardboardView
import com.google.vrtoolkit.cardboard.CardboardView.StereoRenderer
import com.google.vrtoolkit.cardboard.Eye
import com.google.vrtoolkit.cardboard.HeadTransform
import com.google.vrtoolkit.cardboard.Viewport
import javax.microedition.khronos.egl.EGLConfig

internal class GL2JNIView : CardboardView, StereoRenderer {

    private val forwardVector = FloatArray(3)

    override fun onNewFrame(headTransform: HeadTransform) {
        headTransform.getEulerAngles(forwardVector, 0)
        val xz: Float = extractAngleXZFromHeadtransform()
        DerelictJNI.setXZAngle(xz)

    }

    override fun onDrawEye(eye: Eye?) {

        if (eye == null) {
            return;
        }

        val lookAt: FloatArray = eye.eyeView

        DerelictJNI.startFrame()
        DerelictJNI.setPerspective(eye.getPerspective(0.1f, 1000.0f))
        DerelictJNI.setLookAtMatrix(lookAt)
        DerelictJNI.drawFrame()
    }

    override fun onFinishFrame(var1: Viewport?) {
        DerelictJNI.endFrame();
    }

    override fun onSurfaceChanged(var1: Int, var2: Int) {
        DerelictJNI.init(width, height)
    }

    override fun onSurfaceCreated(var1: EGLConfig?) {
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f)
    }

    override fun onRendererShutdown() {
    }

    constructor(context: Context) : super(context) {
        setRenderer(this)
    }

    constructor(context: Context, attrs: AttributeSet?) : super(context, attrs) {
        setRenderer(this)
    }

    private fun extractAngleXZFromHeadtransform(): Float {
        return ((forwardVector[1] * (180 / Math.PI)).toFloat())
    }
}