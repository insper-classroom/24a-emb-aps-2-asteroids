import serial
import uinput

from pynput.keyboard import Key, Controller
import serial
import time

keyboard = Controller()

# Setup Serial Connection
ser = serial.Serial('/dev/ttyACM0', 115200)

# Create new mouse device
device = uinput.Device([
    uinput.BTN_LEFT,
    uinput.BTN_RIGHT,
    uinput.REL_X,
    uinput.REL_Y,
    uinput.KEY_LEFT,
    uinput.KEY_RIGHT,
    uinput.KEY_SPACE  # Include spacebar key for keyboard functionality
])

def parse_data(data):
    axis = data[0]  # 0 for X, 1 for Y, 3 for Keyboard
    if axis <= 1:
        value = int.from_bytes(data[1:3], byteorder='little', signed=True)
    else:
        value = data[1]
    print(f"Received data: {data}")
    print(f"axis: {axis}, value: {value}")
    return axis, value

def move_mouse(axis, value):
    if axis == 0:    # X-axis
        device.emit(uinput.REL_X, value)
    elif axis == 1:  # Y-axis
        device.emit(uinput.REL_Y, value)

def keyboard_function(key_code):
    if key_code == 1:  # Space bar key code
        #device.emit_click(uinput.KEY_SPACE)
        keyboard.press(Key.space)
        time.sleep(0.05)
        keyboard.release(Key.space)
    elif key_code == 2:  # Right arrow
        keyboard.press(Key.right)
        time.sleep(0.05)  # Short delay to simulate press duration
        keyboard.release(Key.right)
    elif key_code == 3:  # Left arrow
        keyboard.press(Key.left)
        time.sleep(0.05)  # Short delay to simulate press duration
        keyboard.release(Key.left)

try:
    # Sync package
    while True:
        print('Waiting for sync package...')
        while True:
            sync_byte = ser.read(1)
            if sync_byte == b'\xff':
                break                                                                                   

        # Read next 3 bytes from UART after sync byte
        data = ser.read(3)                                    
        if len(data) == 3:                                                                                                  
            axis, value = parse_data(data)               
            if axis in [0, 1]:  # Mouse movement                        
                move_mouse(axis, value)                                                                                                                                                                                                                                                                                   
            elif axis == 3:  # Keyboard control                                                                                                                                                     
                keyboard_function(value)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          

except KeyboardInterrupt:
    print("Program terminated by user")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()
