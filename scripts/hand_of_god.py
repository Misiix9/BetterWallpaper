import sys
import os
import time
import subprocess
import psutil
import signal
import shutil

# Lazy/Safe Imports for PyAutoGUI (X11 dependency)
try:
    import pyautogui
    # Disable FailSafe on Wayland/Headless to prevent early exit if mouse logic is wonky
    pyautogui.FAILSAFE = False 
except Exception as e:
    pyautogui = None
    print(f"[WARN] PyAutoGUI unavailable: {e}")

# Attempt to import pygetwindow, but handle Linux limitations if necessary
try:
    import pygetwindow as gw
except ImportError:
    gw = None
except Exception:
    gw = None
import argparse
import random
import signal

# Configuration
APP_NAME = "BetterWallpaper"
# Adjust path based on build location relative to script root
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP_PATH = os.path.join(project_root, "build", "src", "gui", "betterwallpaper")

def log(message):
    print(f"[GOD HAND] {message}")

LOG_DIR = os.path.join(project_root, "logs")
if not os.path.exists(LOG_DIR):
    os.makedirs(LOG_DIR)

def take_screenshot(name):
    filename = os.path.join(LOG_DIR, name)
    log(f"Snapshotting: {filename}")
    
    # Try Wayland/Hyprland first (grim)
    if shutil.which("grim"):
        try:
             subprocess.run(["grim", filename], check=False)
             return
        except Exception as e:
             log(f"Grim failed: {e}")

    # Fallback to PyAutoGUI
    if pyautogui:
        try:
            pyautogui.screenshot(filename)
            return
        except Exception as e:
            log(f"PyAutoGUI screenshot failed: {e}")
            
    log("Screenshot functionality unavailable.")

def launch_app():
    log(f"Launching app from: {APP_PATH}")
    if not os.path.exists(APP_PATH):
        log(f"ERROR: App binary not found at {APP_PATH}")
        sys.exit(1)
        
    # Check if already running
    for proc in psutil.process_iter(['name']):
        if "betterwallpaper" in proc.info['name']:
            log("App already running. Killing old instance...")
            proc.kill()
            time.sleep(1)

    subprocess.Popen([APP_PATH])
    time.sleep(3) # Wait for startup
    log("App process started.")
    take_screenshot("state_startup.png")

def focus_window():
    log(f"Focusing window: {APP_NAME}")
    
    # Platform specific focus logic
    if sys.platform.startswith('linux'):
        # Hyprland Support
        if os.environ.get("XDG_SESSION_DESKTOP") == "Hyprland":
             try:
                 # Hyprland regex match for window title
                 log("Detected Hyprland. Using hyprctl dispatch...")
                 # Focus by title regex
                 subprocess.run(["hyprctl", "dispatch", "focuswindow", f"title:^{APP_NAME}$"], check=False)
                 return
             except Exception as e:
                 log(f"Hyprland dispatch failed: {e}")

        # Fallback to wmctrl (X11/XWayland sometimes)
        if shutil.which("wmctrl"):
             subprocess.run(["wmctrl", "-a", APP_NAME], check=False)
        else:
             log("wmctrl not found. Window focus might fail if not main window.")
            
    else:
        # Windows/Mac
        if gw:
            try:
                windows = gw.getWindowsWithTitle(APP_NAME)
                if windows:
                    win = windows[0]
                    win.activate()
                else:
                    log("Window not found via pygetwindow.")
            except Exception as e:
                log(f"Focus failed: {e}")

    time.sleep(1)

def tab_navigate(times, enter=False):
    log(f"Navigating: TAB x {times} {'+ ENTER' if enter else ''}")
    
    if pyautogui:
        for _ in range(times):
            try: pyautogui.press('tab')
            except: pass
            time.sleep(0.1)
        if enter:
            try: pyautogui.press('enter')
            except: pass
    else:
        # Wayland Fallback (wtype)
        if shutil.which("wtype"):
             for _ in range(times):
                 subprocess.run(["wtype", "-k", "Tab"])
                 time.sleep(0.1)
             if enter:
                 subprocess.run(["wtype", "-k", "Return"])
        else:
             log("WARNING: Cannot navigate (pyautogui missing, wtype missing).")
    
    time.sleep(0.5)

def click_image(image_filename):
    if not pyautogui:
        log("Skipping image search (pyautogui unavailable)")
        return False

    log(f"Looking for image: {image_filename}")
    image_path = os.path.join(project_root, "tests", "assets", image_filename)
# ... check exist ...
    if not os.path.exists(image_path):
        log(f"Image file not found: {image_path}")
        return False
        
    try:
        location = pyautogui.locateOnScreen(image_path, confidence=0.8)
        if location:
            center = pyautogui.center(location)
            log(f"Found at {center}. Clicking...")
            pyautogui.click(center)
            return True
        else:
            log("Image not found on screen.")
            return False
    except Exception as e:
        log(f"Search failed: {e}")
        return False

def chaos_monkey(duration=30):
    log(f"Starting CHAOS MONKEY for {duration} seconds...")
    end_time = time.time() + duration
    
    if not pyautogui:
        log("Cannot run Chaos Monkey: PyAutoGUI unavailable (Wayland restricted?)")
        log("Running limited 'Blind Chaos' (Checking process alive only)")
        while time.time() < end_time:
             if not check_alive():
                 break
             time.sleep(1)
        return

    width, height = pyautogui.size()
    # Assume app is centered or focused, effectively we just click randomly?
    # Better to restrict to app bounds if possible, but fallback to screen center region
    
    while time.time() < end_time:
        action = random.choice(['click', 'key', 'scroll'])
        
        try:
            if action == 'click':
                # Random click in center 50% of screen to hit app usually
                x = random.randint(int(width*0.25), int(width*0.75))
                y = random.randint(int(height*0.25), int(height*0.75))
                pyautogui.click(x, y)
                
            elif action == 'key':
                key = random.choice(['tab', 'space', 'esc', 'enter', 'up', 'down', 'left', 'right'])
                pyautogui.press(key)
                
            elif action == 'scroll':
                pyautogui.scroll(random.randint(-10, 10))
        except Exception:
            pass # Ignore fails on Wayland/odd configs
            
        time.sleep(0.2)
        
    log("Chaos Monkey finished.")

def check_alive():
    for proc in psutil.process_iter(['name']):
        if "betterwallpaper" in proc.info['name']:
            return True
    return False

def main():
    parser = argparse.ArgumentParser(description="Hand of God - GUI Automation")
    parser.add_argument("--mode", choices=['simple', 'chaos'], default='simple', help="Test mode")
    parser.add_argument("--duration", type=int, default=30, help="Chaos duration")
    args = parser.parse_args()

    # FAILSAFE
    if pyautogui:
        pyautogui.FAILSAFE = True
        log("FAILSAFE ENABLED: Move mouse to top-left corner to abort.")
    else:
        log("FAILSAFE DISABLED: PyAutoGUI unavailable.")

    launch_app()
    focus_window()

    if args.mode == 'simple':
        log("Running Simple Navigation Test...")
        tab_navigate(3, enter=True) # Focus something and enter
        time.sleep(2)
        tab_navigate(1, enter=False)
        
    elif args.mode == 'chaos':
        chaos_monkey(args.duration)

    if check_alive():
        log("RESULT: SUCCESS - App survived.")
        take_screenshot("state_success.png")
    else:
        log("RESULT: CRASH - App process died.")
        take_screenshot("state_crash.png")

if __name__ == "__main__":
    main()
