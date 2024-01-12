#
#	pip unistall serial
#	pip install pyserial
#
import serial
from time import sleep
from datetime import datetime
import sys
import os
import re
import threading
import time

clear = lambda: os.system('cls')

ser = None

def logtofile (filename = "sm.log", str = ""):
	with open("./" + filename, "a") as logfile:
		logfile.write(str)

def strbetween(s, start, end):
    return (s.split(start))[1].split(end)[0]

def wait(time):
	try:
		sleep(time)
	except KeyboardInterrupt:
		print("\nStopping serial monitor.")
		sys.exit(0)

#! Connect to a serial port
def connect(first):
	global ser
	ser = None
	# wait(3)

	# Show a message when waiting for a device to connect
	if first:
		print('\nWaiting for device', end = '')

	# Set baud rate
	baud = 9600
	if "-b" in sys.argv:
		baud = sys.argv[sys.argv.index("-b") + 1].split(",")

	# Set communication ports base string
	baseports = ['/dev/ttyUSB', '/dev/ttyACM', 'COM', '/dev/tty.usbmodem1234']

	# Find and connect to a port
	while not ser:

		# If specific port(s) is requested
		if "-p" in sys.argv:
			ports = sys.argv[sys.argv.index("-p") + 1].split(",")
			for port in ports:
				try:
					ser = serial.Serial(port, baud, timeout=1)
					clear()
					print("\nWaiting for device -> Connected on " + port + "\n")

					logtofile("sm.log","\n*****************************************************")
					logtofile("sm.log","\n" + str(datetime.now().date()) + ", " + str(datetime.now().time()) + "\n")

					logtofile("log-at.txt","\n*****************************************************")
					logtofile("log-at.txt","\n" + str(datetime.now().date()) + ", " + str(datetime.now().time()) + "\n")
					break
				except:
					ser = None

		# Find and open the first open communication port
		else:
			for baseport in baseports:
				if ser:
					break
				for i in range(0, 64):
					try:
						port = baseport + str(i)
						ser = serial.Serial(port, baud, timeout=1)
						clear()
						print("\nWaiting for device -> Connected on " + port + "\n")

						logtofile("sm.log","\n*****************************************************")
						logtofile("sm.log","\n" + str(datetime.now().date()) + ", " + str(datetime.now().time()) + "\n")

						logtofile("log-at.txt","\n*****************************************************")
						logtofile("log-at.txt","\n" + str(datetime.now().date()) + ", " + str(datetime.now().time()) + "\n")
						break
					except:
						ser = None

		# If port not avaialbe, retry every 2 seconds
		if not ser:
			first = False
			wait(2)

			if not first:
				try:
					print(" .", end = '')
					pass
				except:
					sys.exit(0)

def inputhandler(str):

	#! Process incoming commands from a GB device
	if str.find("##GB:GDC##") > -1:
		str = str.replace("##GB:GDC##", "").replace("#EOF#", "")
		category = str.split("::")[0]
		data = str.split("::")[1]
		process(category, data)

	#! Log incoming messages to console/file
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
				
			# Print AT messages to file
			else:
				sys.stdout.flush()
				logtofile("log-at.txt", str.replace("\n", ""))
				# save(str.replace("\n", ""))
				pass
		except :
			sleep(0.1)

def process(category, data):
	#
	#	data::SURVEYID,SURVEYDATE,DATE,TIME,RTD,PH,DO,EC,LAT,LNG,TIMESTAMP,TEMP,RH,GPSATTEMPTS,CELLSIGSTR
	#	modem-firm,11-08-2022,11/22/22,15:41:58,10.43,19.29,21.56,17.08,12.34000,-56.78000,1669131718,27,255,5,99#EOF#
	#
	subcategory = category.split(':')[1] if len(category.split(':')) > 1 else ""
	category = category.split(':')[0]

	if category.startswith("data"):
		pass

	if category.startswith("log"):
		if subcategory == "SM":
			data = data.replace(category + ":" + subcategory + "::", "")
			data = data.replace(category + ":" + subcategory, "")
			data = data.replace(category, "")
			data = data.replace(category + "::", "")
			print(data, end = '')
			sys.stdout.flush()
			# logtofile("log-at.txt", data)
		pass
	

if __name__ == "__main__":
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
				inputhandler(serial_str)

			except serial.SerialException as e:
				if(not error):
					print("\nDevice has been disconnected.")
					connect(True)
				error = True
			except AttributeError as e:
				if(not error):
					print("\nDevice has been disconnected. Reconnecting in 15 seconds")
					wait(15)
					connect(True)
				error = True
			except TypeError as e:
				if(not error):
					print("\nDevice has been disconnected. Reconnecting in 15 seconds")
					wait(15)
					connect(True)
				error = True
			except KeyboardInterrupt:
				print("\nStopping serial monitor.")
				sys.exit(0)
			except UnicodeDecodeError:
				print(" x", end = '')