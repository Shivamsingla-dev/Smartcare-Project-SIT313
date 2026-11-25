
import socket
import threading
import json
from http.server import BaseHTTPRequestHandler, HTTPServer

HOST = "0.0.0.0"
TCP_PORT = 5000
HTTP_PORT = 5001

client = None
client_lock = threading.Lock()
data_lock = threading.Lock()

data = {
    "temperature": 0,
    "humidity": 0,
    "gas": 0,
    "water": 0,
    "soil": 0,
    "distance": -1,
    "light": 0,
    "fanMode": "AUTO",
    "fanOn": 0,
    "lightMode": "AUTO",
    "lightsOn": 0,
    "gateLocked": 1,
    "gateOpen": 0,
    "alarmActive": 0,
    "alarmReason": "NONE",
    "falseAlarmProtection": 1,
    "connected": False,
    "events": []
}


def add_event(message):
    with data_lock:
        data["events"].append(message)
        data["events"] = data["events"][-30:]


def parse_data(line):
    parts = line.strip().split(",")

    if len(parts) < 17 or parts[0] != "DATA":
        return

    try:
        values = {
            "temperature": float(parts[1]),
            "humidity": float(parts[2]),
            "gas": int(parts[3]),
            "water": int(parts[4]),
            "soil": int(parts[5]),
            "distance": float(parts[6]),
            "light": int(parts[7]),
            "fanMode": parts[8],
            "fanOn": int(parts[9]),
            "lightMode": parts[10],
            "lightsOn": int(parts[11]),
            "gateLocked": int(parts[12]),
            "gateOpen": int(parts[13]),
            "alarmActive": int(parts[14]),
            "alarmReason": parts[15],
            "falseAlarmProtection": int(parts[16])
        }

        with data_lock:
            data.update(values)
            data["connected"] = True

    except (ValueError, IndexError):
        print("[ERROR] Invalid sensor data:", line)


def receive_data(conn):
    global client

    print("[NANO] Arduino connected")

    with client_lock:
        client = conn

    buffer = ""

    try:
        while True:
            chunk = conn.recv(1024).decode("utf-8", errors="ignore")

            if not chunk:
                break

            buffer += chunk

            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()

                if line.startswith("DATA,"):
                    parse_data(line)
                elif line:
                    print("[NANO]", line)
                    add_event(line)

    except OSError as error:
        print("[ERROR]", error)

    finally:
        with client_lock:
            if client is conn:
                client = None

        with data_lock:
            data["connected"] = False

        conn.close()
        print("[NANO] Arduino disconnected")


def tcp_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, TCP_PORT))
    server.listen(5)

    print("[TCP] Listening on port", TCP_PORT)

    while True:
        conn, address = server.accept()
        print("[TCP] Connection from", address)

        threading.Thread(
            target=receive_data,
            args=(conn,),
            daemon=True
        ).start()


def send_command(command):
    with client_lock:
        connection = client

    if connection is None:
        return False

    try:
        connection.sendall((command + "\n").encode("utf-8"))
        add_event("Command sent: " + command)
        return True
    except OSError:
        return False


class APIHandler(BaseHTTPRequestHandler):

    def send_json(self, result, status=200):
        body = json.dumps(result).encode("utf-8")

        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == "/data":
            with data_lock:
                result = dict(data)

            self.send_json(result)
        else:
            self.send_json({"error": "Not found"}, 404)

    def do_POST(self):
        if self.path != "/command":
            self.send_json({"error": "Not found"}, 404)
            return

        try:
            length = int(self.headers.get("Content-Length", 0))
            body = json.loads(self.rfile.read(length))
            command = body.get("command", "").strip()

            if not command:
                self.send_json({"error": "Missing command"}, 400)
                return

            success = send_command(command)

            self.send_json({
                "success": success,
                "command": command
            }, 200 if success else 503)

        except (ValueError, json.JSONDecodeError):
            self.send_json({"error": "Invalid request"}, 400)

    def log_message(self, format, *args):
        pass


def main():
    threading.Thread(target=tcp_server, daemon=True).start()

    http_server = HTTPServer((HOST, HTTP_PORT), APIHandler)

    print("[API] Dashboard API running on port", HTTP_PORT)

    http_server.serve_forever()


if __name__ == "__main__":
    main()
