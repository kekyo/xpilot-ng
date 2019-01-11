from MapObject import MapObject

class MapBall(MapObject):
    def __init__(self, x = 0, y = 0, team = 1, bmstore = None):
        MapObject.__init__(self, x, y, 21 * 64, 21 * 64, bmstore["ball.gif"])
        self.team = team

    def printXml(self, file):
        file.write('<Ball x="%d" y="%d" team="%d"/>\n' % (
            self.x, self.y, self.team))
