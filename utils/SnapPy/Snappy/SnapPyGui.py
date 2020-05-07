# SNAP: Servere Nuclear Accident Programme
# Copyright (C) 1992-2020   Norwegian Meteorological Institute
#
# This file is part of SNAP. SNAP is free software: you can
# redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

import os
import sys
import glob
import datetime
import logging

import subprocess

from PyQt5.QtWidgets import QMainWindow, QApplication, QMessageBox, QScrollArea, QLabel, QGridLayout
from PyQt5.QtCore import Qt, QRegExp

#Generate main window gui with pyuic5 -o MainWindow.py MainWindow.ui
from Snappy.MainWindow import Ui_MainWindow
from Snappy.SnapPyNAGui import SnapPyNAGui


class SnapPyGui(QMainWindow, Ui_MainWindow):
    def __init__(self, *args, danger=False, **kwargs):
        super(SnapPyGui, self).__init__(*args, **kwargs)

        self.setupUi(self)

        self.logger = logging.getLogger("SnapPyGui")

        self.nuclear_accident = SnapPyNAGui(self)
        self.nuclear_accident.setupUi()

        self.show()
