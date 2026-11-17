import gradio as gr
import numpy as np
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
import librosa
import scipy.io.wavfile as wavfile
from scipy.signal import butter, filtfilt
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import tempfile
import os
import time
from fastapi import FastAPI, File, UploadFile
from fastapi.responses import JSONResponse

# ============ CẤU HÌNH ============
FRAME_SIZE      = 800
FS              = 1000
N_SAMPLES       = 4000
FFT_SIZE        = 350
LUNET_PATH      = "best_model_v4.h5"
CLASSIFIER_PATH = "best_no_leak.keras"

BP_LOW  = 20
BP_HIGH = 400
HP_FREQ = 18

CLASS_NAMES = ['Normal', 'AS', 'MR', 'MS', 'MVP']
CLASS_DESC  = {
    'Normal': '✅ Tim bình thường',
    'AS':     '⚠️ Hẹp van động mạch chủ (Aortic Stenosis)',
    'MR':     '⚠️ Hở van hai lá (Mitral Regurgitation)',
    'MS':     '⚠️ Hẹp van hai lá (Mitral Stenosis)',
    'MVP':    '⚠️ Sa van hai lá (Mitral Valve Prolapse)',
}
CLASS_COLOR = {
    'Normal': '#2ecc71',
    'AS':     '#e74c3c',
    'MR':     '#e67e22',
    'MS':     '#e74c3c',
    'MVP':    '#f39c12',
}

# ============ SHARED STATE ============
latest_result = {
    "time":          "—",
    "prediction":    None,
    "description":   "Chờ ESP32 gửi dữ liệu...",
    "probs":         {},
    "waveform_path": None,
}

# ============ LU-NET ============
def _crop_or_pad(source, target):
    t_len = tf.shape(target)[1]
    s_len = tf.shape(source)[1]
    return tf.cond(
        s_len > t_len,
        lambda: source[:, :t_len, :],
        lambda: tf.pad(source, [[0, 0], [0, t_len - s_len], [0, 0]])
    )

def build_lunet(frame_size=800):
    K = 31
    enc_cfg      = [(16, 1), (32, 2), (32, 2), (64, 2), (64, 2)]
    bilstm_units = [8, 16, 16, 32, 32]
    inp = keras.Input(shape=(frame_size, 1))
    x   = inp
    lstm_skips = []
    for (cout, stride), units in zip(enc_cfg, bilstm_units):
        x    = layers.Conv1D(cout, K, strides=stride, padding='same')(x)
        x    = layers.ReLU()(x)
        skip = layers.Bidirectional(layers.LSTM(units, return_sequences=True))(x)
        lstm_skips.append(skip)
    x = layers.Conv1D(128, K, strides=2, padding='same')(x)
    x = layers.ReLU()(x)
    x = layers.Conv1D(64,  K, strides=1, padding='same')(x)
    x = layers.ReLU()(x)
    x = layers.UpSampling1D(2)(x)
    for i, f_out in enumerate([64, 32, 32, 32], 1):
        skip = lstm_skips[5 - i]
        skip = layers.Lambda(lambda args: _crop_or_pad(args[0], args[1]))([skip, x])
        x    = layers.Concatenate(axis=-1)([x, skip])
        x    = layers.Conv1D(f_out, K, strides=1, padding='same')(x)
        x    = layers.ReLU()(x)
        x    = layers.UpSampling1D(2)(x)
    skip = lstm_skips[0]
    skip = layers.Lambda(lambda args: _crop_or_pad(args[0], args[1]))([skip, x])
    x    = layers.Concatenate(axis=-1)([x, skip])
    out  = layers.Conv1D(1, K, strides=1, padding='same')(x)
    return keras.Model(inp, out)

# ============ LOAD MODEL ============
print("⏳ Loading LU-Net...")
lunet_model = tf.keras.models.load_model(LUNET_PATH, compile=False)
print("✅ LU-Net loaded!")

print("⏳ Loading Classifier...")
classifier_model = tf.keras.models.load_model(CLASSIFIER_PATH, compile=False) \
    if os.path.exists(CLASSIFIER_PATH) else None
print("✅ Classifier loaded!")

# ============ BỘ LỌC PCG ============
def pcg_filter(audio: np.ndarray, sr: int) -> np.ndarray:
    """DC removal → Bandpass 20–400 Hz → HPF 18 Hz"""
    sig  = audio - np.mean(audio)
    nyq  = sr / 2
    high = min(BP_HIGH, nyq * 0.95)
    b, a = butter(4, [BP_LOW / nyq, high / nyq], btype='band')
    sig  = filtfilt(b, a, sig)
    b, a = butter(4, HP_FREQ / nyq, btype='high')
    sig  = filtfilt(b, a, sig)
    return sig

# ============ HÀM XỬ LÝ ============
def denoise_audio(audio_path):
    audio, _ = librosa.load(audio_path, sr=FS, mono=True)
    audio     = audio.astype(np.float32)

    # Bước 1: Lọc PCG
    audio_filtered = pcg_filter(audio, FS).astype(np.float32)

    # Bước 2: LU-Net khử nhiễu
    max_val  = np.max(np.abs(audio_filtered)) + 1e-10
    norm     = audio_filtered / max_val
    n        = int(np.ceil(len(norm) / FRAME_SIZE))
    padded   = np.pad(norm, (0, n * FRAME_SIZE - len(norm)))
    frames   = padded.reshape(n, FRAME_SIZE, 1)
    out      = lunet_model.predict(frames, batch_size=32, verbose=0)
    denoised = out.flatten()[:len(audio_filtered)] * max_val

    return audio, audio_filtered, denoised

def classify_audio(denoised):
    if classifier_model is None:
        return "Error", np.zeros(len(CLASS_NAMES))
    audio = denoised.astype(np.float32)
    if len(audio) < N_SAMPLES:
        audio = np.pad(audio, (0, N_SAMPLES - len(audio)))
    else:
        audio = audio[:N_SAMPLES]
    fft   = np.abs(np.fft.rfft(audio))[:FFT_SIZE]
    fft   = fft / (np.max(fft) + 1e-8)
    X     = fft[np.newaxis, ..., np.newaxis]
    probs = classifier_model.predict(X, verbose=0)[0]
    pred  = CLASS_NAMES[int(np.argmax(probs))]
    return pred, probs

def plot_waveform(raw, filtered, denoised):
    t1 = np.arange(len(raw))      / FS
    t2 = np.arange(len(filtered)) / FS
    t3 = np.arange(len(denoised)) / FS

    fig, axes = plt.subplots(3, 1, figsize=(9, 6), sharex=False)

    axes[0].plot(t1, raw,      color='#888888', linewidth=0.6)
    axes[0].set_title("1. Tín hiệu gốc (ESP32)",          fontsize=10)
    axes[0].set_ylabel("Biên độ")
    axes[0].set_facecolor('#f8f9fa')

    axes[1].plot(t2, filtered, color='#E67E22', linewidth=0.6)
    axes[1].set_title("2. Sau lọc PCG (Bandpass + HPF)",  fontsize=10)
    axes[1].set_ylabel("Biên độ")
    axes[1].set_facecolor('#f8f9fa')

    axes[2].plot(t3, denoised, color='#1D9E75', linewidth=0.6)
    axes[2].set_title("3. Sau LU-Net (khử nhiễu sâu)",    fontsize=10)
    axes[2].set_ylabel("Biên độ")
    axes[2].set_xlabel("Thời gian (s)")
    axes[2].set_facecolor('#f8f9fa')

    fig.tight_layout()
    p = tempfile.NamedTemporaryFile(delete=False, suffix=".png")
    fig.savefig(p.name, dpi=120, bbox_inches='tight')
    plt.close(fig)
    return p.name

# ============ DASHBOARD REFRESH ============
def refresh_dashboard():
    r = latest_result
    if r["prediction"] is None:
        html = """
        <div style='text-align:center;padding:30px;background:#f0f4f8;
                    border-radius:12px;color:#888;font-size:16px;'>
            📡 Đang chờ ESP32 gửi dữ liệu...
        </div>"""
        return html, None, r["time"]

    color = CLASS_COLOR.get(r["prediction"], "#888")
    bars  = ""
    for name, p in r["probs"].items():
        w  = int(p * 220)
        c  = CLASS_COLOR.get(name, "#aaa")
        mk = " ◀" if name == r["prediction"] else ""
        bars += f"""
        <div style='display:flex;align-items:center;gap:8px;margin:5px 0;'>
            <span style='width:55px;font-weight:600;font-size:13px;'>{name}</span>
            <div style='background:{c};width:{w}px;height:16px;
                        border-radius:3px;min-width:3px;'></div>
            <span style='font-size:13px;color:#555;'>{p:.1%}{mk}</span>
        </div>"""

    html = f"""
    <div style='padding:18px;background:#fff;border-radius:12px;
                border-left:6px solid {color};
                box-shadow:0 2px 10px rgba(0,0,0,0.08);'>
        <div style='font-size:12px;color:#aaa;margin-bottom:8px;'>
            🕐 {r["time"]}
        </div>
        <div style='font-size:22px;font-weight:700;color:{color};margin-bottom:14px;'>
            {r["description"]}
        </div>
        <div style='font-size:13px;color:#666;margin-bottom:8px;'>
            📊 Xác suất từng loại bệnh:
        </div>
        {bars}
    </div>"""

    return html, r["waveform_path"], r["time"]

# ============ FASTAPI ============
fapi = FastAPI()

@fapi.post("/predict")
async def predict_api(file: UploadFile = File(...)):
    with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as tmp:
        tmp.write(await file.read())
        tmp_path = tmp.name
    try:
        raw, filtered, denoised = denoise_audio(tmp_path)
        pred, probs             = classify_audio(denoised)
        waveform_path           = plot_waveform(raw, filtered, denoised)

        latest_result.update({
            "time":          time.strftime("%H:%M:%S  %d/%m/%Y"),
            "prediction":    pred,
            "description":   CLASS_DESC.get(pred, pred),
            "probs":         {n: float(f"{p:.4f}") for n, p in zip(CLASS_NAMES, probs)},
            "waveform_path": waveform_path,
        })

        return JSONResponse({
            "status":        "success",
            "prediction":    pred,
            "description":   CLASS_DESC.get(pred, pred),
            "probabilities": latest_result["probs"],
        })
    except Exception as e:
        return JSONResponse({"status": "error", "message": str(e)}, status_code=500)
    finally:
        if os.path.exists(tmp_path):
            os.remove(tmp_path)

# ============ GRADIO DASHBOARD ============
with gr.Blocks(title="Heart Sound Dashboard") as demo:
    gr.Markdown("## 🫀 Heart Sound Diagnostic Dashboard")
    gr.Markdown(
        "**Pipeline:** ESP32 → PCG Filter → LU-Net → Classifier  \n"
        "Dashboard tự động cập nhật mỗi 3 giây."
    )
    with gr.Row():
        with gr.Column(scale=3):
            result_html = gr.HTML(
                value="<div style='text-align:center;padding:30px;"
                      "background:#f0f4f8;border-radius:12px;color:#888;'>"
                      "📡 Đang chờ ESP32 gửi dữ liệu...</div>"
            )
        with gr.Column(scale=1):
            last_time   = gr.Textbox(label="🕐 Lần đo gần nhất", value="—", interactive=False)
            btn_refresh = gr.Button("🔄 Làm mới", variant="secondary")

    waveform_img = gr.Image(
        label="📈 Waveform: Gốc → PCG Filter → LU-Net",
        type="filepath"
    )

    gr.Timer(value=3).tick(
        fn=refresh_dashboard,
        outputs=[result_html, waveform_img, last_time],
    )
    btn_refresh.click(
        fn=refresh_dashboard,
        outputs=[result_html, waveform_img, last_time],
    )

# ============ MOUNT & RUN ============
app = gr.mount_gradio_app(fapi, demo, path="/dashboard")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=7860)