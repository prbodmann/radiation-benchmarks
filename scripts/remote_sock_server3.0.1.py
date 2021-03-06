#!/usr/bin/python

import threading
import socket
import time
import os
from datetime import datetime
import sys
import requests

socketPort = 8080

# Time between checks
sleepTime = 3

# IP of the remote socket server (hardware watchdog)
serverIP = "192.168.1.5"

# Set the machines IP to check, comment the ones we are not checking
IPmachines = [
    # "192.168.1.14",  # CarolTitanX1
    # "192.168.1.2",  #CarolK401
    # "192.168.1.6",  #CarolXeon1
    # "192.168.1.7",  # CarolXeon2
    # "192.168.1.10",  # CarolAPU1
    "192.168.1.11",  # CarolAPU2
    # "192.168.1.12", #CarolX2A
    # "192.168.1.13", #CarolX2B
]

# Set the machine names for each IP
IPtoDiffReboot = {
    "192.168.1.14": 50,  # CarolTitanX1
    "192.168.1.2": 30,  # CarolK401
    "192.168.1.6": 120,  # CarolXeon1
    "192.168.1.7": 120,  # CarolXeon2
    "192.168.1.10": 30,  # CarolAPU1
    "192.168.1.11": 30,  # CarolAPU2
    "192.168.1.12": 30,  # CarolX2A
    "192.168.1.13": 30,  # CarolX2B
}

# Set the machine names for each IP
IPtoNames = {
    "192.168.1.14": "CarolTitanX1",
    "192.168.1.2": "CarolK401",
    "192.168.1.6": "CarolXeon1",
    "192.168.1.7": "CarolXeon2",
    "192.168.1.10": "CarolAPU1",
    "192.168.1.11": "CarolAPU2",
    "192.168.1.12": "CarolX2A",
    "192.168.1.13": "CarolX2B",
}

# Set the switch IP that a machine IP is connected
IPtoSwitchIP = {
    "192.168.1.14": "192.168.1.101",  # CarolTitanX1
    "192.168.1.2": "192.168.1.100",  # CarolK401
    "192.168.1.6": "192.168.1.100",  # CarolXeon1
    "192.168.1.7": "192.168.1.102",  # CarolXeon2
    "192.168.1.10": "192.168.1.101",  # CarolAPU1
    "192.168.1.11": "192.168.1.104",  # CarolAPU2
    "192.168.1.12": "192.168.1.101",  # CarolX2A
    "192.168.1.13": "192.168.1.102",  # CarolX2B
}

# Set the switch Port that a machine IP is connected
IPtoSwitchPort = {
    "192.168.1.14": 3,  # CarolTitanX1
    "192.168.1.2": 3,  # CarolK401
    "192.168.1.6": 1,  # CarolXeon1
    "192.168.1.7": 1,  # CarolXeon2
    "192.168.1.10": 1,  # CarolAPU1
    "192.168.1.11": 2,  # CarolAPU2
    "192.168.1.12": 4,  # CarolX2A
    "192.168.1.13": 4,  # CarolX2B
}

SwitchIPtoModel = {
    "192.168.1.100": "default",
    "192.168.1.101": "default",
    "192.168.1.102": "default",
    "192.168.1.103": "lindy",
    "192.168.1.104": "default",
}

# log in whatever path you are executing this script
logFile = "server.log"

# Socket server
# Create an INET, STREAMing socket
serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Maximum number of reboots
MAX_REBOOT_NUMBER = 20

# Max time in seconds to support MAX_REBOOT_NUMBER
SMALLEST_REBOOTING = 1300


# ----------------------------------------------------------------------------------------------------------------------
# Classes definition

################################################
# Reboot machine thread
################################################
class RebootMachine(threading.Thread):
    def __init__(self, address):
        """
        Rebooting machine thread, serves only for rebooting a machine
        :param address: the machine address
        """
        threading.Thread.__init__(self)
        self.address = address

    def run(self):
        """
        Overriding run method
        :return: None
        """
        port = IPtoSwitchPort[self.address]
        switchIP = IPtoSwitchIP[self.address]
        print "\tRebooting machine: " + self.address + ", switch IP: " + str(switchIP) + ", switch port: " + str(port)
        set_ip_switch(port, "Off", switchIP)
        time.sleep(10)
        set_ip_switch(port, "On", switchIP)


################################################
# Switch class
################################################
class Switch:
    """
    Class for Switch management
    """

    def __init__(self, ip, portCount):
        """
        Constructor for Switch
        :param ip: ip address
        :param portCount: port to set
        """
        self.ip = ip
        self.portCount = portCount
        self.portList = []
        for i in range(0, self.portCount):
            self.portList.append(
                'pw%1dName=&P6%1d=%%s&P6%1d_TS=&P6%1d_TC=&' %
                (i + 1, i, i, i)
            )

    def cmd(self, port, c):
        """
        Execute a c command in a specified port
        :param port: port
        :param c: command
        :return: if command was correct executed it will return 0, and if it was not other value
        """
        assert (port <= self.portCount)

        cmd = 'curl --data \"'

        # the port list is indexed from 0, so fix it
        port = port - 1

        for i in range(0, self.portCount):
            if i == port:
                cmd += self.portList[i] % c
            else:
                cmd += self.portList[i]

        cmd += '&Apply=Apply\" '
        cmd += 'http://%s/tgi/iocontrol.tgi ' % self.ip
        cmd += '-o /dev/null 2>/dev/null'
        return os.system(cmd)

    def on(self, port):
        return self.cmd(port, 'On')

    def off(self, port):
        return self.cmd(port, 'Off')


################################################
# Class to handle machines
################################################
class HandleMachines(threading.Thread):
    def run(self):
        """
        Overriding for run method
        :return: None
        """
        print "\tStarting thread to check machine connections"
        while 1:
            check_machines()
            time.sleep(sleepTime)


# End classes definition
# ----------------------------------------------------------------------------------------------------------------------

################################################
# Log messages adding timestamp before the message
################################################
def log_msg(msg):
    """
    # Log messages adding timestamp before the message
    :param msg: the message tha will be logged
    :return: None
    """
    now = datetime.now()
    fp = open(logFile, 'a')
    print >> fp, now.ctime() + ": " + str(msg)
    fp.close()


################################################
# Routines to perform power cycle user IP SWITCH
################################################
def replace_str_index(text, index=0, replacement=''):
    """
    # Routines to perform power cycle user IP SWITCH
    :param text:
    :param index:
    :param replacement:
    :return: a formatted string
    """
    return '%s%s%s' % (text[:index], replacement, text[index + 1:])


################################################
# Lindy switch specific set
################################################
def lindy_switch(portNumber, status, switchIP):
    """
    # Lindy switch specific set
    :param portNumber: port number of the switch
    :param status:
    :param switchIP:
    :return:
    """
    led = replace_str_index("000000000000000000000000", portNumber - 1, "1")

    if status == "On":
        url = 'http://' + switchIP + '/ons.cgi?led=' + led
    else:
        url = 'http://' + switchIP + '/offs.cgi?led=' + led
    payload = {
        "led": led,
    }
    headers = {
        "Host": switchIP,
        "User-Agent": "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.12; rv:56.0) Gecko/20100101 Firefox/56.0",
        "Accept": "*/*",
        "Accept-Language": "en-US,en;q=0.5",
        "Accept-Encoding": "gzip, deflate",
        "Referer": "http://" + switchIP + "/outlet.htm",
        "Authorization": "Basic c25tcDoxMjM0",
        "Connection": "keep-alive",
        "Content-Length": "0",
    }

    try:
        return requests.post(url, data=json.dumps(payload), headers=headers)
    except BaseException as ex:
        log_msg(
            "Could not change Lindy IP switch status, portNumber: " + portNumber + ", status" + status + ", switchIP:" + switchIP)
        print(str(ex))
        return 1


################################################
# Set ip switch
################################################
def set_ip_switch(portNumber, status, switchIP):
    """
    Set switch to a switchIP
    :param portNumber:
    :param status:
    :param switchIP:
    :return:
    """
    if status == 'on' or status == 'On' or status == 'ON':
        cmd = 'On'
    elif status == 'off' or status == 'Off' or status == 'OFF':
        cmd = 'Off'
    else:
        return 1
    if SwitchIPtoModel[switchIP] == "default":
        s = Switch(switchIP, 4)
        return s.cmd(int(portNumber), cmd)
    elif SwitchIPtoModel[switchIP] == "lindy":
        return lindy_switch(int(portNumber), cmd, switchIP)


################################################
# Socket server
################################################
def start_socket():
    """
    Start socket server
    :return: None
    """
    # Bind the socket to a public host, and a well-known port
    serverSocket.bind((serverIP, socketPort))
    print "\tServer bind to: ", serverIP
    # Become a server socket
    serverSocket.listen(15)

    while 1:
        # Accept connections from outside
        (clientSocket, address) = serverSocket.accept()
        print "connection from " + str(address[0])
        if address[0] in IPmachines:
            IPLastConn[address[0]] = time.time()  # Set new timestamp
            # If machine was set to not check again, now it's alive so start to check again
            # This branch will verify if the machine was rebooted so many times that it cant keep going
            IPActiveTest[address[0]] = True

        clientSocket.close()


################################################
# Check if the reboot interval is feasible
################################################
def check_reboot_interval(address):
    """
    # Check if the reboot interval is feasible
    :param address: the ip address
    :return: True if it is possible to reboot
    """
    global IPMaximumReboot, rebooting

    last_reboot = IPMaximumReboot[address]["last_reboot"] = datetime.fromtimestamp(int(rebooting[address]))
    now = datetime.now()
    time_diff = (now - last_reboot).total_seconds()
    print ("reboot_times {} last_reboot {} now {}".format(IPMaximumReboot[address][
        "reboot_times"], last_reboot, now))

    if time_diff < SMALLEST_REBOOTING and IPMaximumReboot[address][
        "reboot_times"] > MAX_REBOOT_NUMBER:
        return False
    elif time_diff > SMALLEST_REBOOTING:
        print("PASSOU AQUI TIME DIFF {}".format(time_diff))
        IPMaximumReboot[address]["reboot_times"] = 0

    IPMaximumReboot[address]["reboot_times"] += 1
    return True


################################################
# Routines to check machine status
################################################
def check_machines():
    """
    # Check machines while running
    :return:
    """
    for address, timestamp in IPLastConn.copy().iteritems():
        # If machine had a boot problem, stop rebooting it
        if not IPActiveTest[address]:
            continue

        # Check if machine is working fine
        now = datetime.now()
        then = datetime.fromtimestamp(timestamp)
        seconds = (now - then).total_seconds()
        # If machine is not working fine reboot it
        if IPtoDiffReboot[address] < seconds < 3 * IPtoDiffReboot[address]:
            reboot = datetime.fromtimestamp(rebooting[address])
            if (now - reboot).total_seconds() > IPtoDiffReboot[address] and check_reboot_interval(address=address):
                rebooting[address] = time.time()
                if address in IPtoNames:
                    print "Rebooting IP " + address + " (" + IPtoNames[address] + ")"
                    log_msg("Rebooting IP " + address + " (" + IPtoNames[address] + ")")
                else:
                    print "Rebooting IP " + address
                    log_msg("Rebooting IP " + address)
                # Reboot machine in another thread
                RebootMachine(address).start()

        # If machine did not reboot, log this and set it to not check again
        elif 3 * IPtoDiffReboot[address] < seconds < 10 * IPtoDiffReboot[address]:
            if address in IPtoNames:
                print "Boot Problem IP " + address + " (" + IPtoNames[address] + ")"
                log_msg("Boot Problem IP " + address + " (" + IPtoNames[address] + ")")
            else:
                print "Boot Problem IP " + address
                log_msg("Boot Problem IP " + address)

            IPActiveTest[address] = False

        elif seconds > 10 * IPtoDiffReboot[address]:
            if address in IPtoNames:
                print "Rebooting IP " + address + " (" + IPtoNames[address] + ")"
                log_msg("Rebooting IP " + address + " (" + IPtoNames[address] + ")")
            else:
                print "Rebooting IP " + address
                log_msg("Rebooting IP " + address)
            RebootMachine(address).start()


################################################
# Main steps execution function
################################################
def main():
    """
    Main function for the server
    :return: None
    """
    global IPLastConn, IPActiveTest, rebooting, IPMaximumReboot
    try:
        # Set the initial timestamp for all IPs
        IPLastConn = dict()
        IPActiveTest = dict()
        # Set the maximum number of reboots
        IPMaximumReboot = dict()

        rebooting = dict()
        for ip in IPmachines:
            rebooting[ip] = time.time()
            IPLastConn[ip] = time.time()  # Current timestamp
            IPActiveTest[ip] = True
            port = IPtoSwitchPort[ip]
            switchIP = IPtoSwitchIP[ip]
            set_ip_switch(port, "On", switchIP)

            # set the counters to zero
            IPMaximumReboot[ip] = {"reboot_times": 0, "last_reboot": None}

        handle = HandleMachines()
        handle.setDaemon(True)
        handle.start()
        start_socket()
    except KeyboardInterrupt:
        print "\n\tKeyboardInterrupt detected, exiting gracefully!( at least trying :) )"
        serverSocket.close()
        sys.exit(1)


################################################
# Main Execution
################################################
if __name__ == '__main__':
    main()

"""
DEPRECATED
"""

# datetime.fromtimestamp(time.time())
# serverIP = socket.gethostbyname(socket.gethostname())

# """
#    Class HandleMachines
#    """
#
#
# def __init__(self):
#     """
#     Constructor method
#     """
#     threading.Thread.__init__(self)
