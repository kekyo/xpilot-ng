from javastuff import *
from MapObject import MapObject
from PolygonStyle import PolygonStyle
from LineStyle import LineStyle
from PolygonPropertyEditor import PolygonPropertyEditor

import gtk
import GDK

class MapPolygon(MapObject):
    TYPE_NORMAL = 0
    TYPE_BALLAREA = 1
    TYPE_BALLTARGET = 2
    TYPE_DECORATION = 3

    def __init__(self, p = None, style = None, edgeStyles = None,bmstore=None):
        self.polygon = p
        self.style = style
        self.edgeStyles = edgeStyles
        self.type = self.TYPE_NORMAL
        self.team = 1

    def getCreateHandler(self):
        return CreateHandler(self)

    def getBounds(self):
        return self.polygon.getBounds()

    def contains(self, canvas, p):
        for wrap in canvas.wrapOffsets(self.getBounds(), p):
            if self.polygon.contains(Point(p.x + wrap[0], p.y + wrap[1])):
                return 1
        return 0

    def getZOrder(self):
        return 20

    def getPreviewShape(self):
        return self.polygon

    def moveTo(self, x, y):
        self.polygon.transform(AffineTransform(x - self.x, y - self.y))

    def paint(self, di):
        gc = di.gc
        colormap = di.colormap
        tx = di.tx

        trans = [tx(p) for p in self.polygon.points]

        bounds = self.polygon.getBounds()
        if max(bounds.width, bounds.height) * di.canvas.scale > 30000:
            # gtk doesn't like getting coordinates bigger than 32767.
            # Those cause an illogical error message
            # "TypeError: sequence member not a 2-tuple"
            for p in trans:
                if p.x > 30000:
                    p.x = 30000
                elif p.x < -30000:
                    p.x = -30000
                if p.y > 30000:
                    p.y = 30000
                elif p.y < -30000:
                    p.y = -30000

        points = [(t.x, t.y) for t in trans]

        nofill = 1
        if self.style.textured and di.canvas.textured:
            if self.style.texture.scalable:
                gc.tile = self.style.texture.image[di.scale * 64]
            else:
                gc.tile = self.style.texture.image[1]
            # !@# Should align the texture
            gc.fill = GDK.TILED
            nofill = 0
        elif self.style.filled and di.canvas.filled:
            gc.foreground = colormap.alloc(*gcolor(self.style.color))
            nofill = 0
        if not nofill:
            gtk.draw_polygon(di.area, gc, gtk.TRUE, points)
            gc.fill = GDK.SOLID

        if self.edgeStyles is None:
            df = self.style.defaultEdgeStyle
            if df.style != LineStyle.STYLE_HIDDEN or nofill:
                gc.foreground = colormap.alloc(*gcolor(df.color))
                gtk.draw_polygon(di.area, gc, gtk.FALSE, points)
                # !@# dashed lines, width missing here
        else:
            si = 0
            cs = self.edgeStyles[0]
            for i in range(1, len(points)):
                if self.edgeStyles[i] == cs:
                    continue
                lstyle = cs or self.style.defaultEdgeStyle
                if lstyle.style != LineStyle.STYLE_HIDDEN or nofill:
                    gc.foreground = colormap.alloc(*gcolor(lstyle.color))
                    gtk.draw_lines(di.area, gc, points[si:i+1])
                si = i
                cs = self.edgeStyles[i]
            points2 = points[si:]
            points2.append(points[0])
            lstyle = cs or self.style.defaultEdgeStyle
            if lstyle.style != LineStyle.STYLE_HIDDEN or nofill:
                gc.foreground = colormap.alloc(*gcolor(lstyle.color))
                gtk.draw_lines(di.area, gc, points2)

        if di.canvas.showPoints:
            for p in points:
                gtk.draw_rectangle(di.area, di.white_gc, gtk.FALSE,
                                   p[0] - 1, p[1] - 1, 2, 2)

    def getPropertyEditor(self, canvas):
        return PolygonPropertyEditor(canvas, self)

    def checkEvent(self, canvas, me):
        b = self.getBounds()
        p = me.point
        pl = self.polygon
        THRESHOLD = 9 / canvas.scale**2

        larger = Rectangle(b.x - 20 * 64, b.y - 20 * 64,
                           b.width + 40 * 64, b.height + 40 * 64)
        wraps = canvas.wrapOffsets(larger, p)
        if len(wraps) == 0:
            return 0

        if me.button == 1:
            for i in range(len(pl.points)):
                point = pl.points[i]
                if canvas.wrapdist2(p, point) < THRESHOLD:
                    if canvas.isErase():
                        if self.polygon.npoints > 3:
                            self.removePoint(i)
                            canvas.repaint()
                            canvas.saveUndo()
                    else:
                        canvas.setCanvasEventHandler(PolygonPointMoveHandler(
                            self, me, i, canvas.wrap(p, point)))
                    return 1

        if not canvas.isErase():
            for wrap in wraps:
                for i in range(pl.npoints):
                    p1 = pl.points[i - 1]
                    p2 = pl.points[i]
                    if ptSeqDistSq(p1.x, p1.y, p2.x, p2.y, p.x + wrap[0],
                                   p.y + wrap[1]) < THRESHOLD:
                        if me.button == 1:
                            self.insertPoint(i, p)
                            canvas.setCanvasEventHandler(PolygonPointMoveHandler(
                                self, me, i, wrap))
                            return 1

        # MapObject's move handler needs x and y. Would be nicer to trap
        # their access and return appropriate values then...
        self.x = self.polygon.points[0].x
        self.y = self.polygon.points[0].y
        return MapObject.checkEvent(self, canvas, me)

    def removePoint(self, i):
        del self.polygon.points[i]
        self.polygon.npoints -= 1
        self.polygon.recalculate_bounds()
        if self.edgeStyles is not None:
            del self.edgeStyles[i - 1] # works right for i == 0

    def insertPoint(self, i, p):
        self.polygon.points.insert(i, p)
        self.polygon.npoints += 1
        self.polygon.recalculate_bounds()
        if self.edgeStyles is not None:
            self.edgeStyles.insert(i - 1, self.edgeStyles[i - 1])

    def isCounterClockwise(self):
        x1 = long(self.polygon.points[-1].x)
        y1 = long(self.polygon.points[-1].y)
        area = 0
        for p in self.polygon.points:
            x2 = long(p.x)
            y2 = long(p.y)
            area += x1 * y2 - x2 * y1
            x1, y1 = x2, y2
        return area > 0

    def checkWrapping(self, mapsize):
        xwrap = self.polygon.points[0].x
        xwrap -= xwrap % mapsize.width
        ywrap = self.polygon.points[0].y
        ywrap -= ywrap % mapsize.height
        if xwrap != 0 or ywrap != 0:
            for p in self.polygon.points:
                p.x -= xwrap
                p.y -= ywrap
            self.polygon.recalculate_bounds()

    def printXml(self, file):
        if self.polygon.npoints < 3:
            raise "Invalid polygon with less than 3 points"

        if self.type == self.TYPE_BALLAREA:
            file.write('<BallArea>\n')
        elif self.type == self.TYPE_BALLTARGET:
            file.write('<BallTarget team="%d">\n' % self.team)
        elif self.type == self.TYPE_DECORATION:
            file.write('<Decor>\n')

        points = self.polygon.points[:]
        es = self.edgeStyles
        cls = None
        if es is None:
            es = [None] * len(points)
        file.write('<Polygon x="%d" y="%d" style="%s">\n' % (
                   points[0].x, points[0].y, self.style.id))

        if not self.isCounterClockwise():
            points.reverse()
            es = es[:]
            es.reverse()
        else:
            points.append(points.pop(0))

        pp = points[-1]
        for p, e in zip(points, es):
            file.write('<Offset x="%d" y="%d"' % (p.x - pp.x, p.y - pp.y))
            if e is not cls:
                if e is None:
                    id = self.style.defaultEdgeStyle.id
                else:
                    id = e.id
                file.write(' style="%s"' % e)
            file.write('/>\n')
            pp = p
        file.write('</Polygon>\n')

        if self.type == self.TYPE_BALLAREA:
            file.write('</BallArea>\n')
        elif self.type == self.TYPE_BALLTARGET:
            file.write('</BallTarget>\n')
        elif self.type == self.TYPE_DECORATION:
            file.write('</Decor>\n')


# !@# Can create starting points with incorrect wrapping
class CreateHandler(MouseEventHandler):
    def __init__(self, object):
        self.object = object
        self.poly = None
        self.remove = 0

    def mousePressed(self, me):
        c = me.canvas
        gc = c.previewgc
        if self.poly is None:
            self.poly = Polygon()
            first = me.point
            self.poly.addPoint(first.x, first.y)
            self.poly.addPoint(first.x, first.y)
            return
        c = me.canvas
        if self.remove:
            c.drawShape(self.poly, gc)
            self.remove = 0
        latest = me.point
        self.poly.points[self.poly.npoints - 1] = latest
        if me.button == 3:
            if len(self.poly.points) > 2:
                self.poly.recalculate_bounds()
                self.object.polygon = self.poly
                self.object.style = c.getModel().getDefaultPolygonStyle()
                self.object.checkWrapping(c.model.options.size)
                c.getModel().addToFront(self.object)
            c.setCanvasEventHandler(None)
            c.repaint()
            c.saveUndo()
            return
        self.poly.addPoint(latest.x, latest.y)
        c.drawShape(self.poly, gc)
        self.remove = 1

    def mouseMoved(self, me):
        c = me.canvas
        if self.poly is None:
            return
        if self.remove:
            c.drawShape(self.poly)
            self.remove = 0
        self.poly.points[self.poly.npoints - 1] = me.point
        # Bounds not recalculated here so might not be 100% correct
        c.drawShape(self.poly)
        self.remove = 1

class PolygonPointMoveHandler(MouseEventHandler):
    def __init__(self, obj, me, index, wrap):
        self.obj = obj
        self.index = index
        self.virgin = 1
        self.wrap = wrap
        ip = index - 1 % obj.polygon.npoints
        prev = obj.polygon.points[(index - 1) % obj.polygon.npoints]
        self.l1 = Line(prev, prev)
        next = obj.polygon.points[(index + 1) % obj.polygon.npoints]
        self.l2 = Line(next, next)

    def mouseMoved(self, evt):
        c = evt.canvas
        if not self.virgin:
            self.drawPreview(c)
        else:
            self.virgin = 0
        p = evt.point
        p.x += self.wrap[0]
        p.y += self.wrap[1]
        self.l1.p2 = self.l2.p2 = evt.point
        # Bounds not recalculated...
        self.drawPreview(c)

    def mouseReleased(self, evt):
        c = evt.canvas
        p = evt.point
        p.x += self.wrap[0]
        p.y += self.wrap[1]
        self.obj.polygon.points[self.index] = evt.point
        self.obj.polygon.recalculate_bounds()
        if self.index == 0:
            self.obj.checkWrapping(c.model.options.size)
        c.setCanvasEventHandler(None)
        c.repaint()
        c.saveUndo()

    def drawPreview(self, c):
        c.drawShape(self.l1)
        c.drawShape(self.l2)
