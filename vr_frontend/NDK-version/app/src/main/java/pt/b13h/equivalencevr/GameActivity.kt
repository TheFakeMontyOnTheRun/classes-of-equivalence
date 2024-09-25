package pt.b13h.equivalencevr

import android.app.Presentation
import android.content.Context
import android.content.res.Configuration
import android.media.MediaRouter
import android.os.Build
import android.os.Bundle
import android.view.Display
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.view.ViewConfiguration
import android.view.ViewManager
import android.widget.Button
import android.widget.ImageButton
import android.widget.LinearLayout
import androidx.appcompat.app.AppCompatActivity
import com.google.vrtoolkit.cardboard.CardboardActivity

class GameActivity : CardboardActivity() {

    private var mView: GL2JNIView? = null
    private var running = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (savedInstanceState == null) {
            DerelictJNI.initAssets(resources.assets)
        }

        mView = GL2JNIView(this)
        setContentView(mView)
    }

    override fun onPause() {
        running = false
        mView!!.onPause()
        super.onPause()
    }

    override fun onResume() {
        super.onResume()
        mView!!.onResume()
    }

    override fun onDestroy() {
        DerelictJNI.onDestroy()
        super.onDestroy()
    }


    override fun onCardboardTrigger() {
        super.onCardboardTrigger()

        DerelictJNI.sendCommand('z')
    }

    override fun onKeyUp(keyCode: Int, event: KeyEvent?): Boolean {

        var toSend = '.'
        toSend = when (keyCode) {
            KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_W -> 'w'
            KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_S -> 's'
            KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_Q -> 'a'
            KeyEvent.KEYCODE_DPAD_RIGHT, KeyEvent.KEYCODE_E -> 'd'

            KeyEvent.KEYCODE_BUTTON_L1, KeyEvent.KEYCODE_A -> 'n'
            KeyEvent.KEYCODE_BUTTON_R1, KeyEvent.KEYCODE_D -> 'm'

            KeyEvent.KEYCODE_BUTTON_A, KeyEvent.KEYCODE_Z, KeyEvent.KEYCODE_DPAD_CENTER -> 'z'
            KeyEvent.KEYCODE_BUTTON_B, KeyEvent.KEYCODE_X-> 'x'
            KeyEvent.KEYCODE_BUTTON_C, KeyEvent.KEYCODE_BUTTON_Y, KeyEvent.KEYCODE_C-> 'c'
            KeyEvent.KEYCODE_BUTTON_START, KeyEvent.KEYCODE_BUTTON_X, KeyEvent.KEYCODE_ENTER -> 'q'
            else -> return super.onKeyUp(keyCode, event )
        }
        DerelictJNI.sendCommand(toSend)
        return true
    }

    override fun onBackPressed() {
        if (DerelictJNI.isOnMainMenu() == 1) {
            super.onBackPressed()
        } else {
            DerelictJNI.sendCommand('q')
        }
    }
}