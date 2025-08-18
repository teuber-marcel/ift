import pdb

from PyQt5.QtCore import *
from PyQt5.QtWidgets import *
from PyQt5.QtGui import *

from .menu_bar import *
from .main_panel import ToolPanel, ImagePanel
from . import numpy_qt as npqt
from . import util


# CONVENTION: Google Python Style Guide
# module_name, package_name, ClassName, method_name, ExceptionName,
# function_name, GLOBAL_CONSTANT_NAME, global_var_name, instance_var_name,
# function_parameter_name, local_var_name
# camelCase only to conform to pre-existing conventions





class Data():
    def __init__(self, main_image=None, grad_image=None, label_image=None):
        self.main_image = main_image
        self.grad_image = grad_image
        self.label_image = label_image


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.data = Data()

        self.tool_panel = None
        self.image_panel = None

        self.init_ui()

        self.show()


    def init_ui(self):
        stylesheet = util.read_stylesheet("ui/styles/style.qss")
        self.setStyleSheet(stylesheet)
        self.setCentralWidget(QWidget())
        self.set_main_window_general_properties()

        self.image_panel = ImagePanel(self)
        self.tool_panel = ToolPanel(self)

        # creates a layout and set it to the centralWidget
        main_layout = QGridLayout(self.centralWidget())
        main_layout.addWidget(self.tool_panel, 0, 0)
        main_layout.addWidget(self.image_panel, 0, 1)

        # since our MenuBar connects some functions/slots from other files
        # and objects to actions, we load it at the end
        self.setMenuBar(MenuBar(self)) # self (MainWindow) is the parent


    def set_main_window_general_properties(self):
        self.setWindowTitle("Caos Image Visualizer")
        # self.setMinimumSize(800, 500)
        self.setGeometry(150, 45, 900, 700)
        # self.statusBar().showMessage('Ready')



