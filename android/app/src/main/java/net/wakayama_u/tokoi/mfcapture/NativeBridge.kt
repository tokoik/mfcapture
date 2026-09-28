package net.wakayama_u.tokoi.mfcapture

import android.content.res.AssetManager
import android.view.Surface

object NativeBridge {
    init {
        System.loadLibrary("opencv_world")
        System.loadLibrary("mfcapture")
    }

    external fun nativeInit(assetManager: AssetManager, internalPath: String)
    external fun nativeSurfaceCreated(surface: Surface)
    external fun nativeSurfaceChanged(width: Int, height: Int)
    external fun nativeSurfaceDestroyed()

    external fun nativeStartCapture(): Boolean
    external fun nativeStopCapture()
    external fun nativeIsCapturing(): Boolean

    external fun nativeIsDetectMarker(): Boolean
    external fun nativeSetDetectMarker(enabled: Boolean)

    external fun nativeGetMarkerLength(): Float
    external fun nativeSetMarkerLength(length: Float)

    external fun nativeGetDictionaryName(): String
    external fun nativeSetDictionary(name: String)
    external fun nativeGetDictionaryCount(): Int
    external fun nativeGetDictionaryNameByIndex(index: Int): String

    external fun nativeLoadCalibration(filename: String): Boolean
    external fun nativeIsCalibrationReady(): Boolean

    external fun nativeGetUndistortionMode(): Int
    external fun nativeSetUndistortionMode(mode: Int)

    external fun nativeGetStatus(outStatus: FloatArray)
}
