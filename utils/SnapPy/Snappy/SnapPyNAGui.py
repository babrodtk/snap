import logging
import pprint
import re

from PyQt5.QtWidgets import QMessageBox
from PyQt5.QtGui import QValidator, QIntValidator, QDoubleValidator, QRegExpValidator
from PyQt5.QtCore import Qt, QRegExp, QDateTime, QTime

from Snappy.SnapController import SnapController
from Snappy.Resources import Resources, MetModel

class SnapPyNAGui():
    """
    Class which handles the snap nuclear accident GUI
    """

    def __init__(self, gui):
        self.logger = logging.getLogger("SnapPyNAGui")
        self.resources = Resources()
        self.gui = gui
        self.logger.debug("Creating snap controller")
        self.controller = SnapController()
        self.logger.debug("Done creating snap controller")


    def validate(self):
        #Checks that all GUI elements make somewhat sense
        errors = []
        if not self.gui.snap_na_start_time.dateTime():
            errors += ["Invalid start time"]
        if not self.gui.snap_na_run_length.text():
            errors += ["Invalid run length"]
        if not self.gui.snap_na_duration.text():
            errors += ["Invalid emission duration length"]
        if not self.gui.snap_na_location.currentData():
            errors += ["Invalid emission location"]
        if not self.gui.snap_na_latitude.text():
            errors += ["Invalid latitude"]
        if not self.gui.snap_na_longitude.text():
            errors += ["Invalid longitude"]
        if not self.gui.snap_na_met_model.currentData():
            errors += ["Invalid met model"]
        if self.gui.snap_na_release_bomb.isChecked() == self.gui.snap_na_release_npp.isChecked():
            errors += ["Type must be bomb or nuclear power plant"]
        if self.gui.snap_na_release_bomb.isChecked():
            if not self.gui.snap_na_bomb_yield.currentData():
                errors += ["Invalid bomb yield"]
        if self.gui.snap_na_release_npp.isChecked():
            if not self.gui.snap_na_bottom.text():
                errors += ["Invalid release bottom"]
            if not self.gui.snap_na_top.text():
                errors += ["Invalid release top"]
            if not self.gui.snap_na_radius.text():
                errors += ["Invalid release radius"]
            if not self.gui.snap_na_cs137.text():
                errors += ["Invalid Cs137 release"]
            if not self.gui.snap_na_xe133.text():
                errors += ["Invalid Xe133 release"]
            if not self.gui.snap_na_i131.text():
                errors += ["Invalid I131 release"]

        if errors:
            QMessageBox.warning(self.gui, "Errors in parameters",
                "Could not parse required input data:\n * " + "\n * ".join(errors))
            return False
        else:
            return True


    def run(self):
        self.logger.debug("Running snap")

        #Create query dictionary from gui elements
        q_dict = {}
        q_dict['startTime'] = self.gui.snap_na_start_time.dateTime().toPyDateTime()
        q_dict['runTime'] = self.gui.snap_na_run_length.text()
        q_dict['releaseTime'] = self.gui.snap_na_duration.text()
        if not self.gui.snap_na_location.currentData() or self.gui.snap_na_location.currentData() == 'latlon':
            q_dict['npp'] = False
        else:
            q_dict['npp'] = self.gui.snap_na_location.currentData()['site']
        q_dict['latitude'] = self.gui.snap_na_latitude.text()
        q_dict['longitude'] = self.gui.snap_na_longitude.text()

        q_dict['metmodel'] = self.gui.snap_na_met_model.currentData()
        q_dict['ecmodelrun'] = self.gui.snap_na_met_ec_runtime.currentData()

        if (self.gui.snap_na_release_bomb.isChecked()):
            q_dict['isBomb'] = True
            q_dict['yield'] = self.gui.snap_na_bomb_yield.currentData()

        if (self.gui.snap_na_release_npp.isChecked()):
            q_dict['lowerHeight'] = self.gui.snap_na_bottom.text()
            q_dict['upperHeight'] = self.gui.snap_na_top.text()
            q_dict['radius'] = self.gui.snap_na_radius.text()
            q_dict['relI131'] = self.gui.snap_na_cs137.text()
            q_dict['relXE133'] = self.gui.snap_na_xe133.text()
            q_dict['relCS137'] = self.gui.snap_na_i131.text()

        q_dict['dianaversion'] = self.gui.snap_na_bdiana_version.text()
        q_dict['region'] = self.gui.snap_na_plot_region.currentData()


        self.logger.debug(pprint.pformat(q_dict))

        if not self.validate():
            self.logger.error("Invalid arguments")
            return

        #run_snap_query
        self.controller.run_snap_query(q_dict)




    def setupUi(self):
        self.logger.debug("Setting up SNAP nuclear accident GUI elements")

        #Set now as current data
        now = QDateTime.currentDateTimeUtc()
        now.setTime(QTime(now.time().hour(), 0, 0))
        self.gui.snap_na_start_time.setDateTime(now)

        #Check input arguments in Nuclear source term box
        self.gui.snap_na_duration.setValidator(QDoubleValidator(0, 2000, 2))
        self.gui.snap_na_duration.validator().setNotation(QDoubleValidator.StandardNotation)
        self.gui.snap_na_run_length.setValidator(QDoubleValidator(0, 2000, 2))
        self.gui.snap_na_run_length.validator().setNotation(QDoubleValidator.StandardNotation)

        def snap_na_duration_text_changed():
            a = self.gui.snap_na_duration.text()
            b = self.gui.snap_na_run_length.text()
            if a and b and float(a) > float(b):
                self.gui.snap_na_run_length.setText(a)
        self.gui.snap_na_duration.editingFinished.connect(snap_na_duration_text_changed)

        def snap_na_run_length_text_changed():
            a = self.gui.snap_na_duration.text()
            b = self.gui.snap_na_run_length.text()
            if a and b and float(a) > float(b):
                self.gui.snap_na_duration.setText(b)
        self.gui.snap_na_run_length.editingFinished.connect(snap_na_run_length_text_changed)

        #Nulcear source term box
        self.gui.snap_na_location.addItem("Select", None)
        self.gui.snap_na_location.addItem("Latitude-Longitude", "latlon")
        self.gui.snap_na_latitude_value = ""
        self.gui.snap_na_longitude_value = ""
        plantBB = {"west": -60, "east": 70, "north": 85, "south": 30}
        for key, value in self.resources.readNPPs(plantBB).items():
            self.gui.snap_na_location.addItem(key, value)
        def na_location_changed():
            if (self.gui.snap_na_location.currentData() == "latlon"):
                self.gui.snap_na_latitude.setEnabled(True)
                self.gui.snap_na_longitude.setEnabled(True)
                self.gui.snap_na_latitude.setText(self.gui.snap_na_latitude_value)
                self.gui.snap_na_longitude.setText(self.gui.snap_na_longitude_value)
            elif (self.gui.snap_na_location.currentData() is not None):
                self.gui.snap_na_latitude.setEnabled(False)
                self.gui.snap_na_longitude.setEnabled(False)
                self.gui.snap_na_latitude.setText(str(self.gui.snap_na_location.currentData()['lat']))
                self.gui.snap_na_longitude.setText(str(self.gui.snap_na_location.currentData()['lon']))
        self.gui.snap_na_location.currentIndexChanged.connect(na_location_changed)
        self.gui.snap_na_location.setMaxVisibleItems(10)
        self.gui.snap_na_location.view().setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
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
                self.doubleValidator.setNotation(QDoubleValidator.StandardNotation)
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
        self.gui.snap_na_latitude.setValidator(LatLonValidator(latitude=True))
        def snap_na_latitude_text_edited(text):
            self.gui.snap_na_latitude_value = text
        self.gui.snap_na_latitude.textEdited.connect(snap_na_latitude_text_edited)

        #Longitude box
        self.gui.snap_na_longitude.setValidator(LatLonValidator(longitude=True))
        def snap_na_longitude_text_edited(text):
            self.gui.snap_na_longitude_value = text
        self.gui.snap_na_longitude.textEdited.connect(snap_na_longitude_text_edited)

        #Connect "show advanced settings" check box
        self.gui.snap_na_advanced_settings.setVisible(False)
        self.gui.snap_na_show_advanced.stateChanged.connect(
            lambda: self.gui.snap_na_advanced_settings.setVisible(self.gui.snap_na_show_advanced.isChecked())
        )

        #Check input arguments in advanced box
        self.gui.snap_na_met_model.addItem("EC 0.1, NRPA-domain", MetModel.NrpaEC0p1)
        self.gui.snap_na_met_model.addItem("MEPS 2.5km control-member", MetModel.Meps2p5)
        self.gui.snap_na_met_model.addItem("EC 0.1, global, slow", MetModel.NrpaEC0p1Global)
        self.gui.snap_na_met_model.addItem("Hirlam12", MetModel.H12)
        def snap_na_met_model_changed():
            current_data = self.gui.snap_na_met_model.currentData()
            if (current_data is MetModel.NrpaEC0p1 or current_data is MetModel.NrpaEC0p1Global):
                self.gui.snap_na_met_ec_runtime.setEnabled(True)
            else:
                self.gui.snap_na_met_ec_runtime.setEnabled(False)
        self.gui.snap_na_met_model.currentIndexChanged.connect(snap_na_met_model_changed)

        #EC Meteorology dropdown
        self.gui.snap_na_met_ec_runtime.addItem("Best mixed", "best")
        self.logger.debug("Getting available EC runs")
        for run in self.resources.getECRuns():
            self.gui.snap_na_met_ec_runtime.addItem(run)
        self.logger.debug("Done getting available EC runs")

        self.gui.snap_na_release_npp.toggled.connect(
            lambda val: (
                    self.gui.snap_na_release_npp.setChecked(val),
                    self.gui.snap_na_release_bomb.setChecked(not val)
                    )
        )
        self.gui.snap_na_release_bomb.toggled.connect(
            lambda val: (
                    self.gui.snap_na_release_npp.setChecked(not val),
                    self.gui.snap_na_release_bomb.setChecked(val)
                    )
        )


        #Add items to bomb yield selector
        for kt in [1, 3, 10, 30, 100, 300, 1000, 3000]:
            self.gui.snap_na_bomb_yield.addItem("{:d} kt".format(kt), kt)
        self.gui.snap_na_bomb_yield.setCurrentIndex(2)

        self.gui.snap_na_bottom.setText("15")
        self.gui.snap_na_bottom.setValidator(QDoubleValidator(0, 50000, 2))
        self.gui.snap_na_bottom.validator().setNotation(QDoubleValidator.StandardNotation)

        self.gui.snap_na_top.setText("500")
        self.gui.snap_na_top.setValidator(QDoubleValidator(0, 50000, 2))
        self.gui.snap_na_top.validator().setNotation(QDoubleValidator.StandardNotation)

        def snap_na_bottom_text_changed():
            a = self.gui.snap_na_bottom.text()
            b = self.gui.snap_na_top.text()
            if a and b and float(a) > float(b):
                self.gui.snap_na_top.setText(a)
        self.gui.snap_na_bottom.editingFinished.connect(snap_na_bottom_text_changed)

        def snap_na_top_text_changed():
            a = self.gui.snap_na_bottom.text()
            b = self.gui.snap_na_top.text()
            if a and b and float(a) > float(b):
                self.gui.snap_na_bottom.setText(b)
        self.gui.snap_na_top.editingFinished.connect(snap_na_top_text_changed)

        self.gui.snap_na_radius.setText("50")
        self.gui.snap_na_radius.setValidator(QDoubleValidator(0, 50000, 2))
        self.gui.snap_na_radius.validator().setNotation(QDoubleValidator.StandardNotation)
        self.gui.snap_na_cs137.setText("2.6e+11")
        self.gui.snap_na_cs137.setValidator(QDoubleValidator(0, 1.0e100, 5))
        self.gui.snap_na_xe133.setText("1.0e+13")
        self.gui.snap_na_xe133.setValidator(QDoubleValidator(0, 1.0e100, 5))
        self.gui.snap_na_i131.setText("1.39e+13")
        self.gui.snap_na_i131.setValidator(QDoubleValidator(0, 1.0e100, 5))

        #plot region data
        self.gui.snap_na_plot_region.addItem("No E-Mail", "")
        self.gui.snap_na_plot_region.addItem("Geografisk", "Geografisk")
        self.gui.snap_na_plot_region.addItem("Merkator", "Merkator")
        self.gui.snap_na_plot_region.addItem("Globalt", "Globalt")
        self.gui.snap_na_plot_region.addItem("N.halvkule", "N.halvkule")
        self.gui.snap_na_plot_region.addItem("N.halvkule-90", "N.halvkule-90")
        self.gui.snap_na_plot_region.addItem("N.halvkule+90", "N.halvkule+90")
        self.gui.snap_na_plot_region.addItem("S.halvkule", "S.halvkule")
        self.gui.snap_na_plot_region.addItem("S.halvkule+180", "S.halvkule+180")
        self.gui.snap_na_plot_region.addItem("Atlant", "Atlant")
        self.gui.snap_na_plot_region.addItem("Europa", "Europa")
        self.gui.snap_na_plot_region.addItem("N-Europa", "N-Europa")
        self.gui.snap_na_plot_region.addItem("Skandinavia", "Skandinavia")
        self.gui.snap_na_plot_region.addItem("Norge", "Norge")
        self.gui.snap_na_plot_region.addItem("S-Norge", "S-Norge")
        self.gui.snap_na_plot_region.addItem("N-Norge", "N-Norge")
        self.gui.snap_na_plot_region.addItem("Norge.20W", "Norge.20W")
        self.gui.snap_na_plot_region.addItem("VA-Norge", "VA-Norge")
        self.gui.snap_na_plot_region.addItem("VV-Norge", "VV-Norge")
        self.gui.snap_na_plot_region.addItem("VNN-Norge", "VNN-Norge")

        #self.gui.snap_na_bdiana_version.setValidator(QDoubleValidator(0, 50000, 5))

        self.gui.snap_na_run.clicked.connect(self.run)
