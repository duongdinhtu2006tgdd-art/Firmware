"""
Dashboard Node 2 - he tuoi thong minh

Kien truc:
  ESP32 --USB/Serial--> app.py --HTTP/JSON--> trinh duyet

  Luong DU LIEU (ESP32 -> web):
    ESP32 in 1 dong JSON moi giay tren Serial
    -> luong serial_listen_worker doc va luu vao telemetry_data
    -> trinh duyet goi GET /api/telemetry moi 2 giay de lay ra

  Luong LENH (web -> ESP32):
    nguoi dung bam nut -> POST /api/control {"action": "PUMP_ON"}
    -> app.py ghi "PUMP_ON\n" ra Serial
    -> ESP32 doc trong telemetry_poll_command() va thuc thi
    -> ESP32 tra lai {"ack": ...} -> luu vao last_ack de hien len web

VI SAO CAN LUONG RIENG (thread) DE DOC SERIAL:
  Flask xu ly request tuan tu. Neu doc Serial ngay trong handler thi khi
  khong co du lieu, ca web server bi treo cho. Luong rieng doc lien tuc vao
  bo nho dem, handler chi tra ve ban chup moi nhat -> khong bao gio cho.
"""

import json
import threading
import time
from collections import deque

import serial
import serial.tools.list_ports
from flask import Flask, jsonify, render_template, request

app = Flask(__name__, template_folder="dashboard/templates")

# De None de tu do tim cong ESP32; hoac dat cung 'COM3'
SERIAL_PORT = None
BAUD_RATE = 115200

# Ban chup so lieu moi nhat tu ESP32.
# Khoi tao day du moi truong de trinh duyet khong gap 'undefined' truoc
# khi ban tin dau tien ve.
telemetry_data = {
    "temp": 0.0, "hum": 0.0,
    "soil": 0.0, "soil2": 0.0,
    "soil_raw": 0, "soil2_raw": 0,
    "flow": 0.0, "liters": 0.0,
    "dht_ok": 0, "soil_ok": 0, "soil2_ok": 0,
    "pump": "OFF", "pump_liters": 0.0, "pump_left_s": 0, "pump_stop": "NONE",
    "lift": "STOP", "lift_pos": "UNKNOWN", "lift_stop": "NONE",
    "lift_amp_v": 0.0,
    "uptime": 0, "dht_err": 0, "dry_run": 0,
    # Truong do app.py tu them, khong den tu ESP32
    "connected": False,
    "last_update": 0.0,
}

last_ack = {"ack": "", "ok": 1, "reason": "", "time": 0.0}

# Log dong nguoi doc tu ESP32, gioi han 200 dong de khong phinh bo nho
device_log = deque(maxlen=200)

ser = None
data_lock = threading.Lock()


def find_esp32_port():
    """Tim cong COM cua ESP32 qua VID/PID cua chip USB-UART."""
    # CP210x: VID 0x10C4 | CH340: 0x1A86 | FTDI: 0x0403 | ESP32-S USB: 0x303A
    known_vids = {0x10C4, 0x1A86, 0x0403, 0x303A}

    for port in serial.tools.list_ports.comports():
        if port.vid in known_vids:
            return port.device
    return None


def open_serial():
    global ser

    port = SERIAL_PORT or find_esp32_port()
    if not port:
        print("[CANH BAO] Khong tim thay cong ESP32. Cac cong dang co:")
        for p in serial.tools.list_ports.comports():
            print(f"    {p.device}  {p.description}")
        return False

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)  # ESP32 tu reset khi mo cong, cho no boot xong
        ser.reset_input_buffer()
        print(f"[OK] Da mo {port} @ {BAUD_RATE}")
        return True
    except Exception as e:
        print(f"[LOI] Khong mo duoc {port}: {e}")
        ser = None
        return False


def serial_listen_worker():
    """Doc Serial lien tuc. Tu ket noi lai neu rut day."""
    global ser

    while True:
        if ser is None or not ser.is_open:
            with data_lock:
                telemetry_data["connected"] = False
            if not open_serial():
                time.sleep(3)  # cho roi thu lai, khong quay vong lien tuc
            continue

        try:
            raw = ser.readline()
            if not raw:
                continue

            line = raw.decode("utf-8", errors="ignore").strip()
            if not line:
                continue

            # ESP32 gui 2 loai dong tren cung 1 day:
            #   bat dau '{' -> JSON cho may doc
            #   con lai     -> log cho nguoi doc
            if line.startswith("{") and line.endswith("}"):
                handle_json_line(line)
            else:
                print(f"[ESP32] {line}")
                with data_lock:
                    device_log.append({"t": time.time(), "msg": line})

        except (serial.SerialException, OSError) as e:
            # Rut day USB roi vao day. Dong cong de vong lap tu mo lai.
            print(f"[LOI] Mat ket noi Serial: {e}")
            try:
                ser.close()
            except Exception:
                pass
            ser = None


def handle_json_line(line):
    try:
        parsed = json.loads(line)
    except json.JSONDecodeError:
        # Dong bi cat giua (mo cong dung luc ESP32 dang gui) -> bo qua
        return

    with data_lock:
        # Ban tin xac nhan lenh di duong rieng, khong tron vao telemetry
        if "ack" in parsed:
            last_ack.update(parsed)
            last_ack["time"] = time.time()
            print(f"[ACK] {parsed}")
            return

        # Chi cap nhat khoa da biet -> ESP32 them truong moi khong pha vo web
        for key in telemetry_data:
            if key in parsed:
                telemetry_data[key] = parsed[key]

        telemetry_data["connected"] = True
        telemetry_data["last_update"] = time.time()


def send_command(text):
    """Gui 1 lenh xuong ESP32. Tra ve (thanh_cong, thong_bao)."""
    if ser is None or not ser.is_open:
        return False, "Chua ket noi voi ESP32"

    try:
        ser.write((text + "\n").encode("ascii"))
        ser.flush()
        print(f"[GUI] {text}")
        return True, ""
    except Exception as e:
        return False, str(e)


threading.Thread(target=serial_listen_worker, daemon=True).start()


@app.route("/")
def home():
    return render_template("index.html")


@app.route("/api/telemetry", methods=["GET"])
def get_telemetry():
    with data_lock:
        data = dict(telemetry_data)

        # Coi la mat ket noi neu qua 5 giay khong co ban tin moi.
        # Cong Serial van "mo" khi ESP32 treo, nen phai kiem tra thoi gian.
        if data["last_update"] and time.time() - data["last_update"] > 5:
            data["connected"] = False

        data["ack"] = dict(last_ack)

    return jsonify(data)


@app.route("/api/log", methods=["GET"])
def get_log():
    with data_lock:
        return jsonify(list(device_log))


# Danh sach trang: chi lenh nam trong day duoc chuyen xuong ESP32.
# Khong bao gio ghep chuoi tu input nguoi dung roi gui thang - de tranh
# nguoi dung gui lenh tuy y xuong thiet bi.
ALLOWED_COMMANDS = {
    "PUMP_ON", "PUMP_OFF",
    "LIFT_UP", "LIFT_DOWN", "LIFT_STOP",
    "CAL_DRY", "CAL_WET",
    "STATUS",
}


@app.route("/api/control", methods=["POST"])
def post_control():
    payload = request.get_json(silent=True) or {}
    action = str(payload.get("action", "")).upper().strip()

    if action not in ALLOWED_COMMANDS:
        return jsonify({"status": "error", "message": f"Lenh khong hop le: {action}"}), 400

    command = action

    # PUMP_ON co the kem so lit muc tieu
    if action == "PUMP_ON":
        liters = payload.get("liters")
        if liters is not None:
            try:
                value = float(liters)
                if not (0 < value <= 100):
                    return jsonify({"status": "error", "message": "So lit phai trong 0-100"}), 400
                command = f"PUMP_ON:{value:.2f}"
            except (TypeError, ValueError):
                return jsonify({"status": "error", "message": "So lit khong hop le"}), 400

    # CAL_DRY / CAL_WET kem so hieu cam bien
    elif action in ("CAL_DRY", "CAL_WET"):
        try:
            index = int(payload.get("index", 1))
            if index not in (1, 2):
                raise ValueError
            command = f"{action}:{index}"
        except (TypeError, ValueError):
            return jsonify({"status": "error", "message": "So cam bien phai la 1 hoac 2"}), 400

    ok, message = send_command(command)
    if not ok:
        return jsonify({"status": "error", "message": message}), 503

    return jsonify({"status": "success", "sent": command})


if __name__ == "__main__":
    print("Dashboard: http://127.0.0.1:5000")
    # debug=False: che do debug cua Flask khoi dong lai tien trinh 2 lan,
    # se mo cong Serial 2 lan va gay xung dot.
    app.run(host="0.0.0.0", port=5000, debug=False)
