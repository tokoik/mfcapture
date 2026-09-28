package net.wakayama_u.tokoi.mfcapture

import android.content.res.AssetManager
import android.view.Surface

object NativeBridge {
    init {
        try {
            System.loadLibrary("mfcapture")
        } catch (e: UnsatisfiedLinkError) {
            e.printStackTrace()
        }
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

    // フレーム解像度
    external fun nativeGetFrameWidth(): Int
    external fun nativeGetFrameHeight(): Int

    // カメラ解像度選択
    external fun nativeGetResolutionCount(): Int
    external fun nativeGetResolutionByIndex(index: Int): String
    external fun nativeGetCurrentResolution(): String
    external fun nativeSelectResolution(resolution: String): Boolean
}
