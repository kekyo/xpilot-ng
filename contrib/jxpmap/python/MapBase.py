from MapObject import MapObject

class MapBase(MapObject):
    LEFT = 0
    DOWN = 1
    RIGHT = 2
    UP = 3

    def __init__(self, x = 0, y = 0, dir = 32, team = 1, bmstore = None):
        if dir < 16:
            imgname = 'base_left.gif'
        elif dir < 48:
            imgname = 'base_down.gif'
        elif dir < 80:
            imgname = 'base_right.gif'
        elif dir < 112:
            imgname = 'base_up.gif'
        else:
            imgname = 'base_left.gif'
        MapObject.__init__(self, x, y, 35 * 64, 35 * 64, bmstore[imgname])
        self.dir = dir
        self.team = team

    def printXml(self, file):
        file.write('<Base x="%d" y="%d" team="%d" dir="%d"/>\n' % (
            self.x, self.y, self.team, self.dir))
