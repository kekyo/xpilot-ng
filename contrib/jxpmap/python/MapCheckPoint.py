from MapObject import MapObject


class MapCheckPoint(MapObject):
    def __init__(self, x = 0, y = 0, bmstore = None):
        MapObject.__init__(self, x, y, 35 * 64, 35 * 64,
                           bmstore["checkpoint.gif"])

    def printXml(self, file):
        file.write('<Check x="%d" y="%d"/>\n' % (self.x, self.y))
