from tkinter import W
import serial
from time import sleep
from datetime import datetime
import sys

from rich import box
from rich.align import Align
from rich.console import Console, Group
from rich.layout import Layout
from rich.panel import Panel
from rich.progress import Progress, SpinnerColumn, BarColumn, TextColumn
from rich.syntax import Syntax
from rich.table import Table
from rich.text import Text
from rich.live import Live
from time import sleep

console = Console()

def make_layout() -> Layout:
    layout = Layout(name="root")

    layout.split(
        Layout(name="header", size=3),
        Layout(name="display", size=10),
        Layout(name="main"),
        # Layout(name="footer", size=7),
    )
    layout["main"].split_row(
        Layout(name="monitor"),
    )
    layout["display"].split_row(
        Layout(name="one"),
        Layout(name="two"),
        Layout(name="three"),
        Layout(name="four"),
        Layout(name="five"),
    )
    layout["one"].split_column(
        Layout(name="1"),
        Layout(name="2"),
    )
    layout["two"].split_column(
        Layout(name="3"),
        Layout(name="4"),
    )
    layout["three"].split_column(
        Layout(name="5"),
        Layout(name="6"),
    )
    layout["four"].split_column(
        Layout(name="7"),
        Layout(name="8"),
    )
    layout["five"].split_column(
        Layout(name="9"),
        Layout(name="10"),
    )
    return layout

def show_header():

    grid = Table.grid(expand=True)
    grid.add_column(justify="left")
    grid.add_column(justify="left", ratio=1)
    grid.add_column(justify="right")
    grid.add_row(
        "[b]GatorByte Monitor[/b] v1.0",
        " [green]Connected on " + str(conport) if constatus else " [red]Not connected",
        datetime.now().ctime().replace(":", "[blink]:[/]"),
    )
    return Panel(grid)

logtext = ""
def show_log(msg):
    global logtext
    logtext += msg
    newlog = Text.from_markup(logtext)

    message_panel = Panel(
        newlog,
        padding=(1, 2),
        title="[b white]Serial monitor",
        border_style="green"
    )

    return message_panel


def show_parameter(location, parameter, text, unit=""):

    value = Text.from_markup(text + " " + unit)

    message = Table.grid(padding=0)
    message.add_column()
    message.add_row(value)

    message_panel = Panel(
        Align.center(value),
        box=box.ROUNDED,
        padding=(1, 0),
        title="[b white]" + parameter,
        border_style="orange1"
    )
    layout[str(location)].update(message_panel)

def blank_reading_panel():

    text = "N/A"
    value = Text.from_markup(text)

    message = Table.grid(padding=1)
    message.add_column()
    message.add_column(no_wrap=True)
    message.add_row(value)

    message_panel = Panel(
        Align.center(
            value
        ),
        box=box.ROUNDED,
        padding=(1, 2),
        title="[b white]-",
        border_style="#444444"
    )
    return message_panel

ser = None
command_mode = False

constatus = False
conport = 0

if __name__ == "__main__":

    layout = make_layout()

    while(True):
        for i in range(9, 11):
            layout[str(i)].update(blank_reading_panel())

        with Live(layout, refresh_per_second=10, screen=True):
            counter = 0;
            while True:
                
                import random
                counter = counter + (1 if (random.random() == 0) else -1)
                
                layout["header"].update(show_header())

                show_parameter(1, "Temperature", str(round(26.790 + counter / 10, 2)), "Celcius")
                show_parameter(2, "pH", str(round(7.938 + counter / 23, 2)))
                show_parameter(3, "D.O.", "11.45", "mg/L")
                show_parameter(4, "E.C.", "1144.55", "uS/cm")
                show_parameter(5, "Internal temperature", "26.79", "Celcius")
                show_parameter(6, "Internal humidity", "56", "%")
                show_parameter(7, "GPS", "1144.5566, 1144.5566")
                show_parameter(8, "RTC", "July 12th, 12:03:56")
                
                for i in range(9, 11):
                    layout[str(i)].update(blank_reading_panel())
                    
                layout["header"].update(show_header())


                # Connect
                baud = 9600
                baseports = ['/dev/ttyUSB', '/dev/ttyACM', 'COM', '/dev/tty.usbmodem1234']
                if "-p" in sys.argv:
                    ports = sys.argv[sys.argv.index("-p") + 1].split(",")

                    for port in ports:
                        try:
                            ser = serial.Serial(port, baud, timeout=1)
                            constatus = True
                            conport = port
                            
                            break
                        except:
                            ser = None
                            constatus = False
                            conport = 0
                else:
                    for baseport in baseports:
                        if ser:
                            break
                        for i in range(0, 64):
                            try:
                                port = baseport + str(i)
                                ser = serial.Serial(port, baud, timeout=1)
                                
                                constatus = True
                                conport = port
                                
                                break
                            except:
                                ser = None
                
                # Read
                try:
                    serial_str = str(ser.readline().decode())
                    if(serial_str == ""): 
                        continue

                    # Send incoming string for processing
                    layout["monitor"].update(show_log(serial_str))
                except:
                    constatus = False
                    conport = 0
                sleep(1)
                pass