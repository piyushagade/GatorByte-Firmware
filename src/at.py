import serial
import time

# Define the COM port and baud rate
port = "COMX"  # Change this to your COM port
baud_rate = 9600  # Adjust as needed

# Create a serial connection
ser = serial.Serial(port, baud_rate, timeout=1)

def send_command(command):
    ser.write(command.encode() + b'\r\n')

def read_response():
    response = ser.readline().decode().strip()
    return response

# Send an AT command to the module and read the response
send_command("AT")  # For testing connectivity
response = read_response()
print("Response to 'AT' command:", response)

# You can send other AT commands and read their responses as needed
# Example: Send an AT command to change the module name
send_command("AT+NAME=myBluetoothDevice")
response = read_response()
print("Response to 'AT+NAME=myBluetoothDevice':", response)

# Send and receive data
data_to_send = "Hello, AT-09!"
send_command(data_to_send)
response = read_response()
print("Received data:", response)

# Close the serial connection when done
ser.close()
