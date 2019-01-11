from __future__ import nested_scopes
from LineStyle import LineStyle
from PolygonStyle import PolygonStyle
from MapOptions import MapOptions
from MapBall import MapBall
from MapPolygon import MapPolygon
from MapBase import MapBase
from MapFuel import MapFuel
from MapCheckPoint import MapCheckPoint
from Pixmap import Pixmap
from javastuff import *
import xml.sax

class Poly:
    def __init__(self):
        self. hasSpecialEdges = 0
        self.points = []
        self.type = MapPolygon.TYPE_NORMAL
        self.team = 1

class PolyPoint:
    def __init__(self, x, y, style):
        self.x = int(x)
        self.y = int(y)
        self.style = style

class PolyStyle:
    def __init__(self, id, color, textureId, defEdgeId, flags):
        self.id = id
        self.color = color
        self.textureId = textureId
        self.defEdgeId = defEdgeId
        self.flags = flags

class MapModel:
    def __init__(self):
        self.defaultScale = 1./64
        self.objects = []
        self.pixmaps = []
        self.polyStyles = []
        self.edgeStyles = []
        self.options = MapOptions()

        ls = LineStyle("internal", 0, 0xFFFF, LineStyle.STYLE_INTERNAL)
        self.edgeStyles.append(ls)
        ls = LineStyle("default", 0, 0xFF, LineStyle.STYLE_SOLID)
        self.edgeStyles.append(ls)
        self.defEdgeStyleIndex = 1
        ps = PolygonStyle()
        ps.id = "default"
        ps.visible = ps.visibleInRadar= 1
        ps.filled = ps.textured = 0
        ps.defaultEdgeStyle = ls
        self.polyStyles.append(ps)
        self.defPolygonStyleIndex = 1

    def getDefaultScale(self):
        return self.defaultScale

    def setDefaultScale(self, ds):
        self.defaultScale = ds

    def getDefaultPolygonStyle(self):
        if self.defPolygonStyleIndex >= len(self.polyStyles):
            self.defPolygonStyleIndex = len(self.polyStyles) - 1
        return self.polyStyles[self.defPolygonStyleIndex]

    def setDefaultPolygonStyle(self, index):
        self.defPolygonStyleIndex = index

    def load(self, file, bmstore):
        self.edgeStyles = []
        ls = LineStyle("internal", 0, 0xFFFF, LineStyle.STYLE_INTERNAL)
        self.edgeStyles.append(ls)
        self.polyStyles = []

        xml.sax.parse(file, MapDocumentHandler(self, bmstore))
        self.defEdgeStyleIndex = len(self.edgeStyles) - 1
        self.defPolygonStyleIndex = len(self.polyStyles) - 1

    def save(self, file):
        file.write('<XPilotMap>\n')
        self.options.printXml(file)
        for pm in self.pixmaps:
            pm.printXml(file)
        for es in self.edgeStyles:
            es.printXml(file)
        for ps in self.polyStyles:
            ps.printXml(file)
        o = self.objects[:]
        o.reverse()
        for obj in o:
            obj.printXml(file)
        file.write('</XPilotMap>\n')
        file.close()

    def addToFront(self, moNew):
        self.addObject(moNew, 1)

    def addToBack(self, moNew):
        self.addObject(moNew, 0)

    def addObject(self, moNew, front):
        znew = moNew.getZOrder()
        for i in range(len(self.objects)):
            zold = self.objects[i].getZOrder()
            if znew < zold or front and znew == zold:
                self.objects.insert(i, moNew)
                return
        self.objects.append(moNew)

    def removeObject(self, mo):
        self.objects.remove(mo)

class MapParseError(Exception):
    pass

class MapDocumentHandler(xml.sax.ContentHandler):
    def __init__(self, model, bmstore):
        self.model = model
        self.bmstore = bmstore
        self.polys = []
        self.estyles = {}
        self.estyles["internal"] = model.edgeStyles[0]
        self.pstyles = {}
        self.bstyles = {}
        self.opMap = {}
        self.ballArea = 0
        self.ballTargetTeam = -1
        self.decor = 0

    def startElement(self, name, atts):
        bm = self.bmstore
        name = name.lower()
        def getxy():
            return int(atts["x"]), int(atts["y"])
        def getint(name, base = 10):
            return int(atts[name], base)
        def addobj(o):
            self.model.addToFront(o)
        if name == "polygon":
            poly = Poly()
            poly.style = atts["style"]
            poly.points.append(PolyPoint(atts["x"], atts["y"], None))
            if self.ballArea:
                poly.type = MapPolygon.TYPE_BALLAREA
            elif self.ballTargetTeam != -1:
                poly.type = MapPolygon.TYPE_BALLTARGET
                poly.team = self.ballTargetTeam
            elif self.decor:
                poly.type = MapPolygon.TYPE_DECORATION
            self.poly = poly
        elif name == "offset":
            es = atts.get("style")
            if es != None:
                self.poly.hasSpecialEdges = 1
            self.poly.points.append(PolyPoint(atts["x"], atts["y"], es))
        elif name == 'edgestyle':
            id = atts["id"]
            color = getint("color", 16)
            width = getint("width")
            style = getint("style")
            if width == -1:
                width = 0
                style = LineStyle.STYLE_HIDDEN
            ls = LineStyle(id, width, color, style)
            self.estyles[id] = ls
            self.model.edgeStyles.append(ls)
        elif name == 'polystyle':
            id = atts["id"]
            cstr = atts.get("color")
            if cstr is not None:
                col = int(cstr, 16)
            else:
                col = None
            flagstr = atts.get('flags', '1')
            flags = int(flagstr)
            self.pstyles[id] = PolyStyle(id, col, atts.get("texture"),
                                         atts["defedge"], flags)
        elif name == 'bmpstyle':
            id = atts["id"]
            fileName = atts["filename"]
            flags = getint("flags")
            pm = Pixmap(self.bmstore, fileName)
            pm.fileName = fileName
            pm.scalable = flags != 0
            self.bstyles[id] = pm
            self.model.pixmaps.append(pm)
        elif name == 'option':
            self.opMap[atts["name"]] = atts["value"]
        elif name == 'fuel':
            x, y = getxy()
            addobj(MapFuel(x, y, bm))
        elif name == 'ball':
            x, y = getxy()
            team = getint("team")
            addobj(MapBall(x, y, team, bm))
        elif name == 'base':
            x, y = getxy()
            dir = getint("dir")
            team = getint("team")
            addobj(MapBase(x, y, dir, team, bm))
        elif name == 'check':
            x,y = getxy()
            addobj(MapCheckPoint(x, y, bm))
        elif name == 'ballarea':
            self.ballArea = 1
        elif name == 'balltarget':
            self.ballTargetTeam = getint("team")
        elif name == 'decor':
            decor = 1
        elif name == 'xpilotmap':
            pass
        elif name == 'generaloptions':
            pass
        else:
            print "Tag: ", name
            raise MapParseError("Unknown tag")

    def endElement(self, name):
        name = name.lower()
        if name == 'polygon':
            self.polys.append(self.poly)
        elif name == 'ballarea':
            self.ballArea = 0
        elif name == 'balltarget':
            self.ballTargetTeam = -1
        elif name == 'decor':
            self.decor = 0

    def endDocument(self):
        self.model.options = MapOptions(self.opMap)
        for ps in self.pstyles.values():
            style = PolygonStyle()
            style.id = ps.id
            style.setFromFlags(ps.flags)
            if style.textured:
                if ps.textureId is None:
                    raise MapParseError("Textured polygon, no texture name")
                style.texture = self.bstyles[ps.textureId]
            if ps.color is None:
                style.color = 0xFFFFFF
            else:
                style.color = ps.color
            ls = self.estyles[ps.defEdgeId]
            style.defaultEdgeStyle = ls
            self.model.polyStyles.append(style)
            ps.ref = style
        for p in self.polys:
            ps = self.pstyles[p.style]
            style = ps.ref
            defls = style.defaultEdgeStyle
            if p.hasSpecialEdges:
                edges = []
            else:
                edges = None
            awtp = Polygon()
            pnt = p.points[0]
            x = pnt.x
            y = pnt.y
            awtp.addPoint(x, y)
            for pnt in p.points[1:-1]:
                x += pnt.x
                y += pnt.y
                awtp.addPoint(x, y)
            cls = None
            if edges != None:
                for pnt in p.points[1:]:
                    if pnt.style != None:
                        cls = self.estyles[pnt.style]
                        if cls == defls:
                            cls = None
                    edges.append(cls)
            mp = MapPolygon(awtp, style, edges)
            mp.type = p.type
            mp.team = p.team
            self.model.addToFront(mp)
