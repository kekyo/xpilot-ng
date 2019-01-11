from javastuff import *

class PolygonStyle:
    def __init__(self):
        self.visible = 1
        self.visibleInRadar = 1
        self.color = 0xFFFFFF
        self.texture = None
        self.filled = 0
        self.textured = 0
        self.defaultEdgeStyle = None
        id = ""

    def setFromFlags(self, flags):
        self.filled = not not flags & 1
        self.textured = not not flags & 2
        self.visible = not flags & 4
        self.visibleInRadar = not flags & 8

    def computeFlags(self):
        flags = 0
        if self.filled:
            flags |= 1
        if self.textured:
            flags |= 2
        if not self.visible:
            flags |= 4
        if not self.visibleInRadar:
            flags |= 8
        return flags

    def printXml(self, file):
        file.write('<Polystyle id="%s"' % self.id)
        file.write(' color="%s"' % toRGB(self.color))
        if self.texture != None:
            file.write(' texture="%s"' % self.texture.fileName)
        file.write(' defedge="%s" flags="%s"/>\n' % (
            self.defaultEdgeStyle.id, self.computeFlags()))
