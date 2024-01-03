#
#	pip unistall serial
#	pip install pyserial
#
import serial
from time import sleep
from datetime import datetime
import sys
import os
from rich.console import Console
from rich.table import Table
from rich.live import Live
from rich.layout import Layout
layout = Layout()
console = Console()
clear = lambda: os.system('cls')

ser = None
command_mode = False
CONNECTION_STATUS = False
CONNECTED_PORT = 0

def save (str):
	with open("./sm.log", "a") as logfile:
		logfile.write(str)

def strbetween(s, start, end):
    return (s.split(start))[1].split(end)[0]

def wait(time):
	try:
		sleep(time)
	except KeyboardInterrupt:
		print("\nStopping serial monitor.")
		sys.exit(0)

def connect(first):
	global ser
	ser = None
	wait(3)

	if first:
		# clear()
		CONNECTION_STATUS = False
		CONNECTED_PORT = 0

	baud = 9600
	baseports = ['/dev/ttyUSB', '/dev/ttyACM', 'COM', '/dev/tty.usbmodem1234']

	while not ser:
		if "-p" in sys.argv:
			ports = sys.argv[sys.argv.index("-p") + 1].split(",")

			for port in ports:
				try:
					ser = serial.Serial(port, baud, timeout=1)
					# clear()
					# print("\nWaiting for device -> Connected on " + port + "\n")
					
					CONNECTION_STATUS = True
					CONNECTED_PORT = port
					
					break
				except:
					ser = None
		else:
			for baseport in baseports:
				if ser:
					break
				for i in range(0, 64):
					try:
						port = baseport + str(i)
						ser = serial.Serial(port, baud, timeout=1)
						# clear()
						# print("\nWaiting for device -> Connected on " + port + "\n")
						
						CONNECTION_STATUS = True
						CONNECTED_PORT = port
						
						break
					except:
						ser = None

		# Not devices connected yet
		if not ser:
			first = False
			wait(2)
			if not first:
				try:
					print(" .", end = '')
					pass
				except:
					sys.exit(0)

def link(url):
    return f'[blue][u]{url}[/u][/blue]'

def showui ():
	layout.split_column(
		Layout(name="upper"),
		Layout(name="lower")
	)

	layout["lower"].split_row(
		Layout(name="left"),
		Layout(name="right"),
	)
	layout["left"].update(
		"The mystery of life isn't a problem to solve, but a reality to experience."
	)
	print(layout)

	# # Connection status
	# table = Table(show_header=False)
	# table.add_column('Status', justify='left')
	# with Live(table, refresh_per_second=4):
	# 		time.sleep(0.4)
	# 		table.add_row('[white] Connection status -> ' + '[orange1]Not connected')
	# 		console.int(table)

	# # Log screen
	# table = Table(show_header=False)
	# table.add_column('Status', justify='left')
	# with Live(table, refresh_per_second=4):
	# 	table = Table(show_header=False)
	# 	table.add_column('Log', justify='right')
	# 	table.add_row('[green]bugfix')
	# 	console.print(table)

def inputHandler(str):
	global command_mode

	# If the incoming string a meta command
	if str.startswith("###"):
		str = str.strip("###")

		# If the device sends a fresh file, delete local old file first
		if "SOF#" in str and os.path.exists("./extract/data.txt"):
			str = str.strip("SOF#")
			os.remove("./data.txt")
			print("Downloading data from SD card", end = '')

		# At this point str would be a set of '<= (MAX_LINES_READ_FROM_SD)' rows of data

		# Append new data to the dump
		with open("./data.txt", 'a') as outf:
			if("EOF#" in str):
				str = str.strip("EOF#")
				print(" -> Done\n")
			else:
				print(" .", end = '')
			

			# Process incoming string	
			for i, cell in enumerate(str.split(",")):
				cell = cell + ("," if i < len(str.split(",")) - 1 else "")
				for char in cell.split("!"):
					# char could be an r, t, d, or a comma, LF, etc but in their ascii code (exxcept ',')
					if char == "":
						continue
					try:
						outf.write(chr(int(char.strip(","))) + ("," if "," in char else ""))
					except ValueError:
						pass

	# If the incoming string is to be logged/printed
	else:
		# Print to console
		print(("" if command_mode else "") + str, end = '')

def command_mode_listerner():
	global command_mode
	while True:
		time.sleep(1)
		cmd = input()
		if ser != None and cmd == 'exit':
			ser.write(bytes("##exitcm", 'utf8'))
			if command_mode:
				print('Command mode terminated.\n')
			command_mode = False
		elif ser != None and cmd == 'cm':
			ser.write(bytes("##cm", 'utf8'))
			if not command_mode:
				print('\nCommand mode activated.')
			command_mode = True
		elif ser != None and cmd == 'dd':
			command_mode = False
			ser.close()
		elif ser != None and cmd == 'extract':
			if not command_mode:
				print("\nEnter command mode first by typing 'cm' and press Enter.")
			else:
				ser.write(bytes("##extract", 'utf8'))
		elif ser == None:
			print("\nSerial port not ready.\n")
		else:
			if not command_mode and False:
				print("\nEnter command mode first by typing 'cm' and press Enter.")
			else:
				ser.write(bytes(cmd, 'utf8'))
		

import threading
import time

threading1 = threading.Thread(target=command_mode_listerner)
threading1.daemon = True
threading1.start()

if __name__ == "__main__":
	
	# Show live UI
	showui()

	# Attempt connection
	connect(True)
	error = False

	# Monitor connection and process strings
	while True:
		if ser is not None:
			try:
				serial_str = str(ser.readline().decode())
				# serial_str = str(ser.read().decode())
				error = False
				if(serial_str == ""):
					continue

				# Send incoming string for processing
				inputHandler(serial_str)
			except serial.SerialException as e:
				if(not error):
					print("\nDevice has been disconnected.")
					command_mode = False
					connect(True)
				error = True
			except AttributeError as e:
				if(not error):
					print("\nDevice has been disconnected. Reconnecting in 15 seconds")
					command_mode = False
					wait(15)
					connect(True)
				error = True
			except TypeError as e:
				if(not error):
					print("\nDevice has been disconnected. Reconnecting in 15 seconds")
					command_mode = False
					wait(15)
					connect(True)
				error = True
			except KeyboardInterrupt:
				print("\nStopping serial monitor.")
				sys.exit(0)
			except UnicodeDecodeError:
				print(" x", end = '')