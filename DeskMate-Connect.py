import asyncio
import json
import serial
import serial.tools.list_ports
import struct
import io
import time
from datetime import datetime
import os
import ctypes
from ctypes import wintypes

import numpy as np
from sklearn.cluster import KMeans
from PIL import Image
import winsdk.windows.media.control as wmc
import winsdk.windows.storage.streams as wss


# ─────────────────────────────────────────────────────────────────────────────
#  MEDIA CONTROLLER
# ─────────────────────────────────────────────────────────────────────────────

class MediaController:
    def __init__(self):
        self.session = None

    async def _update_session(self):
        manager = await wmc.GlobalSystemMediaTransportControlsSessionManager.request_async()
        self.session = manager.get_current_session()

    async def _ensure_session(self):
        await self._update_session()

    async def play_pause(self):
        await self._ensure_session()
        if self.session:
            await self.session.try_toggle_play_pause_async()

    async def next(self):
        await self._ensure_session()
        if self.session:
            await self.session.try_skip_next_async()

    async def previous(self):
        await self._ensure_session()
        if self.session:
            await self.session.try_skip_previous_async()

    async def fast_forward(self):
        await self._ensure_session()
        if self.session:
            await self.session.try_fast_forward_async()

    async def rewind(self):
        await self._ensure_session()
        if self.session:
            await self.session.try_rewind_async()

    async def seek(self, position_ms: int):
        await self._ensure_session()
        if self.session:
            await self.session.try_change_playback_position_async(position_ms)


mc = MediaController()


# ─────────────────────────────────────────────────────────────────────────────
#  CONFIG
# ─────────────────────────────────────────────────────────────────────────────

SERIAL_PORT    = "COM6"
BAUD_RATE      = 115200
ART_SIZE       = 150
APP_LOGO_SIZE  = 22
POLL_INTERVAL  = 0.1
last_real_pos = 0
last_update_time = time.time()

last_track  = None
last_minute = None
last_state  = None


# ─────────────────────────────────────────────────────────────────────────────
#  HELPERS
# ─────────────────────────────────────────────────────────────────────────────

def ms_to_mmss(ms: int) -> str:
    if ms <= 0:
        return "00:00"
    s = ms // 1000
    m = s // 60
    s = s % 60
    return f"{m:02d}:{s:02d}"


def timespan_to_ms(ts) -> int:
    if ts is None:
        return 0
    return int(ts.duration // 10_000)


# ─────────────────────────────────────────────────────────────────────────────
#  FIXED: get_position_and_duration now respects pause state
# ─────────────────────────────────────────────────────────────────────────────

async def get_position_and_duration(session, is_playing):
    global last_real_pos, last_update_time

    try:
        timeline = session.get_timeline_properties()

        pos = timeline.position
        start = timeline.start_time
        end = timeline.end_time

        pos_ms   = int(pos.total_seconds() * 1000) if pos else 0
        start_ms = int(start.total_seconds() * 1000) if start else 0
        end_ms   = int(end.total_seconds() * 1000) if end else 0

        dur_ms = max(0, end_ms - start_ms)
        pos_ms = max(0, pos_ms - start_ms)

        # SMTC updated → reset interpolation
        if pos_ms != last_real_pos:
            last_real_pos = pos_ms
            last_update_time = time.time()
            return pos_ms, dur_ms

        # FIX: do NOT interpolate when paused
        if not is_playing:
            last_update_time = time.time()
            return last_real_pos, dur_ms

        # Otherwise interpolate smoothly
        now = time.time()
        elapsed = int((now - last_update_time) * 1000)

        smooth_pos = last_real_pos + elapsed
        if smooth_pos > dur_ms:
            smooth_pos = dur_ms

        return smooth_pos, dur_ms

    except Exception as e:
        print("DEBUG ERROR:", e)
        return 0, 0


async def get_thumbnail_bytes(session) -> bytes | None:
    try:
        props = await session.try_get_media_properties_async()
        thumb = props.thumbnail
        if thumb is None:
            return None
        stream = await thumb.open_read_async()
        reader = wss.DataReader(stream)
        size = stream.size
        await reader.load_async(size)
        buf = bytearray(size)
        reader.read_bytes(buf)
        return bytes(buf)
    except Exception:
        return None


def get_dominant_color(jpeg_bytes: bytes, k: int = 5) -> tuple[int, int, int]:
    img = Image.open(io.BytesIO(jpeg_bytes)).convert("RGB")
    img = img.resize((64, 64), Image.LANCZOS)
    data = np.array(img).reshape(-1, 3)

    # ORIGINAL MAGIC FILTER (keeps vibrant colors)
    mask = np.logical_and(data > 20, data < 235).all(axis=1)
    filtered = data[mask] if mask.sum() > 50 else data

    kmeans = KMeans(n_clusters=k, n_init="auto").fit(filtered)
    colors = kmeans.cluster_centers_
    counts = np.bincount(kmeans.labels_)

    dom = colors[counts.argmax()]
    r, g, b = int(dom[0]), int(dom[1]), int(dom[2])

    # DEBUG PRINT — SEE EXACT COLOR PYTHON IS SENDING
    color565 = rgb888_to_565(r, g, b)
    print(f"[DOM] RGB=({r},{g},{b})  →  RGB565=0x{color565:04X}")

    return r, g, b


def rgb888_to_565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def resize_to_jpeg(raw: bytes, size: int):
    if not raw:
        return None, 0

    img = Image.open(io.BytesIO(raw)).convert("RGB")
    img = img.resize((size, size), Image.LANCZOS)

    r, g, b = get_dominant_color(raw)
    color565 = rgb888_to_565(r, g, b)

    out = io.BytesIO()
    img.save(out, format="JPEG", quality=80)
    return out.getvalue(), color565


# ─────────────────────────────────────────────────────────────────────────────
#  APP ICON EXTRACTION → ARGB8888
# ─────────────────────────────────────────────────────────────────────────────

SHGFI_ICON = 0x000000100
SHGFI_LARGEICON = 0x000000000

class SHFILEINFO(ctypes.Structure):
    _fields_ = [
        ("hIcon", wintypes.HICON),
        ("iIcon", wintypes.INT),
        ("dwAttributes", wintypes.DWORD),
        ("szDisplayName", wintypes.WCHAR * 260),
        ("szTypeName", wintypes.WCHAR * 80),
    ]


def extract_icon_from_exe(exe_path: str, size: int) -> Image.Image | None:
    if not exe_path or not os.path.exists(exe_path):
        return None

    shell32 = ctypes.windll.shell32
    user32 = ctypes.windll.user32
    gdi32 = ctypes.windll.gdi32

    shfi = SHFILEINFO()
    res = shell32.SHGetFileInfoW(
        exe_path, 0, ctypes.byref(shfi), ctypes.sizeof(shfi),
        SHGFI_ICON | SHGFI_LARGEICON
    )
    if res == 0 or not shfi.hIcon:
        return None

    hicon = shfi.hIcon

    class ICONINFO(ctypes.Structure):
        _fields_ = [
            ("fIcon", wintypes.BOOL),
            ("xHotspot", wintypes.DWORD),
            ("yHotspot", wintypes.DWORD),
            ("hbmMask", wintypes.HBITMAP),
            ("hbmColor", wintypes.HBITMAP),
        ]

    iconinfo = ICONINFO()
    if not user32.GetIconInfo(hicon, ctypes.byref(iconinfo)):
        user32.DestroyIcon(hicon)
        return None

    class BITMAP(ctypes.Structure):
        _fields_ = [
            ("bmType", wintypes.LONG),
            ("bmWidth", wintypes.LONG),
            ("bmHeight", wintypes.LONG),
            ("bmWidthBytes", wintypes.LONG),
            ("bmPlanes", wintypes.WORD),
            ("bmBitsPixel", wintypes.WORD),
            ("bmBits", ctypes.c_void_p),
        ]

    bmp = BITMAP()
    gdi32.GetObjectW(iconinfo.hbmColor, ctypes.sizeof(bmp), ctypes.byref(bmp))

    width = bmp.bmWidth
    height = bmp.bmHeight

    hdc = user32.GetDC(0)
    memdc = gdi32.CreateCompatibleDC(hdc)
    hbitmap = gdi32.CreateCompatibleBitmap(hdc, width, height)
    gdi32.SelectObject(memdc, hbitmap)

    user32.DrawIconEx(memdc, 0, 0, hicon, width, height, 0, 0, 0x0003)

    class BITMAPINFOHEADER(ctypes.Structure):
        _fields_ = [
            ("biSize", wintypes.DWORD),
            ("biWidth", wintypes.LONG),
            ("biHeight", wintypes.LONG),
            ("biPlanes", wintypes.WORD),
            ("biBitCount", wintypes.WORD),
            ("biCompression", wintypes.DWORD),
            ("biSizeImage", wintypes.DWORD),
            ("biXPelsPerMeter", wintypes.LONG),
            ("biYPelsPerMeter", wintypes.LONG),
            ("biClrUsed", wintypes.DWORD),
            ("biClrImportant", wintypes.DWORD),
        ]

    class BITMAPINFO(ctypes.Structure):
        _fields_ = [
            ("bmiHeader", BITMAPINFOHEADER),
            ("bmiColors", wintypes.DWORD * 3),
        ]

    bmi = BITMAPINFO()
    bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bmi.bmiHeader.biWidth = width
    bmi.bmiHeader.biHeight = -height
    bmi.bmiHeader.biPlanes = 1
    bmi.bmiHeader.biBitCount = 32
    bmi.bmiHeader.biCompression = 0

    buf = (ctypes.c_byte * (width * height * 4))()
    gdi32.GetDIBits(memdc, hbitmap, 0, height, ctypes.byref(buf), ctypes.byref(bmi), 0)

    img = Image.frombuffer("RGBA", (width, height), bytes(buf), "raw", "BGRA", 0, 1)
    img = img.resize((size, size), Image.LANCZOS)

    user32.DestroyIcon(hicon)
    gdi32.DeleteObject(hbitmap)
    gdi32.DeleteDC(memdc)
    user32.ReleaseDC(0, hdc)

    return img


def pil_to_argb8888(img: Image.Image) -> bytes:
    arr = np.array(img.convert("RGBA"), dtype=np.uint8)
    a = arr[:, :, 3:4]
    rgb = arr[:, :, :3]
    argb = np.concatenate([a, rgb], axis=2)
    return argb.tobytes()


def get_app_exe_from_app_id(app_id: str) -> str | None:
    if ".exe" in app_id.lower():
        exe = app_id.split("!")[0]
        if os.path.exists(exe):
            return exe
    return None


def get_app_logo_argb(app_id: str, size: int) -> bytes | None:
    exe = get_app_exe_from_app_id(app_id)
    if not exe:
        return None
    img = extract_icon_from_exe(exe, size)
    if img is None:
        return None
    return pil_to_argb8888(img)


# ─────────────────────────────────────────────────────────────────────────────
#  NOW PLAYING
# ─────────────────────────────────────────────────────────────────────────────

async def get_now_playing():
    try:
        manager = await wmc.GlobalSystemMediaTransportControlsSessionManager.request_async()
        session = manager.get_current_session()
        if session is None:
            return None

        props = await session.try_get_media_properties_async()
        app_id = session.source_app_user_model_id or ""

        if "!" in app_id:
            app_name = app_id.split("!")[-1]
        else:
            app_name = app_id.split("\\")[-1].replace(".exe", "")

        playback = session.get_playback_info()

        return {
            "title": props.title or "",
            "artist": props.artist or "",
            "album": props.album_title or "",
            "app": app_name,
            "app_id": app_id,
            "status": int(playback.playback_status) if playback else 0,
            "media_type": int(playback.playback_type) if playback else 0,
            "session": session,
        }
    except Exception:
        return None


# ─────────────────────────────────────────────────────────────────────────────
#  SERIAL SENDERS
# ─────────────────────────────────────────────────────────────────────────────

def send_clock(ser):
    t, d = get_time_date()
    meta = {"type": "clock", "time": t, "date": d}
    ser.write((json.dumps(meta) + "\n").encode())
    ser.flush()
    print(f"→ clock {t} {d}")


def get_time_date():
    now = datetime.now()
    return now.strftime("%H:%M"), now.strftime("%d.%m.%Y")


def send_track(ser, info, jpeg, color565, logo_argb, pos_ms, dur_ms):
    t, d = get_time_date()

    art_len = len(jpeg) if jpeg else 0
    logo_len = len(logo_argb) if logo_argb else 0

    meta = {
        "type": "track",
        "title": info["title"],
        "artist": info["artist"],
        "album": info["album"],
        "app": info["app"],
        "status": info["status"],
        "media_type": info["media_type"],
        "color": color565,
        "time": t,
        "date": d,

        "position": pos_ms,
        "duration": dur_ms,

        "has_art": art_len > 0,
        "has_logo": logo_len > 0,
        "art_len": art_len,
        "logo_len": logo_len,
        "logo_format": "ARGB8888",
        "logo_w": APP_LOGO_SIZE,
        "logo_h": APP_LOGO_SIZE,
    }

    ser.write((json.dumps(meta, ensure_ascii=False) + "\n").encode())

    ser.write(struct.pack("<I", art_len))
    if art_len:
        ser.write(jpeg)

    ser.write(struct.pack("<I", logo_len))
    if logo_len:
        ser.write(logo_argb)

    ser.flush()

    print(
        f"→ [{info['app']}] {info['artist']} - {info['title']} "
        f"({ms_to_mmss(pos_ms)} / {ms_to_mmss(dur_ms)})"
    )


def send_state(ser, info):
    t, d = get_time_date()
    meta = {"type": "state", "status": info["status"], "time": t, "date": d}
    ser.write((json.dumps(meta) + "\n").encode())
    ser.flush()
    print(f"→ state {info['status']} {t} {d}")


def send_empty(ser):
    t, d = get_time_date()
    meta = {
        "type": "track",
        "title": "",
        "artist": "",
        "album": "",
        "app": "",
        "status": 0,
        "media_type": 0,
        "time": t,
        "date": d,
        "position": 0,
        "duration": 0,
        "has_art": False,
        "has_logo": False,
        "art_len": 0,
        "logo_len": 0,
    }
    ser.write((json.dumps(meta) + "\n").encode())
    ser.flush()
    print("→ nothing playing")


def read_control_from_serial(ser):
    try:
        if ser.in_waiting:
            line = ser.readline().decode(errors="ignore").strip()
            return line or None
    except:
        return None


async def send_control(session, action):
    try:
        if action == "playpause":
            await mc.play_pause()
        elif action == "next":
            await mc.next()
        elif action == "prev":
            await mc.previous()
        elif action == "ff":
            await mc.fast_forward()
        elif action == "rew":
            await mc.rewind()
        elif action.startswith("seek:"):
            await mc.seek(int(action.split(":")[1]))

        print(f"→ control {action}")

    except Exception as e:
        print("Control error:", e)

def send_time_update(ser, pos_ms, dur_ms):
    meta = {
        "type": "time",
        "position": pos_ms,
        "duration": dur_ms
    }
    ser.write((json.dumps(meta) + "\n").encode())
    ser.flush()



# ─────────────────────────────────────────────────────────────────────────────
#  MAIN LOOP
# ─────────────────────────────────────────────────────────────────────────────

async def main():
    global last_track, last_minute, last_state

    port = SERIAL_PORT
    available = [p.device for p in serial.tools.list_ports.comports()]
    if port not in available:
        print("Available ports:", available)
        port = input("Enter port: ").strip()

    ser = serial.Serial(port, BAUD_RATE, timeout=1, write_timeout=2)
    print(f"Connected to {port}")

    time.sleep(2)
    send_clock(ser)
    print("Polling Windows media...\n")
    last_ping = time.time()
    
    while True:
        try:
            current_minute = datetime.now().strftime("%H:%M")
            if current_minute != last_minute:
                last_minute = current_minute
                send_clock(ser)

            info = await get_now_playing()

            cmd = read_control_from_serial(ser)
            if cmd:
                await send_control(info["session"] if info else None, cmd)

            if info is None:
                if last_track is not None:
                    send_empty(ser)
                    last_track = None
                    last_state = None
            else:
                is_playing = (info["status"] == 4)
                pos_ms, dur_ms = await get_position_and_duration(info["session"], is_playing)
                print(f"Playback time: {ms_to_mmss(pos_ms)} / {ms_to_mmss(dur_ms)}")

                track_key = info["title"] + info["artist"] + info["app"]
                metadata_empty_before = (last_track is None or last_track == "")
                metadata_empty_now = ( info["title"] == "" and info["artist"] == "")

                if metadata_empty_before and not metadata_empty_now:
                    last_track = None

                if track_key != last_track:
                    last_track = track_key
                    last_state = None

                    raw_thumb = await get_thumbnail_bytes(info["session"])
                    jpeg, color565 = resize_to_jpeg(raw_thumb, ART_SIZE) if raw_thumb else (None, 0)

                    logo_argb = get_app_logo_argb(info["app_id"], APP_LOGO_SIZE)

                    send_track(ser, info, jpeg, color565, logo_argb, pos_ms, dur_ms)

                else:
                     send_time_update(ser, pos_ms, dur_ms)

                     if info["status"] != last_state:
                         last_state = info["status"]
                         send_state(ser, info)
        
        except Exception as e:
            print(f"ERROR in main loop: {e}")
        
        now = time.time()

        # Always send a heartbeat every 2 seconds (guaranteed)
        if now - last_ping >= 2:
          try:
              t, d = get_time_date()
              meta = {"type": "state", "status": 0, "time": t, "date": d}
              ser.write((json.dumps(meta) + "\n").encode())
              ser.flush()
          except Exception as e:
              print(f"Heartbeat error: {e}")
          last_ping = now

        await asyncio.sleep(POLL_INTERVAL)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Stopped.")




