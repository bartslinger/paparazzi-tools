import wx
import sys
import time
import threading

import serialmessagelink

from os import path, getenv

# if PAPARAZZI_SRC not set, then assume the tree containing this
# file is a reasonable substitute
PPRZ_SRC = getenv("PAPARAZZI_SRC", 
    path.normpath(
    path.join(path.dirname(path.abspath(__file__)), '../../../../')))
sys.path.append(PPRZ_SRC + "/sw/lib/python")
from settings_xml_parse import PaparazziACSettings

PPRZ_HOME = getenv("PAPARAZZI_HOME", PPRZ_SRC)

WIDTH = 480
HEIGHT = 320

class SDLogDownloadFrame(wx.Frame):
    def __init__(self, options, msg_class='telemetry'):
        self.ac_id = options['ac_id'][0]
        self.settings = PaparazziACSettings(self.ac_id)
        self.msg_class = msg_class

        # Init serial message link
        self.InitSerialMessageLink(options['port'][0], options['baud'][0])

        # GUI
        wx.Frame.__init__(self, id=-1,
            parent=None, name=u'MessagesFrame',
            size=wx.Size(WIDTH, HEIGHT),
            style=wx.DEFAULT_FRAME_STYLE,
            title=u'SD Logger Download Tool')
        self.Bind(wx.EVT_CLOSE, self.OnClose)

        # Menubar
        menuBar = wx.MenuBar()
        fileMenu = wx.Menu()
        fitem = fileMenu.Append(wx.ID_EXIT,
            'Quit', 'Close the SD Logger Download Tool')
        self.Bind(wx.EVT_MENU, self.OnClose, fitem)
        advancedMenu = wx.Menu()
        fitem = advancedMenu.Append(1, 'Log Index', 'Show log index')
        self.Bind(wx.EVT_MENU, self.OnLogIndexRequest, fitem)
        menuBar.Append(fileMenu, '&File')
        menuBar.Append(advancedMenu, '&Advanced')
        self.SetMenuBar(menuBar)

        # Other widgets
        self.inDataLabel = wx.StaticText(self, id=12, label="",
            pos=wx.Point(20, 200))
        self.statusDataLabel = wx.StaticText(self, id=13, label="",
            pos=wx.Point(20, 220))
        self.progressBar = wx.Gauge(self, range=100, 
            pos=wx.Point(100, 50), size=wx.Size(280, 30))
        self.downloadButton = wx.Button(self, id=3, label="Download!",
            pos=wx.Point(190, 150), size=wx.Size(100, 25))
        self.Bind(wx.EVT_BUTTON, self.OnButton)

        sizer = wx.BoxSizer(wx.HORIZONTAL)
        self.SetSizer(sizer)
        sizer.Layout()

    # Called on button push
    def OnButton(self, event):
        button = event.GetEventObject()
        index = int(button.GetId())
        setting_index = self.settings.name_lookup["sdlogger_spi.command"].index
       #print setting_index
        if index == 3:
            print "JOLOOLOLOOOOOOOOOOOOOOOOOOOOOOOOOOO"
            self.msglink.sendMessage('datalink', 'SETTING', (setting_index, self.ac_id, 255))
            #self.downloadButton.Disable()

    def OnLogIndexRequest(self, event):
        setting_index = self.settings.name_lookup["sdlogger_spi.command"].index
        ###IvySendMsg("dl DL_SETTING %s %s %s" % (self.ac_id, setting_index, 3))
        #self.msglink.sendMessage('datalink', 'SETTING', (setting_index, self.ac_id, 255))

    def OnSettingConfirmation(self, larg):
        wx.CallAfter(self.process_setting_confirmation, larg)

    def process_setting_confirmation(self, msg):
        print "OK"
        cmd_idx = self.settings.name_lookup["sdlogger_spi.command"].index
        given_idx = msg.payload_items[0]
        print given_idx
        if given_idx == cmd_idx:
            print "Confirmation received"

    def InitSerialMessageLink(self, port, baud):
        self.msglink = serialmessagelink.SerialMessageLink(port, baud)
        self.msglink.subscribe("DL_VALUE", self.OnSettingConfirmation)
        #self.msglink.subscribe("LOG_DATAPACKET", self.OnLogPacketReceive)

    def OnClose(self, event):
        self.msglink.close()
        print "msg closed"
        self.Destroy()
