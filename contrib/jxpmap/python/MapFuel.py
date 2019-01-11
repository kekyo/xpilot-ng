from MapObject import MapObject

# !@# One-team fuels missing, would be easy to implement

class MapFuel(MapObject):
    def __init__(self, x = 0, y = 0, bmstore = None):
        MapObject.__init__(self, x, y, 35 * 64, 35 * 64, bmstore['fuel.gif'])

    def printXml(self, file):
        file.write('<Fuel x="%d" y="%d"/>\n' % (self.x, self.y))
