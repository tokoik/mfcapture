package net.wakayama_u.tokoi.mfcapture

import android.Manifest
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.*
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import kotlinx.coroutines.delay
import java.io.File

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // ネイティブエンジンのアセット・ストレージ初期化
        NativeBridge.nativeInit(assets, filesDir.absolutePath)

        setContent {
            MaterialTheme(
                colorScheme = darkColorScheme(
                    primary = Color(0xFF64B5F6),
                    secondary = Color(0xFF81C784),
                    background = Color(0xFF121212),
                    surface = Color(0xFF1E1E1E),
                    error = Color(0xFFE57373)
                )
            ) {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    MainScreen()
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen() {
    val context = LocalContext.current
    var hasCameraPermission by remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.CAMERA
            ) == PackageManager.PERMISSION_GRANTED
        )
    }

    val permissionLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        hasCameraPermission = isGranted
    }

    LaunchedEffect(Unit) {
        if (!hasCameraPermission) {
            permissionLauncher.launch(Manifest.permission.CAMERA)
        }
    }

    // キャプチャ状態
    var isCapturing by remember { mutableStateOf(false) }

    // オーバーレイ（UIバー）の表示・非表示フラグ（画面タップでトグル）
    var showOverlay by remember { mutableStateOf(true) }

    // 設定ボトムシートの表示フラグ
    var showSettingsSheet by remember { mutableStateOf(false) }

    // 各種状態（一括定期ポーリング）
    var isDetectMarker by remember { mutableStateOf(true) }
    var isCalibrated by remember { mutableStateOf(false) }
    var undistortionMode by remember { mutableStateOf(0) }
    var markerLength by remember { mutableStateOf(5.0f) }
    var fps by remember { mutableStateOf(0.0f) }

    // 較正ファイル選択ピッカー (Storage Access Framework)
    val filePickerLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetContent()
    ) { uri: Uri? ->
        uri?.let {
            try {
                context.contentResolver.openInputStream(it)?.use { input ->
                    val destFile = File(context.filesDir, "calibration.json")
                    destFile.outputStream().use { output ->
                        input.copyTo(output)
                    }
                    val success = NativeBridge.nativeLoadCalibration(destFile.absolutePath)
                    if (success) {
                        isCalibrated = true
                        Toast.makeText(context, "較正データを読み込みました（姿勢推定が有効です）", Toast.LENGTH_SHORT).show()
                    } else {
                        Toast.makeText(context, "較正ファイルの解析に失敗しました", Toast.LENGTH_SHORT).show()
                    }
                }
            } catch (e: Exception) {
                Toast.makeText(context, "ファイルの読み込みエラー: ${e.message}", Toast.LENGTH_SHORT).show()
            }
        }
    }

    // キャプチャフレーム解像度 (アスペクト比計算用)
    var frameWidth by remember { mutableStateOf(1280) }
    var frameHeight by remember { mutableStateOf(720) }

    // 一括ポーリングによる UI 状態の同期 (Mutex 競合を解消)
    val statusArray = remember { FloatArray(8) }
    LaunchedEffect(Unit) {
        while (true) {
            NativeBridge.nativeGetStatus(statusArray)
            isCapturing = statusArray[0] > 0.5f
            isDetectMarker = statusArray[1] > 0.5f
            isCalibrated = statusArray[2] > 0.5f
            undistortionMode = statusArray[3].toInt()
            markerLength = statusArray[4]
            fps = statusArray[5]
            if (statusArray.size >= 8 && statusArray[6] > 0f && statusArray[7] > 0f) {
                frameWidth = statusArray[6].toInt()
                frameHeight = statusArray[7].toInt()
            }
            delay(100)
        }
    }

    BoxWithConstraints(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black),
        contentAlignment = Alignment.Center
    ) {
        if (hasCameraPermission) {
            // アスペクト比を維持した SurfaceView のレイアウトサイズを計算 (Contain 方式)
            val videoAspect = if (frameWidth > 0 && frameHeight > 0) {
                frameWidth.toFloat() / frameHeight.toFloat()
            } else {
                16f / 9f
            }

            val screenAspect = maxWidth / maxHeight
            val (surfaceWidth, surfaceHeight) = if (screenAspect > videoAspect) {
                // 画面の方が横長 -> 画面の高さに合わせる（左右に黒帯）
                Pair(maxHeight * videoAspect, maxHeight)
            } else {
                // 画面の方が縦長 -> 画面の幅に合わせる（上下に黒帯）
                Pair(maxWidth, maxWidth / videoAspect)
            }

            // 最背面: C++ / ANativeWindow 直接描画を行う SurfaceView (アスペクト比維持)
            AndroidView(
                factory = { ctx ->
                    SurfaceView(ctx).apply {
                        holder.addCallback(object : SurfaceHolder.Callback {
                            override fun surfaceCreated(holder: SurfaceHolder) {
                                NativeBridge.nativeSurfaceCreated(holder.surface)
                                isCapturing = NativeBridge.nativeIsCapturing()
                            }

                            override fun surfaceChanged(
                                holder: SurfaceHolder,
                                format: Int,
                                width: Int,
                                height: Int
                            ) {
                                NativeBridge.nativeSurfaceChanged(width, height)
                            }

                            override fun surfaceDestroyed(holder: SurfaceHolder) {
                                NativeBridge.nativeSurfaceDestroyed()
                                isCapturing = false
                            }
                        })
                    }
                },
                modifier = Modifier.size(surfaceWidth, surfaceHeight)
            )

            // 画面タップ検知用の透明レイヤー（SurfaceView の前面、UI コントロールの背面）
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .clickable(
                        interactionSource = remember { MutableInteractionSource() },
                        indication = null
                    ) {
                        showOverlay = !showOverlay
                    }
            )
        } else {
            // カメラ権限要求画面
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .background(Color.Black),
                contentAlignment = Alignment.Center
            ) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(
                        text = "カメラへのアクセス許可が必要です",
                        color = Color.White
                    )
                    Spacer(modifier = Modifier.height(16.dp))
                    Button(onClick = { permissionLauncher.launch(Manifest.permission.CAMERA) }) {
                        Text("許可をリクエスト")
                    }
                }
            }
        }

        // 最前面: アプリバー（半透明、タップでトグル表示）
        AnimatedVisibility(
            visible = showOverlay,
            enter = fadeIn() + slideInVertically(initialOffsetY = { -it }),
            exit = fadeOut() + slideOutVertically(targetOffsetY = { -it }),
            modifier = Modifier.align(Alignment.TopCenter)
        ) {
            TopAppBar(
                title = {
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Text("mfcapture", fontWeight = FontWeight.Bold, fontSize = 20.sp)

                        // マーカー検出バッジ
                        Surface(
                            color = if (isDetectMarker) Color(0xFF1976D2) else Color(0xFF424242),
                            shape = RoundedCornerShape(12.dp)
                        ) {
                            Text(
                                text = if (isDetectMarker) "マーカー検出中" else "マーカー非検出",
                                color = Color.White,
                                fontSize = 12.sp,
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp)
                            )
                        }

                        // 較正データ・姿勢推定バッジ
                        Surface(
                            color = if (isCalibrated) Color(0xFF2E7D32) else Color(0xFF424242),
                            shape = RoundedCornerShape(12.dp)
                        ) {
                            Text(
                                text = if (isCalibrated) "較正済 (姿勢推定ON)" else "未較正 (姿勢推定OFF)",
                                color = Color.White,
                                fontSize = 12.sp,
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp)
                            )
                        }

                        // 歪み補正バッジ
                        if (undistortionMode == 1) {
                            Surface(
                                color = Color(0xFF6A1B9A),
                                shape = RoundedCornerShape(12.dp)
                            ) {
                                Text(
                                    text = "歪み補正: OpenCV",
                                    color = Color.White,
                                    fontSize = 12.sp,
                                    modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp)
                                )
                            }
                        }

                        // FPS バッジ
                        if (fps > 0.0f) {
                            Surface(
                                color = Color(0xFF333333),
                                shape = RoundedCornerShape(12.dp)
                            ) {
                                Text(
                                    text = "%.1f fps".format(fps),
                                    color = Color.LightGray,
                                    fontSize = 12.sp,
                                    modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp)
                                )
                            }
                        }
                    }
                },
                actions = {
                    // マーカー検出トグルボタン
                    FilledTonalIconButton(
                        onClick = {
                            val newState = !isDetectMarker
                            NativeBridge.nativeSetDetectMarker(newState)
                            isDetectMarker = newState
                        },
                        colors = IconButtonDefaults.filledTonalIconButtonColors(
                            containerColor = if (isDetectMarker) MaterialTheme.colorScheme.primaryContainer else Color.DarkGray
                        )
                    ) {
                        Icon(
                            imageVector = Icons.Default.QrCodeScanner,
                            contentDescription = "マーカー検出切り替え",
                            tint = if (isDetectMarker) MaterialTheme.colorScheme.onPrimaryContainer else Color.White
                        )
                    }

                    Spacer(modifier = Modifier.width(4.dp))

                    // 較正ファイル読み込みボタン
                    IconButton(
                        onClick = { filePickerLauncher.launch("*/*") }
                    ) {
                        Icon(
                            imageVector = Icons.Default.FolderOpen,
                            contentDescription = "較正ファイルを読み込む",
                            tint = if (isCalibrated) Color(0xFF81C784) else Color.White
                        )
                    }

                    Spacer(modifier = Modifier.width(4.dp))

                    // 設定ボタン
                    IconButton(onClick = { showSettingsSheet = true }) {
                        Icon(
                            imageVector = Icons.Default.Settings,
                            contentDescription = "設定",
                            tint = Color.White
                        )
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = Color(0xCC1E1E1E),
                    titleContentColor = Color.White
                )
            )
        }
    }

    // --- 設定ボトムシート ---
    if (showSettingsSheet) {
        ModalBottomSheet(
            onDismissRequest = { showSettingsSheet = false },
            containerColor = MaterialTheme.colorScheme.surface
        ) {
            SettingsContent(
                isCalibrated = isCalibrated,
                onSelectCalibrationFile = {
                    showSettingsSheet = false
                    filePickerLauncher.launch("*/*")
                }
            )
        }
    }
}

//
// 設定ボトムシートの内容
//
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsContent(
    isCalibrated: Boolean,
    onSelectCalibrationFile: () -> Unit
) {
    val scrollState = rememberScrollState()

    var detectMarker by remember { mutableStateOf(NativeBridge.nativeIsDetectMarker()) }
    var markerLength by remember { mutableStateOf(NativeBridge.nativeGetMarkerLength()) }
    var undistortionMode by remember { mutableStateOf(NativeBridge.nativeGetUndistortionMode()) }

    var dictIndex by remember {
        val current = NativeBridge.nativeGetDictionaryName()
        val count = NativeBridge.nativeGetDictionaryCount()
        val names = (0 until count).map { NativeBridge.nativeGetDictionaryNameByIndex(it) }
        val idx = names.indexOf(current)
        mutableStateOf(if (idx >= 0) idx else 0)
    }
    val dictCount = remember { NativeBridge.nativeGetDictionaryCount() }
    val dictNames = remember {
        (0 until dictCount).map { NativeBridge.nativeGetDictionaryNameByIndex(it) }
    }
    var dictExpanded by remember { mutableStateOf(false) }

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 24.dp, vertical = 8.dp)
            .verticalScroll(scrollState)
    ) {
        Text(
            text = "mfcapture 設定",
            fontSize = 20.sp,
            fontWeight = FontWeight.Bold,
            color = Color.White
        )
        Spacer(modifier = Modifier.height(16.dp))

        // --- レンズ歪み補正 ---
        Text("レンズ歪み補正", fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
        Spacer(modifier = Modifier.height(4.dp))
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            FilterChip(
                selected = undistortionMode == 0,
                onClick = {
                    undistortionMode = 0
                    NativeBridge.nativeSetUndistortionMode(0)
                },
                label = { Text("補正なし") }
            )
            FilterChip(
                selected = undistortionMode == 1,
                onClick = {
                    if (isCalibrated) {
                        undistortionMode = 1
                        NativeBridge.nativeSetUndistortionMode(1)
                    }
                },
                enabled = isCalibrated,
                label = { Text("OpenCV 補正") }
            )
        }
        if (!isCalibrated) {
            Text(
                text = "※較正データ (calibration.json) を読み込むと OpenCV 補正が有効になります",
                fontSize = 12.sp,
                color = Color.Gray,
                modifier = Modifier.padding(top = 4.dp)
            )
        }

        Spacer(modifier = Modifier.height(16.dp))
        HorizontalDivider()
        Spacer(modifier = Modifier.height(16.dp))

        // --- ArUco Marker 認識設定 ---
        Text("ArUco Marker 認識", fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
        Spacer(modifier = Modifier.height(8.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text("マーカー検出")
            Switch(
                checked = detectMarker,
                onCheckedChange = {
                    detectMarker = it
                    NativeBridge.nativeSetDetectMarker(it)
                }
            )
        }

        Spacer(modifier = Modifier.height(8.dp))

        // 辞書選択ドロップダウン
        Text("マーカー辞書", fontSize = 12.sp, color = Color.Gray)
        Spacer(modifier = Modifier.height(4.dp))
        ExposedDropdownMenuBox(
            expanded = dictExpanded,
            onExpandedChange = { dictExpanded = !dictExpanded }
        ) {
            OutlinedTextField(
                value = dictNames.getOrElse(dictIndex) { "" },
                onValueChange = {},
                readOnly = true,
                trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = dictExpanded) },
                modifier = Modifier
                    .menuAnchor(MenuAnchorType.PrimaryNotEditable)
                    .fillMaxWidth()
            )
            ExposedDropdownMenu(
                expanded = dictExpanded,
                onDismissRequest = { dictExpanded = false }
            ) {
                dictNames.forEachIndexed { index, name ->
                    DropdownMenuItem(
                        text = { Text(name) },
                        onClick = {
                            dictIndex = index
                            NativeBridge.nativeSetDictionary(name)
                            dictExpanded = false
                        }
                    )
                }
            }
        }

        Spacer(modifier = Modifier.height(12.dp))

        // マーカー一辺の長さ
        Text("マーカー長: %.1f cm".format(markerLength), fontSize = 12.sp, color = Color.Gray)
        Slider(
            value = markerLength,
            onValueChange = {
                markerLength = it
                NativeBridge.nativeSetMarkerLength(it)
            },
            valueRange = 1.0f..30.0f
        )

        Spacer(modifier = Modifier.height(16.dp))
        HorizontalDivider()
        Spacer(modifier = Modifier.height(16.dp))

        // --- 較正ファイル管理 ---
        Text("較正ファイル (キャリブレーションデータ)", fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
        Spacer(modifier = Modifier.height(8.dp))
        Text(
            text = if (isCalibrated) "状態: 読み込み完了 (マーカー姿勢推定が有効)" else "状態: 未読み込み (枠とIDのみ表示)",
            fontSize = 12.sp,
            color = if (isCalibrated) Color(0xFF81C784) else Color(0xFFE57373)
        )
        Spacer(modifier = Modifier.height(8.dp))
        OutlinedButton(
            onClick = onSelectCalibrationFile,
            modifier = Modifier.fillMaxWidth()
        ) {
            Icon(Icons.Default.FolderOpen, contentDescription = null)
            Spacer(modifier = Modifier.width(8.dp))
            Text("較正ファイル (calibration.json) を選択")
        }

        Spacer(modifier = Modifier.height(24.dp))
    }
}
