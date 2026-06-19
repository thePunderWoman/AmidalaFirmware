#!/usr/bin/env python3
"""Local development server for the Amidala web UI.

Serves files from web/ and mocks the firmware REST API so you can preview
and iterate on the UI without flashing to hardware.

Usage:
    python3 scripts/web_dev.py
Then open http://localhost:8080 in your browser.
"""

import json
import os
from http.server import HTTPServer, SimpleHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

_HERE        = os.path.dirname(os.path.abspath(__file__))
_PROJECT_DIR = os.path.dirname(_HERE)
_WEB_DIR     = os.path.join(_PROJECT_DIR, "web")
_PORT        = 8080

# In-memory config state; POST /api/config mutates this
_general = {
    "volume":      50,
    "startup":     "y",
    "rndon":       "y",
    "mindelay":    60,
    "maxdelay":    120,
    "ackon":       "n",
    "goslow":      "n",
    "mix12":       "n",
    "auto":        "n",
    "serialbaud":  9600,
    "serialdelim": 58,
    "serialeol":   13,
    "myi2c":       0,
}

_wifi = {
    "wifion":       "y",
    "wifissid":     "amidala",
    "wifipassword": "Astromech",
}

_xbee = {
    "xbr": "00000000",
    "xbl": "00000000",
}

_audio = {
    "audiohw":        "hcr",
    "volumeChA":      50,
    "volumeChB":      50,
    "volumewheel":    0,
    "altvolumewheel": 0,
    "startupem":      0,
    "startuplvl":     0,
    "ackem":          0,
    "acklvl":         0,
}

_rc_radio = {
    "rcchn":  6,
    "rcd":    30,
    "rcj":    5,
    "fst":    1000,
    "rvrmin": 0,
    "rvrmax": 1023,
    "rvlmin": 0,
    "rvlmax": 1023,
    "j1adjv": 0,
    "j1adjh": 0,
}

_dome = {
    "domespeed":     80,
    "domespeedhome": 50,
    "domespeedseek": 50,
    "domespeedmin":  10,
    "domedecelzone": 20,
    "domehome":      0,
    "domeflip":      "n",
    "domeimu":       "y",
    "domech6":       "n",
    "domeseekl":     90,
    "domeseekr":     90,
    "domefudge":     5,
    "domercaddr":    128,
    "domercchan":    1,
    "domercqpps":    1000,
    "domefront":     0,
    "domestall":     2000,
}

_info = {
    "firmware": "Amidala RC",
    "version":  "1.3",
    "date":     "Jun 19 2026",
    "drive":    "roboteq-pwm",
    "dome":     "roboclaw",
    "audio":    "hcr",
    "wifi_ssid":"amidala",
    "wifi_ip":  "192.168.4.1",
}


class _Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=_WEB_DIR, **kwargs)

    # ------------------------------------------------------------------ GET --
    def do_GET(self):
        path = urlparse(self.path).path

        _api = {
            "/api/info":             _info,
            "/api/config/general":   _general,
            "/api/config/wifi":      _wifi,
            "/api/config/xbee":      _xbee,
            "/api/config/audio":     _audio,
            "/api/config/rc-radio":  _rc_radio,
            "/api/config/dome":      _dome,
        }
        if path in _api:
            self._json(_api[path])
            return

        # Map extension-less paths to .html (e.g. /config/general → general.html)
        local = os.path.join(_WEB_DIR, path.lstrip("/"))
        if not os.path.exists(local) and not os.path.splitext(path)[1]:
            candidate = local + ".html"
            if os.path.exists(candidate):
                self.path = path + ".html"

        super().do_GET()

    # ----------------------------------------------------------------- POST --
    def do_POST(self):
        path = urlparse(self.path).path
        if path != "/api/config":
            self._text("Not Found", 404)
            return

        length = int(self.headers.get("Content-Length", 0))
        body   = self.rfile.read(length).decode()
        params = {k: v[0] for k, v in parse_qs(body).items()}
        key    = params.get("key", "")
        value  = params.get("value", "")

        print(f"  CONFIG  {key} = {value!r}")
        if key in _general:
            try:
                _general[key] = int(value)
            except ValueError:
                _general[key] = value
        else:
            print(f"           (unknown key — not stored)")

        self._text("OK")

    # ------------------------------------------------------------ helpers ---
    def _json(self, data: dict, status: int = 200) -> None:
        body = json.dumps(data).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def _text(self, text: str, status: int = 200) -> None:
        body = text.encode()
        self.send_response(status)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        # Only log non-200 responses to keep the console clean
        if len(args) >= 2 and args[1] != "200":
            super().log_message(fmt, *args)


if __name__ == "__main__":
    print(f"Amidala web UI  —  dev server")
    print(f"  Serving : {_WEB_DIR}")
    print(f"  Open    : http://localhost:{_PORT}")
    print(f"  Stop    : Ctrl+C\n")
    httpd = HTTPServer(("", _PORT), _Handler)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")
