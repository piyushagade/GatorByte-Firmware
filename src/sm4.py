##
#	-p [Port list]		-	Wait and connect to the ports comma-separated) 
#	-c					-	Clear console on re-connect 
##

import subprocess, sys
def checklib (library_name):
	try:
		__import__(library_name)
		return True
	except ImportError:
		return False

def install(name):
    subprocess.call([sys.executable, '-m', 'pip', 'install', name])

def remove(name):
    subprocess.call([sys.executable, '-m', 'pip', 'uninstall', name])

# Remove 'serial'
if (checklib("serial")):
	try:
		remove("serial")
	except Exception as e:
		print(f'Failed to install library: {str(e)}')

# Install 'pyserial'
if (not checklib("pyserial")):
	try:
		install("pyserial")
	except Exception as e:
		print(f'Failed to install library: {str(e)}')

import serial
from time import sleep
from datetime import datetime
import sys
import os
import re
import threading
import time
import serial.tools.list_ports

clear = lambda: os.system('cls')

ser = None
command_mode = False

def listports ():
	portsdetected = list(serial.tools.list_ports.comports())

	print("\nAvailable NB1500 ports:")

	flag = False
	if (len(portsdetected) > 0):
		for p in portsdetected:
			if ("NB 1500" in p.description):
				print (" " * 3 + str(p))
				flag = True
	
	if (len(portsdetected) == 0 or not flag):
		print ("None detected")

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
		print('\nWaiting for device', end = '')

	baud = 500000
	baseports = ['/dev/ttyUSB', '/dev/ttyACM', 'COM', '/dev/tty.usbmodem1234']

	# while not ser and not keystate = "ctrl+u":
	while not ser:
    
		if "-p" in sys.argv:
			# port = sys.argv[sys.argv.index("-p") + 1]
			ports = sys.argv[sys.argv.index("-p") + 1].split(",")

			for port in ports:
				try:
					ser = serial.Serial(port, baud, timeout=1)
					if "-c" in sys.argv:
						clear()
					print("\nWaiting for device -> \033[1;37;42mConnected\033[0m on " + port + "\n")
					save("\n*****************************************************")
					save("\n" + str(datetime.now().date()) + ", " + str(datetime.now().time()) + "\n")
					
					# while True:
					# 	ser.write("###gb-monitor-ping###", "utf8");
					break
				except:
					ser = None
		else:

			flag = False;

			if (not flag):
				portsdetected = list(serial.tools.list_ports.comports())
				if (len(portsdetected) > 0):
					for p in portsdetected:
						if ("NB 1500" in p.description and "bootloader" not in p.description):
							try:
								ser = serial.Serial(p.name, baud, timeout=1)
								if "-c" in sys.argv:
									clear()
								print("\nWaiting for device -> \033[1;37;42mConnected\033[0m on " + str(p) + "\n")
								save("\n*****************************************************")
								save("\n" + str(datetime.now().date()) + ", " + str(datetime.now().time()) + "\n")
								flag = True
							except serial.SerialException as e:
								print("\n\033[1;37;41mError opening port " + str(p) + "\033[0m")
								print(e)
								print("")
								sys.exit(1)

				
		if not ser:
			first = False
			wait(2)
			if not first:
				try:
					# print(" .", end = '')
					pass
				except:
					sys.exit(0)

def inputHandler(str):
	global command_mode
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
				# print(" .", end = '')
				pass
			

			# Process incoming string	
			for i,cell in enumerate(str.split(",")):
				cell = cell + ("," if i < len(str.split(",")) - 1 else "")
				for char in cell.split("!"):
					# char could be an r, t, d, or a comma, LF, etc but in their ascii code (exxcept ',')
					if char == "":
						continue
					
					try:
						outf.write(chr(int(char.strip(","))) + ("," if "," in char else ""))
					except ValueError:
						pass
	elif str.startswith("##GB:GDC##"):
		str = str.strip("##GB:GDC##").strip("#EOF#")
		category = str.split("::")[0]
		data = str.split("::")[1]
		process(category, data)
		pass
	else:
		try:

			# Log to a file
			if (str.startswith("|*") and str.index("*|") > -1):
				text = str
				left = "|*"
				right = "*|"
				file = text[text.index(left)+len(left):text.index(right)]
				with open("sm_" + file + ".log", "a") as file:
					file.write(str.replace("\n", "").replace(left + str[len(left):text.index(right)] + right, ""))
				
			# Print to console
			else:
				if not str.startswith("##"):
					print(("" if command_mode else "") + str, end = '')
					sys.stdout.flush()
					save(str.replace("\n", ""))
		except :
			sleep(0.1)
			print(("" if command_mode else "") + str, end = '')

def process(category, data):
	#
	#	data::SURVEYID,SURVEYDATE,DATE,TIME,RTD,PH,DO,EC,LAT,LNG,TIMESTAMP,TEMP,RH,GPSATTEMPTS,CELLSIGSTR
	#	modem-firm,11-08-2022,11/22/22,15:41:58,10.43,19.29,21.56,17.08,12.34000,-56.78000,1669131718,27,255,5,99#EOF#
	#
	
	pass

def command_mode_listerner():
	global command_mode
	while True:
		time.sleep(1)
		try:
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
		except:
			pass

threading1 = threading.Thread(target=command_mode_listerner)
threading1.daemon = True
threading1.start()


keystate = ""

if __name__ == "__main__":
	listports()

	connect(True)
	error = False

	while True:
     
		if ser is not None:
			try:
				serial_str = str(ser.readline().decode())
				error = False
				if(serial_str == ""):
					continue

				# Send incoming string for processing
				inputHandler(serial_str)
			except serial.SerialException as e:
				if(not error):
					print("\nDevice has been disconnected.\n")
					command_mode = False
					connect(True)
				error = True
			except AttributeError as e:
				if(not error):
					print("\nDevice has been disconnected. Reconnecting in 15 seconds\n")
					command_mode = False
					wait(15)
					connect(True)
				error = True
			except TypeError as e:
				if(not error):
					print("\nDevice has been disconnected. Reconnecting in 15 seconds\n")
					command_mode = False
					wait(15)
					connect(True)
				error = True
			except KeyboardInterrupt:
				print("\nStopping serial monitor.\n")
				sys.exit(0)
			except UnicodeDecodeError:
				print(" x", end = '')