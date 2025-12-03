import serial
import time

def read_arduino_data():
    # Configure the serial connection
    # You may need to change the port name
    # On Windows it's usually 'COM3', 'COM4', etc.
    # On Linux/Mac it's usually '/dev/ttyUSB0', '/dev/ttyACM0', etc.
    port = '/dev/tty.usbmodem101'  # Change this to your Arduino's port
    baud_rate = 9600       # Make sure this matches the baud rate in your Arduino sketch
    
    try:
        # Open the serial connection
        ser = serial.Serial(port, baud_rate, timeout=1)
        print(f"Connected to {port} at {baud_rate} baud")
        
        # Give the serial connection time to establish
        time.sleep(2)
        
        # Read data continuously
        while True:
            if ser.in_waiting > 0:
                # Read a line from the serial port
                line = ser.readline().decode('utf-8').strip()
                print(f"Received: {line}")
                
    except serial.SerialException as e:
        print(f"Error: Could not connect to serial port: {e}")
    except KeyboardInterrupt:
        print("Program terminated by user")
    finally:
        # Close the serial connection if it's open
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Serial connection closed")

if __name__ == "__main__":
    # First install pyserial if you don't have it:
    # pip install pyserial
    read_arduino_data()
