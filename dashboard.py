
import tkinter as tk
import urllib.request
import json

API = "http://127.0.0.1:5001"

BG = "#101820"
PANEL = "#1c2935"
TEXT = "#ffffff"
GREEN = "#35c98b"
RED = "#ed6262"


class SmartCareDashboard:

    def __init__(self, root):
        self.root = root
        self.root.title("SmartCare - Assisted Living System")
        self.root.geometry("1050x750")
        self.root.configure(bg=BG)

        self.values = {}

        title = tk.Label(
            root,
            text="SMARTCARE",
            font=("Arial", 25, "bold"),
            fg=GREEN,
            bg=BG
        )
        title.pack(pady=12)

        self.connection = tk.Label(
            root,
            text="Connecting...",
            font=("Arial", 11),
            fg="orange",
            bg=BG
        )
        self.connection.pack()

        self.create_sensor_panel()
        self.create_status_panel()
        self.create_controls()
        self.create_event_panel()

        self.update_dashboard()

    def create_sensor_panel(self):
        panel = tk.LabelFrame(
            self.root,
            text="LIVE SENSOR MONITORING",
            bg=PANEL,
            fg=GREEN,
            font=("Arial", 12, "bold")
        )
        panel.pack(fill="x", padx=15, pady=10)

        sensors = [
            ("Temperature", "temperature", "°C"),
            ("Humidity", "humidity", "%"),
            ("Air Quality", "gas", ""),
            ("Water Sensor", "water", ""),
            ("Soil Moisture", "soil", ""),
            ("Distance", "distance", "cm"),
            ("Light Level", "light", "")
        ]

        for index, (name, key, unit) in enumerate(sensors):
            card = tk.Frame(panel, bg=BG, padx=12, pady=10)
            card.grid(row=index // 4, column=index % 4,
                      padx=8, pady=8, sticky="nsew")

            tk.Label(
                card, text=name, bg=BG, fg=TEXT,
                font=("Arial", 10)
            ).pack()

            value = tk.Label(
                card, text="-- " + unit,
                bg=BG, fg=GREEN,
                font=("Arial", 16, "bold")
            )
            value.pack()

            self.values[key] = (value, unit)

    def create_status_panel(self):
        panel = tk.LabelFrame(
            self.root,
            text="DEVICE STATUS",
            bg=PANEL,
            fg=GREEN,
            font=("Arial", 12, "bold")
        )
        panel.pack(fill="x", padx=15, pady=8)

        self.status = {}

        for index, name in enumerate(
            ["Gate", "Lights", "Fan", "Alarm"]
        ):
            label = tk.Label(
                panel,
                text=name + ": --",
                bg=BG,
                fg=TEXT,
                font=("Arial", 12, "bold"),
                width=20,
                pady=12
            )
            label.grid(row=0, column=index, padx=8, pady=8)
            self.status[name] = label

    def create_controls(self):
        panel = tk.LabelFrame(
            self.root,
            text="MANUAL CONTROLS",
            bg=PANEL,
            fg=GREEN,
            font=("Arial", 12, "bold")
        )
        panel.pack(fill="x", padx=15, pady=8)

        controls = [
            ("UNLOCK GATE", "UNLOCK"),
            ("LOCK GATE", "LOCK"),
            ("RESET ALARM", "ALARM_RESET"),
            ("LIGHTS ON", "LIGHT_ON"),
            ("LIGHTS OFF", "LIGHT_OFF"),
            ("LIGHTS AUTO", "LIGHT_AUTO"),
            ("FAN ON", "FAN_ON"),
            ("FAN OFF", "FAN_OFF"),
            ("FAN AUTO", "FAN_AUTO"),
            ("PROTECTION ON", "FALSE_PROTECTION_ON"),
            ("PROTECTION OFF", "FALSE_PROTECTION_OFF")
        ]

        for index, (label, command) in enumerate(controls):
            button = tk.Button(
                panel,
                text=label,
                command=lambda c=command: self.send_command(c),
                bg="#29475b",
                fg=TEXT,
                activebackground=GREEN,
                font=("Arial", 9, "bold"),
                width=17,
                pady=7
            )
            button.grid(
                row=index // 4,
                column=index % 4,
                padx=5,
                pady=6
            )

    def create_event_panel(self):
        panel = tk.LabelFrame(
            self.root,
            text="SYSTEM EVENT LOG",
            bg=PANEL,
            fg=GREEN,
            font=("Arial", 12, "bold")
        )
        panel.pack(fill="both", expand=True, padx=15, pady=8)

        self.log = tk.Listbox(
            panel,
            bg=BG,
            fg=TEXT,
            font=("Arial", 10),
            height=6
        )
        self.log.pack(fill="both", expand=True, padx=8, pady=8)

    def send_command(self, command):
        try:
            body = json.dumps({"command": command}).encode()

            request = urllib.request.Request(
                API + "/command",
                data=body,
                headers={"Content-Type": "application/json"},
                method="POST"
            )

            with urllib.request.urlopen(request, timeout=2) as response:
                result = json.loads(response.read().decode())

            if not result.get("success"):
                self.connection.config(
                    text="Arduino not connected",
                    fg=RED
                )

        except Exception as error:
            self.connection.config(
                text="Command failed: " + str(error),
                fg=RED
            )

    def update_dashboard(self):
        try:
            with urllib.request.urlopen(
                API + "/data", timeout=2
            ) as response:
                data = json.loads(response.read().decode())

            self.connection.config(
                text="Arduino Connected",
                fg=GREEN
            )

            for key, (label, unit) in self.values.items():
                label.config(
                    text=str(data.get(key, "--")) + " " + unit
                )

            gate = "LOCKED" if data.get("gateLocked") else "UNLOCKED"
            lights = "ON" if data.get("lightsOn") else "OFF"
            fan = "ON" if data.get("fanOn") else "OFF"
            alarm = "ACTIVE" if data.get("alarmActive") else "INACTIVE"

            self.status["Gate"].config(text="Gate: " + gate)
            self.status["Lights"].config(text="Lights: " + lights)
            self.status["Fan"].config(text="Fan: " + fan)
            self.status["Alarm"].config(text="Alarm: " + alarm)

            events = data.get("events", [])
            self.log.delete(0, tk.END)

            for event in events[-20:]:
                self.log.insert(tk.END, event)

        except Exception:
            self.connection.config(
                text="Waiting for Raspberry Pi server...",
                fg="orange"
            )

        self.root.after(1000, self.update_dashboard)


if __name__ == "__main__":
    root = tk.Tk()
    app = SmartCareDashboard(root)
    root.mainloop()
