from PyQt5.QtWidgets import QWidget
from PyQt5.QtCore import QSize


class StatusBar(QWidget):

    def __init__(self, editor):
        super(StatusBar, self).__init__(editor)
        self.codeEditor = editor

    def sizeHint(self):
        return QSize(self.codeEditor.width(), 0)

    def paintEvent(self, event):
        """It Invokes the draw method(statusBarPaintEvent) in CodeEditor"""
        self.codeEditor.statusBarPaintEvent(event)
