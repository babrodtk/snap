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
import re
import glob
import datetime
import logging
import argparse

import subprocess

from PyQt5.QtGui import QValidator, QIntValidator, QDoubleValidator, QRegExpValidator
from PyQt5.QtWidgets import QMainWindow, QApplication, QMessageBox, QScrollArea, QLabel, QGridLayout
from PyQt5.QtCore import Qt, QRegExp

#Generate with pyuic5 -o MainWindow.py MainWindow.ui
from MainWindow import Ui_MainWindow
from Resources import Resources, MetModel



class SnapPyGui(QMainWindow, Ui_MainWindow):
    def __init__(self, *args, danger=False, **kwargs):
        super(SnapPyGui, self).__init__(*args, **kwargs)
        self.setupUi(self)

        self.logger = logging.getLogger("SnapPyGui")

        self.resources = Resources()

        self.setup_snap_na_gui()

        self.show()


    def setup_snap_na_gui(self):
        self.logger.debug("Setting up SNAP nuclear accident GUI elements")

        #Check input arguments in Nuclear source term box
        self.snap_na_duration.setValidator(QDoubleValidator(0, 2000, 2))
        self.snap_na_run_length.setValidator(QDoubleValidator(0, 2000, 2))

        #Nulcear source term box
        self.snap_na_location.addItem("Select", None)
        self.snap_na_location.addItem("Latitude-Longitude", "latlon")
        self.snap_na_latitude_value = ""
        self.snap_na_longitude_value = ""
        for key, value in self.resources.readNPPs().items():
            self.snap_na_location.addItem(key, value)
        def na_location_changed():
            if (self.snap_na_location.currentData() == "latlon"):
                self.snap_na_latitude.setEnabled(True)
                self.snap_na_longitude.setEnabled(True)
                self.snap_na_latitude.setText(self.snap_na_latitude_value)
                self.snap_na_longitude.setText(self.snap_na_longitude_value)
            elif (self.snap_na_location.currentData() is not None):
                self.snap_na_latitude.setEnabled(False)
                self.snap_na_longitude.setEnabled(False)
                self.snap_na_latitude.setText(str(self.snap_na_location.currentData()['lat']))
                self.snap_na_longitude.setText(str(self.snap_na_location.currentData()['lon']))
        self.snap_na_location.currentIndexChanged.connect(na_location_changed)
        self.snap_na_location.setMaxVisibleItems(10)
        self.snap_na_location.view().setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        na_location_changed()

        class LatLonValidator(QValidator):
            """
            Class which validates different latitude/longitude formats
            """
            def __init__(self, latitude=False, longitude=False, parent=None):
                super(LatLonValidator, self).__init__(parent)

                self.logger = logging.getLogger("LatLonValidator")

                if latitude:
                    self.lmax = 90
                    self.lmin = -90
                    ldir = "NSns"
                elif longitude:
                    self.lmax = 180
                    self.lmin = -180
                    ldir = "EWewØVøv"

                #Check formats including
                # decimal degrees -3.54
                self.doubleValidator = QDoubleValidator()
                #format 1: 3:5:3 S
                self.degreesRegex1 = re.compile(
                    r"""
                    (?P<match>
                        (?P<degrees>[ +-]?\d{1,3})
                        (?:
                            :
                            (?:
                                (?P<minutes>\d{1,2})
                                (?:
                                    :
                                    (?:
                                        (?P<seconds>\d{1,2}(.\d+)?)
                                    )?
                                )?
                            )?
                        )?
                        (?:
                            \s{0,1}
                            (?:
                                (?P<direction>[""" + ldir + r"""])
                            )
                        )?
                    )
                    """, re.VERBOSE)
                #format 2: 3° 5' 34"S
                self.degreesRegex2 = re.compile(
                    r"""
                    (?P<match>
                        (?P<degrees>[ +-]?\d{1,3})
                        (?:
                            ° #Degrees symbol
                            (?:
                                (?P<minutes>\d{1,2})
                                (?:
                                    ['’] #minutes symbol
                                    (?:
                                        (?P<seconds>\d{1,2}(.\d+)?)
                                        (?:
                                            ["”] #Seconds symbol
                                        )?
                                    )?
                                )?
                            )?
                        )?
                        (?:
                            \s{0,1}
                            (?:
                                (?P<direction>[""" + ldir + r"""])
                            )
                        )?
                    )
                    """, re.VERBOSE)

            def fixup(self, text):
                return text

            def validate(self, text, pos):
                #No text
                if (len(text) == 0):
                    return (QValidator.Intermediate, text, pos)
                else:
                    d_val = self.doubleValidator.validate(text, pos)
                    #Pure double text
                    if (d_val[0] == QValidator.Acceptable or d_val[0] == QValidator.Intermediate):
                        if (float(text) >= self.lmin and float(text) <= self.lmax):
                            return d_val
                        else:
                            return (QValidator.Invalid, d_val[1], d_val[2])
                    #Non-trivial format
                    else:
                        m1 = self.degreesRegex1.match(text)
                        m2 = self.degreesRegex2.match(text)

                        def decimalDegrees(degrees, minutes, seconds, direction):
                            """
                            Convert degrees, minutes, seconds, direction to corresponding decimal degree
                            """
                            if minutes is None: minutes = 0
                            if seconds is None: seconds = 0
                            if direction == "S" or direction == "W":
                                direction = -1
                            else:
                                direction = 1
                            return direction * (float(degrees) + float(minutes)/60 + float(seconds)/3600)

                        degrees = None
                        #If both format 1 and format 2 matches
                        if m1 and m2:
                            #Use the one which matches best
                            if len(m1.group("match")) > len(m2.group("match")):
                                m = m1
                            else:
                                m = m2
                        #If only format 1 matches
                        elif m1:
                            m = m1
                        #If only format 2 matches
                        elif m2:
                            m = m2
                        #No matching format
                        else:
                            return (QValidator.Invalid, text, pos)

                        text = m.group("match")
                        degrees = m.group("degrees")
                        minutes = m.group("minutes")
                        seconds = m.group("seconds")
                        direction = m.group("direction")
                        decimal_degrees = decimalDegrees(degrees, minutes, seconds, direction)

                        #Check values are in range
                        if (decimal_degrees < self.lmin or decimal_degrees > self.lmax):
                            return (QValidator.Invalid, text, pos)
                        #Check minutes in range
                        elif (minutes is not None and (float(minutes) < 0 or float(minutes) > 60)):
                            return (QValidator.Invalid, text, pos)
                        #Check seconds in range
                        elif (seconds is not None and (float(seconds) < 0 or float(seconds) > 60)):
                            return (QValidator.Invalid, text, pos)
                        #Check if incomplete format
                        elif (degrees is not None or
                                minutes is not None or
                                seconds is not None or
                                direction is not None):
                            return (QValidator.Intermediate, text, pos)
                        else:
                            return (QValidator.Acceptable, text, pos)

        #Set up latitude box
        self.snap_na_latitude.setValidator(LatLonValidator(latitude=True))
        def snap_na_latitude_text_edited(text):
            self.snap_na_latitude_value = text
        self.snap_na_latitude.textEdited.connect(snap_na_latitude_text_edited)

        #Longitude box
        self.snap_na_longitude.setValidator(LatLonValidator(longitude=True))
        def snap_na_longitude_text_edited(text):
            self.snap_na_longitude_value = text
        self.snap_na_longitude.textEdited.connect(snap_na_longitude_text_edited)

        #Connect "show advanced settings" check box
        self.snap_na_advanced_settings.setVisible(False)
        self.snap_na_show_advanced.stateChanged.connect(
            lambda: self.snap_na_advanced_settings.setVisible(self.snap_na_show_advanced.isChecked())
        )

        #Check input arguments in advanced box
        self.snap_na_met_model.addItem("EC 0.1, NRPA-domain", MetModel.NrpaEC0p1)
        self.snap_na_met_model.addItem("MEPS 2.5km control-member", MetModel.Meps2p5)
        self.snap_na_met_model.addItem("EC 0.1, global, slow", MetModel.NrpaEC0p1Global)
        self.snap_na_met_model.addItem("Hirlam12", MetModel.H12)
        def snap_na_met_model_changed():
            current_data = self.snap_na_met_model.currentData()
            if (current_data is MetModel.NrpaEC0p1 or current_data is MetModel.NrpaEC0p1Global):
                self.snap_na_met_ec_runtime.setEnabled(True)
            else:
                self.snap_na_met_ec_runtime.setEnabled(False)
        self.snap_na_met_model.currentIndexChanged.connect(snap_na_met_model_changed)

        #EC Meteorology dropdown
        self.snap_na_met_ec_runtime.addItem("Best mixed", "best")
        for run in self.resources.getECRuns():
            self.snap_na_met_ec_runtime.addItem(run)

        self.snap_na_release_npp.toggled.connect(
            lambda val: (
                    self.snap_na_release_npp.setChecked(val),
                    self.snap_na_release_bomb.setChecked(not val)
                    )
        )
        self.snap_na_release_bomb.toggled.connect(
            lambda val: (
                    self.snap_na_release_npp.setChecked(not val),
                    self.snap_na_release_bomb.setChecked(val)
                    )
        )


        #Add items to bomb yield selector
        bomb_params = [
            {'yield': 1, 'lower': 500, 'upper': 1500, 'radius': 600, 'activity': 2.0e19},
            {'yield': 3, 'lower': 1400, 'upper': 3100, 'radius': 1000, 'activity': 6.0e19},
            {'yield': 10, 'lower': 2250, 'upper': 4750, 'radius': 1400, 'activity': 2.0e20},
            {'yield': 30, 'lower': 4100, 'upper': 8400, 'radius': 2300, 'activity': 6.0e20},
            {'yield': 100, 'lower': 5950, 'upper': 12050, 'radius': 3200, 'activity': 2.0e21},
            {'yield': 300, 'lower': 8000, 'upper': 18500, 'radius': 5800, 'activity': 6.0e21},
            {'yield': 1000, 'lower': 10000, 'upper': 25000, 'radius': 8500, 'activity': 2.0e22},
            {'yield': 3000, 'lower': 12000, 'upper': 32000, 'radius': 11100, 'activity': 6.0e22}
        ]
        for item in bomb_params:
            self.snap_na_bomb_yield.addItem("{:d} kt".format(item['yield']), item)
        self.snap_na_bomb_yield.setCurrentIndex(2)

        self.snap_na_bottom.setText("15")
        self.snap_na_bottom.setValidator(QDoubleValidator(0, 50000, 2))
        self.snap_na_bottom.textChanged.connect(
            lambda val: self.snap_na_top.validator().setBottom(float(val))
        )
        self.snap_na_top.setText("500")
        self.snap_na_top.setValidator(QDoubleValidator(0, 50000, 2))
        self.snap_na_top.textChanged.connect(
            lambda val: self.snap_na_bottom.validator().setTop(float(val))
        )
        self.snap_na_radius.setText("50")
        self.snap_na_radius.setValidator(QDoubleValidator(0, 50000, 2))
        self.snap_na_cs137.setText("2.6e+11")
        self.snap_na_cs137.setValidator(QDoubleValidator(0, 1.0e100, 5))
        self.snap_na_xe133.setText("1.0e+13")
        self.snap_na_xe133.setValidator(QDoubleValidator(0, 1.0e100, 5))
        self.snap_na_i131.setText("1.39e+13")
        self.snap_na_i131.setValidator(QDoubleValidator(0, 1.0e100, 5))

        #plot region data
        self.snap_na_plot_region.addItem("No E-Mail", "")
        self.snap_na_plot_region.addItem("Geografisk", "Geografisk")
        self.snap_na_plot_region.addItem("Merkator", "Merkator")
        self.snap_na_plot_region.addItem("Globalt", "Globalt")
        self.snap_na_plot_region.addItem("N.halvkule", "N.halvkule")
        self.snap_na_plot_region.addItem("N.halvkule-90", "N.halvkule-90")
        self.snap_na_plot_region.addItem("N.halvkule+90", "N.halvkule+90")
        self.snap_na_plot_region.addItem("S.halvkule", "S.halvkule")
        self.snap_na_plot_region.addItem("S.halvkule+180", "S.halvkule+180")
        self.snap_na_plot_region.addItem("Atlant", "Atlant")
        self.snap_na_plot_region.addItem("Europa", "Europa")
        self.snap_na_plot_region.addItem("N-Europa", "N-Europa")
        self.snap_na_plot_region.addItem("Skandinavia", "Skandinavia")
        self.snap_na_plot_region.addItem("Norge", "Norge")
        self.snap_na_plot_region.addItem("S-Norge", "S-Norge")
        self.snap_na_plot_region.addItem("N-Norge", "N-Norge")
        self.snap_na_plot_region.addItem("Norge.20W", "Norge.20W")
        self.snap_na_plot_region.addItem("VA-Norge", "VA-Norge")
        self.snap_na_plot_region.addItem("VV-Norge", "VV-Norge")
        self.snap_na_plot_region.addItem("VNN-Norge", "VNN-Norge")

        #self.snap_na_bdiana_version.setValidator(QDoubleValidator(0, 50000, 5))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='GUI for publishing Ash simulations')
    #parser.add_argument('--danger', help="Use proper directories", action="store_true")
    args = parser.parse_args()

    app = QApplication([])

    logging.basicConfig(format='%(asctime)s (%(levelname)s) : %(message)s',
                         level=logging.INFO, stream=sys.stdout)
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)

    window = SnapPyGui()

    logger.debug("Starting now")
    app.exec_()
