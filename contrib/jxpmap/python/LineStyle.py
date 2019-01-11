from javastuff import *

class LineStyle:
    STYLE_SOLID = 0
    STYLE_ONOFFDASH = 1
    STYLE_DOUBLEDASH = 2
    STYLE_HIDDEN = 3
    STYLE_INTERNAL = 4

    def __init__(self, id = None, width = 1, color = 0, style = STYLE_SOLID):
        self.id = id
        self.width = width
        self.color = color
        self.style = style

    def printXml(self, file):
        if self.id == 'internal':
            return
        if self.style == self.STYLE_HIDDEN:
            w = -1
            s = self.STYLE_SOLID
        else:
            w = self.width
            s = self.style
        file.write('<Edgestyle id="%s" width="%d" color="%s" style="%d"/>\n' %
                   (self.id, w, toRGB(self.color), s))
